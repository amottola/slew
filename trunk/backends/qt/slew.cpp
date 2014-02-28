#include "slew.h"
#include "datetime.h"

#include "qt_it.h"
#include "unzip.h"

#include "constants/gdi.h"
#include "constants/widget.h"

#include "frame.h"
#include "dialog.h"
#include "foldpanel.h"
#include "menuitem.h"
#include "sceneview.h"
#include "objects.h"


#ifdef Q_OS_MAC
	#include <CoreFoundation/CoreFoundation.h>
	#include <CoreServices/CoreServices.h>
	#include <ApplicationServices/ApplicationServices.h>
	
	#include <mach-o/dyld.h>
	#include <sys/param.h>
	#include <sys/types.h>
	#include <sys/sysctl.h>
	#include <crt_externs.h>
	#include <dlfcn.h>
	
	static bool in_bundle(void)
	{
		/* This comes from the ADC tips & tricks section: how to detect if the app lives inside a bundle */
		FSRef processRef;
		ProcessSerialNumber psn = { 0, kCurrentProcess };
		FSCatalogInfo processInfo;
		GetProcessBundleLocation(&psn, &processRef);
		FSGetCatalogInfo(&processRef, kFSCatInfoNodeFlags, &processInfo, NULL, NULL, NULL);
		if (processInfo.nodeFlags & kFSNodeIsDirectoryMask) 
			return true;
		else
			return false;
	}
	
	static PyObject *sCurrentDockMenu = NULL;

#elif defined(Q_OS_WIN)
	#include <shlobj.h>
	#include <shlwapi.h>
	#include <direct.h>
	#define SECURITY_WIN32
	#include <security.h>
	#define strdup				_strdup
#else
	#include <unistd.h>
	#include <dlfcn.h>
	#include <locale.h>
	#include <pwd.h>
#endif

#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QSemaphore>
#include <QPointer>
#include <QMutexLocker>
#include <QAbstractItemView>
#include <QAbstractButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QShortcut>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QFile>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QUrl>
#include <QStyle>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QTranslator>
#include <QLocale>
#include <QColorDialog>
#include <QFontDialog>
#include <QFileDialog>
#include <QPixmapCache>
#include <QClipboard>
#include <QTimer>
#include <QDragMoveEvent>
#include <QFileOpenEvent>
#include <QLineEdit>
#include <QTextEdit>
#include <QListView>
#include <QTreeView>
#include <QScrollBar>
#include <QScrollArea>
#include <QMetaObject>
#include <QSettings>
#include <QBitmap>
#include <QFontDatabase>
#include <QPrintDialog>
#include <QDialogButtonBox>
#include <QEventLoop>
#include <QNetworkProxyFactory>
#include <QDrag>


class ResourceReader;
class InfoBalloon;
class TimedCall;

static PyObject *sApplication = NULL;
static bool sIsRunning = false;
static QEvent::Type sExceptionEvent;
static QEvent::Type sFreeBitmapResourcesEvent;
static int sArgc;
static char **sArgv;
static QLocale sLocale;
static QTranslator sTranslator;
static QWidget *sMouseGrabber = NULL;
static int sPyObjectMetaType;
static QHash<QString, bool> sSkipException;
static InfoBalloon *sInfoBalloon = NULL;
static bool sModalBalloon = false;
static ResourceReader *sResourceReader = NULL;
static PyObject *sVectorType;
static PyObject *sColorType;
static PyObject *sFontType;
static PyObject *sBitmapType;
static PyObject *sIconType;
static PyObject *sSerializeData = NULL;
static PyObject *sUnserializeData = NULL;
static QHash<int, TimedCall *> sTimers;

PyObject *PyDC_Type;
PyObject *PyPrintDC_Type;
PyObject *PyPaper_Type;
PyObject *PyDataIndex_Type;
PyObject *PyDataSpecifier_Type;
PyObject *PyDataModel_Type;
PyObject *PyEvent_Type = NULL;

static int encodeButtons(int buttons);
static int decodeButton(int button);



class TimedCall : public QObject
{
	Q_OBJECT
	
public:
	TimedCall(QObject *parent, PyObject *func, PyObject *args)
		: QObject(), fFunc(func), fArgs(args), fID(-1)
	{
		Py_XINCREF(func);
		Py_XINCREF(args);
	}
	
	~TimedCall()
	{
		if (Py_IsInitialized()) {
			PyAutoLocker locker;
			
			Py_XDECREF(fFunc);
			Py_XDECREF(fArgs);
		}
	}
	
	void execute()
	{
		if (Py_IsInitialized()) {
			PyAutoLocker locker;
			
			QObject *parent = QObject::parent();
			if (parent) {
				EventRunner runner(parent, "onTimer");
				if (runner.isValid()) {
					if (fArgs)
						runner.set("args", fArgs, false);
					else
						runner.set("args", PyTuple_New(0));
					runner.run();
				}
			}
			else {
				PyObject *result = PyObject_CallObject(fFunc, fArgs);
				if (!result) {
					PyErr_Print();
					PyErr_Clear();
				}
				else {
					Py_DECREF(result);
				}
			}
		}
	}
	
	void setId(int id) { fID = id; }
	
public slots:
	void executeAndDelete()
	{
		execute();
		deleteLater();
	}
	
	void handleDestroyed()
	{
		QObject *parent = QObject::parent();
		
		if ((parent) && (fID >= 0)) {
			parent->killTimer(fID);
			delete sTimers.take(fID);
		}
	}

private:
	PyObject		*fFunc;
	PyObject		*fArgs;
	int				fID;
};



class ExceptionDialog : public QDialog
{
	Q_OBJECT
	
public:
	ExceptionDialog(const QString& message)
		: QDialog(), fStopReporting(false)
	{
		QVBoxLayout *layout = new QVBoxLayout;
		QHBoxLayout *hbox = new QHBoxLayout;
		QVBoxLayout *vbox = new QVBoxLayout;
		QLabel *label;
		QPushButton *button;
		QCheckBox *check;
		QFrame *line;
		QFont font;
		QPalette palette;
		
		QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical);
		label = new QLabel;
		label->setPixmap(icon.pixmap(QSize(64,64)));
		hbox->addWidget(label);
		hbox->setAlignment(label, Qt::AlignTop | Qt::AlignLeft);
		hbox->addSpacing(10);
		
		label = new QLabel("A Python exception has occurred");
		font = label->font();
		font.setBold(true);
		label->setFont(font);
		vbox->addWidget(label);
		vbox->addSpacing(10);
		
		font.setBold(false);
		font.setPointSize(11);
		label = new QLabel(message);
		label->setFont(font);
		vbox->addWidget(label);
			
		vbox->addSpacing(10);
		vbox->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		hbox->addLayout(vbox, 1);
		hbox->setAlignment(Qt::AlignTop | Qt::AlignLeft);
		
		layout->addLayout(hbox);
		
		line = new QFrame;
		line->setFrameStyle(QFrame::Sunken | QFrame::HLine);
		layout->addWidget(line);
		layout->addSpacing(10);
		
		hbox = new QHBoxLayout;
		check = new QCheckBox("Don't tell me again");
		check->setChecked(fStopReporting);
		hbox->addWidget(check);
		connect(check, SIGNAL(toggled(bool)), this, SLOT(stopReporting(bool)));
		
		hbox->addStretch(1);
		
		QHBoxLayout *bbox = new QHBoxLayout;
		
		QPushButton *quit = button = new QPushButton("Force quit");
		bbox->addWidget(button, 1);
		connect(button, SIGNAL(clicked()), this, SLOT(accept()));
		
		button = new QPushButton("Ok");
		button->setDefault(true);
		bbox->addWidget(button, 1);
		connect(button, SIGNAL(clicked()), this, SLOT(reject()));
		
		bbox->setSizeConstraint(QLayout::SetFixedSize);
		hbox->addLayout(bbox);
		layout->addLayout(hbox);
		
		setLayout(layout);
		button->setMinimumSize(quit->sizeHint());
		
		setWindowTitle("Python exception");
	}
	
	bool shouldStopReporting() { return fStopReporting; }
	
public slots:
	void stopReporting(bool toggled) { fStopReporting = toggled; }

private:
	bool	fStopReporting;
};



class ExceptionEvent : public QEvent
{
public:
	ExceptionEvent(const QString& message) : QEvent(sExceptionEvent), fMessage(message) {}
	
	void report()
	{
		if (!sSkipException.value(fMessage, false)) {
			PyAutoLocker locker;
			ExceptionDialog dialog(fMessage);
			centerWindow(&dialog);
			
			qApp->removeEventFilter(qApp);
			if (sMouseGrabber)
				sMouseGrabber->releaseMouse();
			
			dialog.exec();
			
			if (sMouseGrabber)
				sMouseGrabber->grabMouse();
			qApp->installEventFilter(qApp);
			
			if (dialog.result() == QDialog::Accepted) {
				qApp->quit();
				exit(-1);
			}
			else {
				if (dialog.shouldStopReporting())
					sSkipException[fMessage] = true;
			}
			
		}
	}

private:
	QString		fMessage;
};



class FreeBitmapResourcesEvent : public QEvent
{
public:
	FreeBitmapResourcesEvent(QPainter *painter, QPaintDevice *device) : QEvent(sFreeBitmapResourcesEvent), fPainter(painter), fDevice(device) {}
	
	void deleteResources()
	{
		delete fPainter;
		delete fDevice;
	}
	
private:
	QPainter		*fPainter;
	QPaintDevice	*fDevice;
};



class Shortcut : public QShortcut
{
	Q_OBJECT
	
public:
	Shortcut(QWidget *parent, const QKeySequence& key, Qt::ShortcutContext context, PyObject *callback)
		: QShortcut(parent), fCallback(callback)
	{
		setKey(key);
		setContext(context);
		connect(this, SIGNAL(activated()), this, SLOT(handleActivated()));
		Py_INCREF(callback);
	}
	
	~Shortcut() {
		if (Py_IsInitialized()) {
			PyAutoLocker locker;
			Py_DECREF(fCallback);
		}
	}
	
public slots:
	void handleActivated() {
		PyAutoLocker locker;
		PyObject *result = PyObject_CallFunctionObjArgs(fCallback, NULL);
		if (!result) {
			PyErr_Print();
			PyErr_Clear();
		}
		else {
			Py_DECREF(result);
		}
	}

private:
	PyObject		*fCallback;
};



class InfoBalloon : public QWidget
{
	Q_OBJECT

public:
	static void fade() { if (sInfoBalloon) sInfoBalloon->fadeOut(); }
	
	InfoBalloon(QWidget *parent, QWidget *editor, const QString& text, const QPoint& hotspot, int where = SL_TOP, int buttons = 0)
		: QWidget(parent->window(), Qt::ToolTip), fEditor(editor ? editor : parent), fButtonBox(NULL), fWhere(where)
	{
		delete sInfoBalloon;
		sInfoBalloon = this;
		
		fContent = new QWidget(this);
		QVBoxLayout *layout = new QVBoxLayout(fContent);
		QLabel *label = new QLabel(text, fContent);
		QFont font = label->font();
		font.setPointSize(10);
		label->setFont(font);
		label->setWordWrap(true);
		label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
		QSizePolicy policy(QSizePolicy::Minimum, QSizePolicy::Minimum);
		policy.setHeightForWidth(true);
		label->setSizePolicy(policy);
		layout->addWidget(label);
		layout->setContentsMargins(0, 0, 0, 0);
		
		if (buttons) {
			fButtonBox = new QDialogButtonBox((QDialogButtonBox::StandardButtons)encodeButtons(buttons), Qt::Horizontal, this);
			foreach (QAbstractButton *button, fButtonBox->buttons()) {
				button->setAttribute(Qt::WA_MacSmallSize);
				connect(button, SIGNAL(clicked()), this, SLOT(handleClicked()));
			}
			layout->addWidget(fButtonBox);
			fContent->setMinimumWidth(150);
		}
		fContent->setMaximumWidth(350);
		fContent->setMinimumHeight(15);
		fContent->setLayout(layout);
		fContent->resize(fContent->minimumSizeHint());
		
		setAutoFillBackground(false);
		setAttribute(Qt::WA_TranslucentBackground);
		
		doLayout();
	}
	
	void doLayout()
	{
		if (!fEditor) {
			deleteLater();
			return;
		}
		QPoint hotspot;
		QPoint pos, editorPos = fEditor->mapToGlobal(QPoint(0, 0));
		QSize size, editorSize = fEditor->size();
		QRect drect = QApplication::desktop()->availableGeometry();
		int diff, where = fWhere;
		
		size = fContent->sizeHint() + QSize(20, 20);
		
		if (where == SL_LEFT) {
			QSize oldSize = size;
			size.rheight() = qMax(50, size.height());
			hotspot = QPoint(size.width() + 15, size.height() / 2);
			pos = QPoint(-(size.width() + 15), -(size.height() / 2) + (editorSize.height() / 2));
			if (editorPos.x() + pos.x() < drect.left() + 10) {
				where = SL_RIGHT;
				size = oldSize;
			}
		}
		
		if (where == SL_RIGHT) {
			QSize oldSize = size;
			size.rheight() = qMax(50, size.height());
			hotspot = QPoint(0, size.height() / 2);
			pos = QPoint(editorSize.width(), -(size.height() / 2) + (editorSize.height() / 2));
			if (editorPos.x() + pos.x() + size.width() + 15 > drect.right() - 10) {
				where = SL_LEFT;
				size = oldSize;
				size.rheight() = qMax(50, size.height());
				hotspot = QPoint(size.width() + 15, size.height() / 2);
				pos = QPoint(-(size.width() + 15), -(size.height() / 2) + (editorSize.height() / 2));
			}
		}
		
		if ((where == SL_LEFT) || (where == SL_RIGHT)) {
			if (editorPos.y() + pos.y() + size.height() > drect.bottom() - 10)
				where = SL_TOP;
			else if (editorPos.y() + pos.y() < drect.top() + 10)
				where = SL_BOTTOM;
		}
		
		if (where == SL_TOP) {
			hotspot = QPoint(size.width() / 2, size.height() + 15);
			pos = QPoint((editorSize.width() / 2) - (size.width() / 2), -(size.height() + 15));
			if (hotspot.y() + editorPos.y() - size.height() - 15 < drect.top() + 10)
				where = SL_BOTTOM;
		}
		
		if (where == SL_BOTTOM) {
			hotspot = QPoint(size.width() / 2, 0);
			pos = QPoint((editorSize.width() / 2) - (size.width() / 2), editorSize.height());
			if (editorPos.y() + pos.y() + size.height() + 15 > drect.bottom() - 10) {
				where = SL_TOP;
				hotspot = QPoint(size.width() / 2, size.height() + 15);
				pos = QPoint((editorSize.width() / 2) - (size.width() / 2), -(size.height() + 15));
			}
		}
		
		pos += editorPos;
		if ((where == SL_TOP) || (where == SL_BOTTOM)) {
			diff = pos.x() + size.width() - (drect.right() - 10);
			if (diff > 0) {
				pos.rx() -= diff;
				hotspot.rx() += diff;
				if (pos.x() + size.width() > drect.right() - 10)
					pos.setX(drect.right() - 10 - size.width());
				if (pos.x() + size.width() < editorPos.x() + 35)
					pos.setX(editorPos.x() + 35 - size.width());
				hotspot.rx() = qMin(hotspot.x(), size.width() - 25);
			}
			
			diff = drect.left() + 10 - pos.x();
			if (diff > 0) {
				pos.rx() -= diff;
				hotspot.rx() -= diff;
				if (pos.x() < drect.left() + 10)
					pos.setX(drect.left() + 10);
				if (pos.x() > editorPos.x() + editorSize.width() - 35)
					pos.setX(editorPos.x() + editorSize.width() - 35);
				hotspot.rx() = qMax(hotspot.x(), 25);
			}
			
			fContent->resize(size - QSize(20, 20));
			size += QSize(0, 15);
			if (where == SL_TOP)
				fContent->move(10, 10);
			else
				fContent->move(10, 25);
		}
		else {
			fContent->resize(size - QSize(20, 20));
			size += QSize(15, 0);
			if (where == SL_LEFT)
				fContent->move(10, 10);
			else
				fContent->move(25, 10);
		}
		
		fPath = QPainterPath();
		fPath.moveTo(hotspot);
		switch (where) {
		case SL_LEFT:
			fPath.lineTo(size.width() - 15, hotspot.y() - 15);
			fPath.lineTo(size.width() - 15, 10);
			fPath.arcTo(size.width() - 35, 0, 20, 20, 0, 90);
			fPath.lineTo(10, 0);
			fPath.arcTo(0, 0, 20, 20, 90, 90);
			fPath.lineTo(0, size.height() - 10);
			fPath.arcTo(0, size.height() - 20, 20, 20, 180, 90);
			fPath.lineTo(size.width() - 25, size.height());
			fPath.arcTo(size.width() - 35, size.height() - 20, 20, 20, 270, 90);
			fPath.lineTo(size.width() - 15, hotspot.y() + 15);
			break;
		case SL_RIGHT:
			fPath.lineTo(15, hotspot.y() + 15);
			fPath.lineTo(15, size.height() - 10);
			fPath.arcTo(15, size.height() - 20, 20, 20, 180, 90);
			fPath.lineTo(size.width() - 10, size.height());
			fPath.arcTo(size.width() - 20, size.height() - 20, 20, 20, 270, 90);
			fPath.lineTo(size.width(), 10);
			fPath.arcTo(size.width() - 20, 0, 20, 20, 0, 90);
			fPath.lineTo(25, 0);
			fPath.arcTo(15, 0, 20, 20, 90, 90);
			fPath.lineTo(15, hotspot.y() - 15);
			break;
		case SL_TOP:
			fPath.lineTo(hotspot.x() + 15, hotspot.y() - 15);
			fPath.lineTo(size.width() - 10, size.height() - 15);
			fPath.arcTo(size.width() - 20, size.height() - 35, 20, 20, 270, 90);
			fPath.lineTo(size.width(), 10);
			fPath.arcTo(size.width() - 20, 0, 20, 20, 0, 90);
			fPath.lineTo(10, 0);
			fPath.arcTo(0, 0, 20, 20, 90, 90);
			fPath.lineTo(0, size.height() - 25);
			fPath.arcTo(0, size.height() - 35, 20, 20, 180, 90);
			fPath.lineTo(hotspot.x() - 15, size.height() - 15);
			break;
		case SL_BOTTOM:
			fPath.lineTo(hotspot.x() - 15, 15);
			fPath.lineTo(10, 15);
			fPath.arcTo(0, 15, 20, 20, 90, 90);
			fPath.lineTo(0, size.height() - 10);
			fPath.arcTo(0, size.height() - 20, 20, 20, 180, 90);
			fPath.lineTo(size.width() - 10, size.height());
			fPath.arcTo(size.width() - 20, size.height() - 20, 20, 20, 270, 90);
			fPath.lineTo(size.width(), 25);
			fPath.arcTo(size.width() - 20, 15, 20, 20, 0, 90);
			fPath.lineTo(hotspot.x() + 15, 15);
			break;
		}
		fPath.lineTo(hotspot);
		fPath.closeSubpath();
		
#ifdef Q_OS_WIN
		QBitmap mask(size);
		{
			QPainter painter(&mask);
			painter.setRenderHints(0);
			painter.fillRect(QRect(QPoint(0,0), size), Qt::color0);
			painter.setPen(QPen(Qt::color1, 1.5, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
			painter.drawPath(fPath);
			painter.fillPath(fPath, QBrush(Qt::color1));
		}
		setMask(mask);
#endif
		
		move(pos);
		resize(size);
		
		update();
	}
	
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		QPalette palette(style()->standardPalette());
		QBrush brush = palette.brush(QPalette::ToolTipBase);
		QColor color = brush.color();
		
		color.setAlpha(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this));
		brush.setColor(color);
		
		painter.setRenderHints(QPainter::Antialiasing);
		painter.fillPath(fPath, brush);
		painter.setPen(QPen(palette.color(QPalette::WindowText), 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
		painter.drawPath(fPath);
	}
	
	int exec()
	{
		int result;
		
		Completer::hide();
		
		if (fButtonBox) {
			setWindowModality(Qt::ApplicationModal);
			setAttribute(Qt::WA_ShowModal, true);
			
			fEditor->installEventFilter(this);
			sModalBalloon = true;
			
			show();
			fEventLoop = new QEventLoop;
			QPointer<QObject> guard = this;
			result = fEventLoop->exec(QEventLoop::DialogExec);
			if (guard) {
				delete fEventLoop;
				if (fEditor)
					fEditor->removeEventFilter(this);
				deleteLater();
			}
			sModalBalloon = false;
		}
		else {
			show();
			result = 0;
			
			fFader.setDuration(300);
			connect(&fFader, SIGNAL(valueChanged(qreal)), this, SLOT(doFade(qreal)));
			connect(&fFader, SIGNAL(finished()), this, SLOT(deleteLater()));
			
			QTimer::singleShot(10000, this, SLOT(fadeOut()));
		}
		
		return result;
	}
	
	virtual ~InfoBalloon() { sInfoBalloon = NULL; }
	
public slots:
	void fadeOut()
	{
		if ((fButtonBox) || (fFader.state() != QTimeLine::NotRunning))
			return;
		fFader.start();
	}
	
	void doFade(qreal value)
	{
		qreal opaque = style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0;
		setWindowOpacity((1.0 - value) * opaque);
	}
	
	void handleClicked()
	{
		int button = decodeButton((int)fButtonBox->standardButton((QAbstractButton *)sender()));
		fEventLoop->exit(button);
	}
	
protected:
	bool eventFilter(QObject *obj, QEvent *event)
	{
		return ((event->type() == QEvent::KeyPress) || (event->type() == QEvent::KeyRelease));
	}
	
	virtual void mouseReleaseEvent(QMouseEvent *event)
	{
		fadeOut();
		QWidget::mouseReleaseEvent(event);
	}

private:
	QPainterPath			fPath;
	QTimeLine				fFader;
	QPointer<QWidget>		fEditor;
	QDialogButtonBox		*fButtonBox;
	QWidget					*fContent;
	QEventLoop				*fEventLoop;
	int						fWhere;
};



class ResourceReader
{
public:
	ResourceReader(const QString& source) : fSource(source) {}
	virtual ~ResourceReader() {}
	
	virtual bool read(const QString& name, QByteArray& data) = 0;
	
	QString source() { return fSource; }

protected:
	QString		fSource;
};



class DirReader : public ResourceReader
{
public:
	DirReader(const QString& path)
		: ResourceReader(path)
	{
	}
	
	virtual bool read(const QString& name, QByteArray& data)
	{
		QFileInfo path(fSource + QDir::separator() + name);
		QFile file(path.absoluteFilePath());
		if (!file.open(QIODevice::ReadOnly))
			return false;
		data = file.readAll();
		file.close();
		return true;
	}
	
	static bool initCheck(const QString& path)
	{
		QDir dir(path);
		return dir.exists() && dir.isReadable();
	}
};



class ZipReader : public ResourceReader
{
public:
	ZipReader(const QString& path)
		: ResourceReader(path + ".sla")
	{
		fFile = unzOpen(fSource.toUtf8());
	}
	
	~ZipReader()
	{
		if (fFile)
			unzClose(fFile);
	}
	
	virtual bool read(const QString& name, QByteArray& data)
	{
		if (!fFile)
			return false;
		
		unz_file_info info;
		if ((unzLocateFile(fFile, name.toUtf8(), 0) != UNZ_OK) ||
			(unzGetCurrentFileInfo(fFile, &info, NULL, 0, NULL, 0, NULL, 0) != UNZ_OK) ||
			(unzOpenCurrentFile(fFile) != UNZ_OK)) {
			return false;
		}
		
		bool result = true;
		data.resize(info.uncompressed_size);
		if (unzReadCurrentFile(fFile, data.data(), data.size()) != data.size())
			result = false;
		
		unzCloseCurrentFile(fFile);
		
		return result;
	}
	
	static bool initCheck(const QString& path)
	{
		unzFile file = unzOpen((path + ".sla").toUtf8());
		if (!file)
			return false;
		unzClose(file);
		return true;
	}
	
private:
	unzFile		fFile;
};



int
convertBuffer(PyObject *object, QByteArray *value)
{
	if (!object)
		return 0;
	if (PyBuffer_Check(object)) {
		const void *source;
		Py_ssize_t size;
		if (PyObject_AsReadBuffer(object, &source, &size))
			return 0;
		*value = QByteArray((const char *)source, size);
	}
	else {
		Py_buffer view;
		if (PyObject_GetBuffer(object, &view, PyBUF_CONTIG_RO))
			return 0;
		*value = QByteArray((const char *)view.buf, view.len);
		PyBuffer_Release(&view);
	}
	return 1;
}


int
convertPoint(PyObject *object, QPoint *value)
{
	int x, y;
	
	if (!object)
		return 0;
	
	if (object == Py_None) {
		*value = QPoint();
		return 1;
	}
		
	if ((!getObjectAttr(object, "x", &x)) ||
		(!getObjectAttr(object, "y", &y)))
		return 0;
	
	*value = QPoint(x, y);
	return 1;
}


int
convertPointF(PyObject *object, QPointF *value)
{
	double x, y;
	
	if (!object)
		return 0;
	
	if (object == Py_None) {
		*value = QPoint();
		return 1;
	}
		
	if ((!getObjectAttr(object, "x", &x)) ||
		(!getObjectAttr(object, "y", &y)))
		return 0;
	
	*value = QPointF(x, y);
	return 1;
}


int
convertSize(PyObject *object, QSize *value)
{
	QPoint point;
	int result = convertPoint(object, &point);
	*value = QSize(point.x(), point.y());
	return result;
}


int
convertSizeF(PyObject *object, QSizeF *value)
{
	QPointF point;
	int result = convertPointF(object, &point);
	*value = QSizeF(point.x(), point.y());
	return result;
}


int
convertBool(PyObject *object, bool *value)
{
	if (!object)
		return 0;
	*value = PyObject_IsTrue(object) ? true : false;
	return 1;
}


int
convertInt(PyObject *object, int *value)
{
	if (!object)
		return 0;
	if ((PyString_Check(object)) || (PyUnicode_Check(object))) {
		QString str;
		bool ok;
		convertString(object, &str);
		*value = str.toInt(&ok, 0);
		if (ok)
			return 1;
	}
	
	*value = PyInt_AsLong(object);
	if (!PyErr_Occurred())
		return 1;
	return 0;
}


int
convertIntList(PyObject *object, QList<int> *value)
{
	if (!object)
		return 0;
	PyObject *seq = PySequence_Fast(object, "expected tuple or list object");
	if (!seq)
		return 0;
	
	value->clear();
	for (Py_ssize_t i = 0; i < PySequence_Fast_GET_SIZE(seq); i++) {
		int v;
		if (!convertInt(PySequence_Fast_GET_ITEM(seq, i), &v)) {
			Py_DECREF(seq);
			return 0;
		}
		value->append(v);
	}
	Py_DECREF(seq);
	
	return 1;
}


int
convertString(PyObject *object, QString *value)
{
	if (!object)
		return 0;
	if (PyString_Check(object)) {
		*value = QString::fromUtf8(PyString_AS_STRING(object));
	}
	else if (PyUnicode_Check(object)) {
		wchar_t *buffer, *malloced = NULL;
		size_t size = (size_t)PyUnicode_GET_SIZE(object);
		if (size < 65536)
			buffer = (wchar_t *)alloca(sizeof(wchar_t) * size);
		else
			buffer = malloced = (wchar_t *)malloc(sizeof(wchar_t) * size);
		PyUnicode_AsWideChar((PyUnicodeObject *)object, buffer, (Py_ssize_t)size);
		*value = QString::fromWCharArray(buffer, size);
		if (malloced)
			free(malloced);
	}
	else {
		PyErr_SetString(PyExc_TypeError, "expected 'str' or 'unicode' object");
		return 0;
	}
	return 1;
}


int
convertColor(PyObject *object, QColor *value)
{
	int r, g, b, a = 255;
	
	if (!object)
		return 0;
	if (object == Py_None) {
		*value = QColor();
		return 1;
	}
	
	PyObject *seq = PySequence_Fast(object, "expected tuple or list object");
	if (seq) {
		Py_ssize_t size = PySequence_Fast_GET_SIZE(seq);
		if ((size < 3) || (size > 4)) {
			PyErr_SetString(PyExc_ValueError, "expected sequence containing 3 or 4 int elements");
		}
		if ((!convertInt(PySequence_Fast_GET_ITEM(seq, 0), &r)) ||
			(!convertInt(PySequence_Fast_GET_ITEM(seq, 1), &g)) ||
			(!convertInt(PySequence_Fast_GET_ITEM(seq, 2), &b))) {
			Py_DECREF(seq);
			return 0;
		}
		if ((size == 4) && (!convertInt(PySequence_Fast_GET_ITEM(seq, 3), &a))) {
			Py_DECREF(seq);
			return 0;
		}
		Py_DECREF(seq);
	}
	else {
		PyErr_Clear();
		if ((!getObjectAttr(object, "r", &r)) ||
			(!getObjectAttr(object, "g", &g)) ||
			(!getObjectAttr(object, "b", &b)) ||
			(!getObjectAttr(object, "a", &a)))
			return 0;
	}
	
	*value = QColor(r, g, b, a);
	return 1;
}


int
convertFont(PyObject *object, QFont *value)
{
	int family_id, size, style, spacing;
	QString family, face;
	QFont::StyleHint hint;
	QFont::StyleStrategy strategy = (QFont::StyleStrategy)(QFont::PreferOutline | QFont::PreferAntialias);
	QFont font;
	
	if (!object)
		return 0;
	if (object == Py_None) {
		return 1;
	}
	
	if ((!getObjectAttr(object, "family", &family_id)) ||
		(!getObjectAttr(object, "face", &face)) ||
		(!getObjectAttr(object, "size", &size)) ||
		(!getObjectAttr(object, "style", &style)) ||
		(!getObjectAttr(object, "spacing", &spacing)))
		return 0;
	
	if (family_id != SL_FONT_FAMILY_DEFAULT) {
		switch (family_id) {
		case SL_FONT_FAMILY_ROMAN:			hint = QFont::Times; 			family = "serif"; break;
		case SL_FONT_FAMILY_SCRIPT:			hint = QFont::Decorative;		family = "cursive"; break;
		case SL_FONT_FAMILY_SANS_SERIF:		hint = QFont::SansSerif;		family = "sans-serif"; break;
		case SL_FONT_FAMILY_FIXED_PITCH:
		case SL_FONT_FAMILY_TELETYPE:		hint = QFont::TypeWriter;		family = "monospace";
											strategy = QFont::PreferBitmap;	break;
		default:							hint = QFont::System;			break;
		}
		
		font.setStyleHint(hint, QFont::StyleStrategy(strategy | QFont::PreferMatch));
	}
	if (!face.isEmpty())
		family = face;
	if (!family.isEmpty())
		font.setFamily(family);
	
	if (size != SL_FONT_SIZE_DEFAULT) {
#ifndef Q_OS_MAC
		font.setPointSizeF(double(size) * 0.75);
#else
		font.setPointSize(size);
#endif
	}
	
	font.setBold(style & SL_FONT_STYLE_BOLD ? true : false);
	font.setItalic(style & SL_FONT_STYLE_ITALIC ? true : false);
	font.setUnderline(style & SL_FONT_STYLE_UNDERLINED ? true : false);
	font.setKerning(style & SL_FONT_STYLE_NO_KERNING ? false : true);
	font.setLetterSpacing(QFont::AbsoluteSpacing, spacing);
	
	*value = font;
	return 1;
}


int
convertPixmap(PyObject *object, QPixmap *value)
{
	if (!object)
		return 0;
	if (object == Py_None) {
		*value = QPixmap();
		return 1;
	}
	if (PyObject_TypeCheck(object, (PyTypeObject *)sBitmapType)) {
		DC_Proxy *proxy = (DC_Proxy *)PyObject_GetAttrString(object, "_impl");
		if (proxy) {
			*value = *((QPixmap *)proxy->fDevice);
			Py_DECREF(proxy);
			return 1;
		}
		PyErr_Clear();
	}
	PyErr_SetString(PyExc_TypeError, "expected 'Bitmap' object");
	return 0;
}


int
convertIcon(PyObject *object, QIcon *value)
{
	if (!object)
		return 0;
	if (object == Py_None) {
		*value = QIcon();
		return 1;
	}
	if (PyObject_TypeCheck(object, (PyTypeObject *)sIconType)) {
		QPixmap normal, disabled, active, selected;
		if (getObjectAttr(object, "normal", &normal) &&
			getObjectAttr(object, "disabled", &disabled) &&
			getObjectAttr(object, "active", &active) &&
			getObjectAttr(object, "selected", &selected)) {
			
			*value = QIcon();
			value->addPixmap(normal, QIcon::Normal);
			if (!disabled.isNull())
				value->addPixmap(disabled, QIcon::Disabled);
			if (!active.isNull())
				value->addPixmap(active, QIcon::Active);
			if (!selected.isNull())
				value->addPixmap(selected, QIcon::Selected);
			return 1;
		}
		PyErr_Clear();
	}
	else {
		QPixmap normal;
		if (convertPixmap(object, &normal)) {
			*value = QIcon(normal);
			return 1;
		}
		PyErr_Clear();
	}
	PyErr_SetString(PyExc_TypeError, "expected 'Icon' object");
	return 0;
}


int
convertDate(PyObject *object, QDate *value)
{
	if (!object)
		return 0;
	if (object == Py_None) {
		*value = QDate();
		return 1;
	}
	if (PyDate_Check(object)) {
		value->setDate(PyDateTime_GET_YEAR(object), PyDateTime_GET_MONTH(object), PyDateTime_GET_DAY(object));
		return 1;
	}
	PyErr_SetString(PyExc_TypeError, "expected 'datetime.date' or 'datetime.datetime' object");
	return 0;
}


int
convertDateTime(PyObject *object, QDateTime *value)
{
	if (!object)
		return 0;
	if (object == Py_None) {
		*value = QDateTime();
		return 1;
	}
	if (PyDateTime_Check(object)) {
		value->setDate(QDate(PyDateTime_GET_YEAR(object), PyDateTime_GET_MONTH(object), PyDateTime_GET_DAY(object)));
		value->setTime(QTime(PyDateTime_TIME_GET_HOUR(object), PyDateTime_TIME_GET_MINUTE(object), PyDateTime_TIME_GET_SECOND(object), PyDateTime_TIME_GET_MICROSECOND(object) / 1000));
		return 1;
	}
	PyErr_SetString(PyExc_TypeError, "expected 'datetime.datetime' object");
	return 0;
}


bool
getObjectAttr(PyObject *object, const char *name, QByteArray *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertBuffer(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QPoint *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertPoint(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QPointF *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertPointF(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QSize *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertSize(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QSizeF *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertSizeF(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, bool *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	*value = PyObject_IsTrue(object) ? true : false;
	Py_DECREF(attr);
	return true;
}


bool
getObjectAttr(PyObject *object, const char *name, int *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	*value = PyInt_AsLong(attr);
	Py_DECREF(attr);
	return bool(!PyErr_Occurred());
}


bool
getObjectAttr(PyObject *object, const char *name, short *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	*value = (short)PyInt_AsLong(attr);
	Py_DECREF(attr);
	return bool(!PyErr_Occurred());
}


bool
getObjectAttr(PyObject *object, const char *name, double *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	*value = PyFloat_AsDouble(attr);
	Py_DECREF(attr);
	return bool(!PyErr_Occurred());
}


bool
getObjectAttr(PyObject *object, const char *name, QString *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertString(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QColor *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertColor(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QFont *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertFont(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QPixmap *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertPixmap(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QIcon *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertIcon(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QDate *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertDate(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


bool
getObjectAttr(PyObject *object, const char *name, QDateTime *value)
{
	PyObject *attr;
	attr = PyObject_GetAttrString(object, name);
	if (!attr)
		return false;
	int result = convertDateTime(attr, value);
	Py_DECREF(attr);
	return bool(result);
}


PyObject *
createBufferObject(const QByteArray& data)
{
	void *target;
	Py_ssize_t size;
	PyObject *buffer = PyBuffer_New(data.size());
	PyObject_AsWriteBuffer(buffer, &target, &size);
	memcpy(target, data.data(), size);
	return buffer;
}


PyObject *
createVectorObject(const QPoint& point)
{
	return PyObject_CallFunction(sVectorType, "ii", point.x(), point.y());
}


PyObject *
createVectorObject(const QPointF& point)
{
	return PyObject_CallFunction(sVectorType, "dd", point.x(), point.y());
}


PyObject *
createVectorObject(const QSize& size)
{
	return createVectorObject(QPoint(size.width(), size.height()));
}


PyObject *
createVectorObject(const QSizeF& size)
{
	return createVectorObject(QPointF(size.width(), size.height()));
}


PyObject *
createBoolObject(bool boolean)
{
	if (boolean)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}


PyObject *
createIntListObject(const QList<int>& list)
{
	PyObject *object = PyTuple_New(list.size());
	for (int i = 0; i < list.size(); i++) {
		PyTuple_SET_ITEM(object, i, PyInt_FromLong(list[i]));
	}
	return object;
}


PyObject *
createStringObject(const QString& string)
{
	QByteArray data = string.toUtf8();
	return PyUnicode_DecodeUTF8(data.data(), data.size(), NULL);
}


PyObject *
createColorObject(const QColor& color)
{
	if (!color.isValid())
		Py_RETURN_NONE;
	
	return PyObject_CallFunction(sColorType, "iiii", color.red(), color.green(), color.blue(), color.alpha());
}


PyObject *
createFontObject(const QFont& font)
{
	if (font == QApplication::font())
		Py_RETURN_NONE;
	
	int family = SL_FONT_FAMILY_DEFAULT, size, style = 0;
	QString face = font.family();
#ifndef Q_OS_MAC
	size = int(font.pointSizeF() * 1.33333333333);
#else
	size = font.pointSize();
#endif
	if (font.bold())
		style |= SL_FONT_STYLE_BOLD;
	if (font.italic())
		style |= SL_FONT_STYLE_ITALIC;
	if (font.underline())
		style |= SL_FONT_STYLE_UNDERLINED;
	
	int spacing = font.letterSpacing();
	
	return PyObject_CallFunction(sFontType, "isiii", family, (const char *)face.toUtf8(), size, style, spacing);
}


PyObject *
createDCObject(QPainter *painter, PyObject *objectType, PyObject *proxyType, QPaintDevice *device)
{
	if (!objectType)
		objectType = PyDC_Type;
	
	PyObject *object = PyObject_CallFunctionObjArgs(objectType, NULL);
	if (!object)
		return NULL;
	
	if (!proxyType)
		proxyType = (PyObject *)&DC_Type;
	DC_Proxy *proxy = PyObject_New(DC_Proxy, (PyTypeObject *)proxyType);
	if (!proxy) {
		Py_DECREF(object);
		return NULL;
	}
	
	PyObject_SetAttrString(object, "_impl", (PyObject *)proxy);
	proxy->fDevice = device;
	proxy->fPainter = painter;
	Py_DECREF(proxy);
	
	return object;
}


PyObject *
createBitmapObject(const QPixmap& pixmap)
{
	if (pixmap.isNull())
		Py_RETURN_NONE;
	
	PyObject *object = PyObject_CallFunctionObjArgs(sBitmapType, NULL);
	if (!object)
		return NULL;
	
	DC_Proxy *proxy = (DC_Proxy *)PyObject_GetAttrString(object, "_impl");
	proxy->fPainter->end();
	*((QPixmap *)proxy->fDevice) = pixmap;
	proxy->fPainter->begin(proxy->fDevice);
	Py_DECREF(proxy);
	
	return object;
}


PyObject *
createIconObject(const QIcon& icon)
{
	PyObject *normal, *disabled, *active, *selected;
	QList<QSize> sizes;
	
	if (icon.isNull())
		Py_RETURN_NONE;
	
	sizes = icon.availableSizes(QIcon::Normal);
	if (!sizes.isEmpty())
		normal = createBitmapObject(icon.pixmap(sizes.first(), QIcon::Normal));
	else {
		normal = Py_None;
		Py_INCREF(Py_None);
	}
	
	sizes = icon.availableSizes(QIcon::Disabled);
	if (!sizes.isEmpty())
		disabled = createBitmapObject(icon.pixmap(sizes.first(), QIcon::Disabled));
	else {
		disabled = Py_None;
		Py_INCREF(Py_None);
	}
	
	sizes = icon.availableSizes(QIcon::Active);
	if (!sizes.isEmpty())
		active = createBitmapObject(icon.pixmap(sizes.first(), QIcon::Active));
	else {
		active = Py_None;
		Py_INCREF(Py_None);
	}
	
	sizes = icon.availableSizes(QIcon::Selected);
	if (!sizes.isEmpty())
		selected = createBitmapObject(icon.pixmap(sizes.first(), QIcon::Selected));
	else {
		selected = Py_None;
		Py_INCREF(Py_None);
	}
	
	PyObject *object = PyObject_CallFunctionObjArgs(sIconType, normal, disabled, active, selected, NULL);
	
	Py_DECREF(normal);
	Py_DECREF(disabled);
	Py_DECREF(active);
	Py_DECREF(selected);
	
	if (!object)
		return NULL;
	return object;
}


PyObject *
createDateObject(const QDate& date)
{
	return PyDate_FromDate(date.year(), date.month(), date.day());
}


PyObject *
createDateTimeObject(const QDateTime& ts)
{
	return PyDateTime_FromDateAndTime(ts.date().year(), ts.date().month(), ts.date().day(), ts.time().hour(), ts.time().minute(), ts.time().second(), ts.time().msec() * 1000);
}


QString
normalizeFormat(const QHash<QString, QString>& vars, const QString& format)
{
	QString result = format, var;
	QHash<QString, QString>::const_iterator it;
	bool retry = true;
	
	while (retry) {
		retry = false;
		for (it = vars.constBegin(); it != vars.constEnd(); it++) {
			var = QString("[%1]").arg(it.key());
			if (result.indexOf(var) >= 0) {
				result.replace(var, it.value());
				retry = true;
			}
		}
	}
	if (result != format)
		return normalizeFormat(vars, result);
	return result;
}


void
centerWindow(QWidget *window, QWidget *parent)
{
	QRect drect;
	QPoint diff = window->geometry().topLeft() - window->pos();
	QSize size = window->sizeHint();
	QPoint pos;
	
	if (!parent)
		parent = window->parentWidget();
	if (parent)
		drect = parent->window()->geometry();
	else
		drect = QApplication::desktop()->availableGeometry();
	
	pos.setX(drect.left() + ((drect.width() - size.width()) / 2) - diff.x());
	pos.setY(drect.top() + ((drect.height() - size.height()) / 2) - diff.y());
	window->move(pos);
}


void
setShortcut(QWidget *widget, const QString& sequence, Qt::ShortcutContext context, PyObject *callback)
{
	QKeySequence key(sequence);
	if (!key.isEmpty()) {
		foreach (QObject *child, widget->children()) {
			Shortcut *shortcut = qobject_cast<Shortcut *>(child);
			if ((shortcut) && (shortcut->key() == key))
				delete shortcut;
		}
		if ((callback) && (callback != Py_None)) {
			new Shortcut(widget, key, context, callback);
		}
	}
}


void
setTimeout(QObject *object, int delay, PyObject *func, PyObject *args)
{
	PyAutoLocker locker;
	TimedCall *timedCall;
	
	if (object) {
		int id = qvariant_cast<int>(object->property("old_timed_call_id"));
		if (id) {
			object->killTimer(id);
			delete sTimers.take(id);
		}
	}
	
	timedCall = new TimedCall(object, func, args);
	timedCall->moveToThread(QApplication::instance()->thread());
	timedCall->setParent(object);
	if (object)
		QObject::connect(object, SIGNAL(destroyed()), timedCall, SLOT(handleDestroyed()));
	if (delay == 0) {
		QMetaObject::invokeMethod(timedCall, "executeAndDelete", Qt::QueuedConnection);
	}
	else {
		QMetaObject::invokeMethod(qApp, "startTimedCall", Qt::AutoConnection, Q_ARG(QObject *, object), Q_ARG(QObject *, timedCall), Q_ARG(int, delay));
	}
}


bool
loadResource(const QString& resource, QByteArray& data)
{
	if (!sResourceReader)
		return false;
	return sResourceReader->read(resource, data);
}


bool
openURI(const QString& uri)
{
	QUrl url(uri, QUrl::TolerantMode);
	bool wasRelative = false;
	if ((url.isRelative()) || (QFile::exists(uri))) {
		url.setScheme("file");
		wasRelative = true;
	}
	if ((url.scheme() == "file") && (wasRelative) && (!QFile::exists(url.toLocalFile()))) {
		if (!uri.startsWith("http://"))
			url = QUrl("http://" + uri);
		else
			url.setScheme("http");
	}
	
	if (!QDesktopServices::openUrl(url)) {
		QString text = url.errorString();
		if (text.isEmpty())
			text = url.toString();
		PyErr_Format(PyExc_RuntimeError, "cannot open URI: %s", (const char *)text.toUtf8());
		return false;
	}
	return true;
}


void
grabMouse(QWidget *window, bool grab)
{
	if (grab) {
		window->grabMouse();
		sMouseGrabber = window;
	}
	else {
		window->releaseMouse();
		sMouseGrabber = NULL;
	}
}


static int
encodeButtons(int buttons)
{
	int qbuttons = QMessageBox::NoButton;
	
	if (buttons & SL_BUTTON_OK)
		qbuttons |= QMessageBox::Ok;
	if (buttons & SL_BUTTON_YES)
		qbuttons |= QMessageBox::Yes;
	if (buttons & SL_BUTTON_YES_ALL)
		qbuttons |= QMessageBox::YesToAll;
	if (buttons & SL_BUTTON_NO)
		qbuttons |= QMessageBox::No;
	if (buttons & SL_BUTTON_NO_ALL)
		qbuttons |= QMessageBox::NoToAll;
	if (buttons & SL_BUTTON_CANCEL)
		qbuttons |= QMessageBox::Cancel;
	if (buttons & SL_BUTTON_OPEN)
		qbuttons |= QMessageBox::Open;
	if (buttons & SL_BUTTON_SAVE)
		qbuttons |= QMessageBox::Save;
	if (buttons & SL_BUTTON_SAVE_ALL)
		qbuttons |= QMessageBox::SaveAll;
	if (buttons & SL_BUTTON_CLOSE)
		qbuttons |= QMessageBox::Close;
	if (buttons & SL_BUTTON_DISCARD)
		qbuttons |= QMessageBox::Discard;
	if (buttons & SL_BUTTON_APPLY)
		qbuttons |= QMessageBox::Apply;
	if (buttons & SL_BUTTON_RESET)
		qbuttons |= QMessageBox::Reset;
	if (buttons & SL_BUTTON_ABORT)
		qbuttons |= QMessageBox::Abort;
	if (buttons & SL_BUTTON_RETRY)
		qbuttons |= QMessageBox::Retry;
	if (buttons & SL_BUTTON_IGNORE)
		qbuttons |= QMessageBox::Ignore;
	
	return qbuttons;
}


static int
decodeButton(int qbutton)
{
	int button;
	switch (qbutton) {
	case QMessageBox::Yes:		button = SL_BUTTON_YES; break;
	case QMessageBox::YesToAll:	button = SL_BUTTON_YES_ALL; break;
	case QMessageBox::No:		button = SL_BUTTON_NO; break;
	case QMessageBox::NoToAll:	button = SL_BUTTON_NO_ALL; break;
	case QMessageBox::Cancel:	button = SL_BUTTON_CANCEL; break;
	case QMessageBox::Open:		button = SL_BUTTON_OPEN; break;
	case QMessageBox::Save:		button = SL_BUTTON_SAVE; break;
	case QMessageBox::SaveAll:	button = SL_BUTTON_SAVE_ALL; break;
	case QMessageBox::Close:	button = SL_BUTTON_CLOSE; break;
	case QMessageBox::Discard:	button = SL_BUTTON_DISCARD; break;
	case QMessageBox::Apply:	button = SL_BUTTON_APPLY; break;
	case QMessageBox::Reset:	button = SL_BUTTON_RESET; break;
	case QMessageBox::Abort:	button = SL_BUTTON_ABORT; break;
	case QMessageBox::Retry:	button = SL_BUTTON_RETRY; break;
	case QMessageBox::Ignore:	button = SL_BUTTON_IGNORE; break;
	case QMessageBox::Ok:
	default:					button = SL_BUTTON_OK; break;
	}
	return button;
}


bool
messageBox(QWidget *window, const QString& title, const QString& message, int buttons, int icon, PyObject *callback, PyObject *userdata, int *button)
{
	QMessageBox::StandardButtons qbuttons = (QMessageBox::StandardButtons)encodeButtons(buttons);
	QMessageBox::Icon qicon = QMessageBox::NoIcon;
	QMessageBox mb(window);
	int result;
	
	switch (icon) {
	case SL_ICON_ERROR:		qicon = QMessageBox::Critical; break;
	case SL_ICON_QUESTION:	qicon = QMessageBox::Question; break;
	case SL_ICON_WARNING:	qicon = QMessageBox::Warning; break;
	default:				qicon = QMessageBox::Information; break;
	}
	
	if (window)
		mb.setWindowModality(Qt::WindowModal);
	else
		mb.setWindowModality(Qt::ApplicationModal);
	mb.setWindowTitle(qApp->applicationName());
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
	int diff = qMax(0, 40 - title.size());
	QString _title;
	if (diff > 0)
#ifdef Q_OS_LINUX
		_title = title + QString("&nbsp;&nbsp;").repeated(diff);
#else
		_title = title + QString(' ').repeated(diff);
#endif
	else
		_title = title;
#ifdef Q_OS_LINUX
	_title = QString("<b>%1</b>").arg(_title);
#endif
	mb.setText(_title);
#else
	mb.setText(title);
#endif
	mb.setInformativeText(message);
	mb.setStandardButtons(qbuttons);
	mb.setIcon(qicon);
	
	Py_BEGIN_ALLOW_THREADS
	
	result = mb.exec();
	
	Py_END_ALLOW_THREADS
	
	*button = decodeButton(result);
	
	if ((callback) && (PyCallable_Check(callback))) {
		PyObject *id = PyInt_FromLong(*button);
		if (!userdata)
			userdata = Py_None;
		Py_INCREF(userdata);
		
		PyObject *result = PyObject_CallFunctionObjArgs(callback, id, userdata, NULL);
		Py_DECREF(id);
		Py_DECREF(userdata);
		if (!result) {
			PyErr_Print();
			PyErr_Clear();
			return false;
		}
		Py_DECREF(result);
	}
	return true;
}


int
showPopupMessage(QWidget *parent, QWidget *editor, const QString& text, const QPoint& hotspot, int where, int buttons)
{
	InfoBalloon *b = new InfoBalloon(parent, editor, text, hotspot, where, buttons);
	return b->exec();
}


void hidePopupMessage(QEvent *event)
{
	if ((sModalBalloon) && (event) && (event->type() == QEvent::Move)) {
		sInfoBalloon->doLayout();
	}
	else {
		InfoBalloon::fade();
	}
}


bool isPopupMessageShown()
{
	return sInfoBalloon != NULL;
}


PyObject *
mimeDataToObject(const QMimeData *mimeData, const QString& mimeType)
{
	PyObject *object = NULL;
	
	if (mimeType.isEmpty()) {
		if (mimeData->hasUrls()) {
			QList<QUrl> urls = mimeData->urls();
			object = PyTuple_New(0);
			for (int i = 0; i < urls.count(); i++) {
				QUrl url = urls.at(i);
				if (url.scheme() == "file") {
					Py_ssize_t count = PyTuple_GET_SIZE(object);
					_PyTuple_Resize(&object, count + 1);
					PyTuple_SET_ITEM(object, count, createStringObject(url.toLocalFile()));
				}
			}
			if (PyTuple_GET_SIZE(object) == 0) {
				Py_DECREF(object);
				object = NULL;
			}
		}
		else if (mimeData->hasText()) {
			object = createStringObject(mimeData->text());
		}
		else if (mimeData->hasColor()) {
			QColor color = qvariant_cast<QColor>(mimeData->colorData());
			object = createColorObject(color);
		}
		else if (mimeData->hasImage()) {
			QImage image = qvariant_cast<QImage>(mimeData->imageData());
			object = createBitmapObject(QPixmap::fromImage(image));
		}
		else if (mimeData->hasFormat(SL_PYOBJECT_MIME_TYPE)) {
			QByteArray array = mimeData->data(SL_PYOBJECT_MIME_TYPE);
			PyObject *data = PyString_FromStringAndSize(array.data(), array.size());
			object = PyObject_CallFunctionObjArgs(sUnserializeData, data, NULL);
			Py_DECREF(data);
			if (!object) {
				PyErr_Print();
				PyErr_Clear();
			}
		}
	}
	else {
		QString _mimeType = mimeType;
		if (_mimeType == "image/*") {
			QImage image = qvariant_cast<QImage>(mimeData->imageData());
			object = createBitmapObject(QPixmap::fromImage(image));
		}
		else if (mimeData->formats().contains(_mimeType)) {
			QByteArray array = mimeData->data(_mimeType);
			object = createBufferObject(array);
		}
	}
	return object;
}


bool
objectToMimeData(PyObject *object, QMimeData *mimeData)
{
	QString text;
	QColor color;
	QPixmap pixmap;
	
	if (convertString(object, &text)) {
		mimeData->setText(text);
	}
	else if (convertColor(object, &color)) {
		mimeData->setColorData(color);
	}
	else if (convertPixmap(object, &pixmap)) {
		mimeData->setImageData(pixmap);
	}
	else {
		PyErr_Clear();
		QByteArray data;
		PyObject *buffer;
		buffer = PyObject_CallFunctionObjArgs(sSerializeData, object, NULL);
		if (buffer) {
			if (convertBuffer(buffer, &data)) {
				mimeData->setData(SL_PYOBJECT_MIME_TYPE, data);
			}
			Py_DECREF(buffer);
		}
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
			return false;
		}
	}
	PyErr_Clear();
	return true;
}


void
relinkActions(QWidget *widget)
{
	if ((!widget) || (!getProxy(widget)))
		return;
	
	QList<QActionGroup *> *groupStorage = qvariant_cast<QList<QActionGroup *> *>(widget->property("action_groups"));
	QList<QAction *> list = widget->actions();
	QActionGroup *group;
	QAction *action;
	int type;
	
	if (!groupStorage)
		return;
	
	for (QList<QAction *>::iterator it = list.begin(); it != list.end(); it++) {
		action = *it;
		group = action->actionGroup();
		if (group)
			group->removeAction(action);
	}
	for (QList<QActionGroup *>::iterator it = groupStorage->begin(); it != groupStorage->end(); it++) {
		group = *it;
		delete group;
	}
	groupStorage->clear();
	group = NULL;
	for (QList<QAction *>::iterator it = list.begin(); it != list.end(); it++) {
		action = *it;
		if (!getProxy(action)) {
			group = NULL;
			continue;
		}
		type = qvariant_cast<int>(action->property("type"));
		action->setCheckable((type == SL_MENU_ITEM_TYPE_CHECK) || (type == SL_MENU_ITEM_TYPE_RADIO));
		if (type == SL_MENU_ITEM_TYPE_RADIO) {
			if (!group) {
				group = new QActionGroup(widget);
				group->setExclusive(true);
				groupStorage->append(group);
			}
			action->setActionGroup(group);
		}
		else {
			group = NULL;
		}
		QObject::disconnect(action, 0, action, 0);
		QObject::connect(action, SIGNAL(triggered()), action, SLOT(handleActionTriggered()));
	}
}


int
getKeyModifiers(Qt::KeyboardModifiers modifiers)
{
	int result = 0;
	if (modifiers & Qt::ShiftModifier)
		result |= SL_MODIFIER_SHIFT;
	if (modifiers & Qt::AltModifier)
		result |= SL_MODIFIER_ALT;
	if (modifiers & Qt::ControlModifier)
		result |= SL_MODIFIER_CONTROL;
	if (modifiers & Qt::MetaModifier)
		result |= SL_MODIFIER_META;
	return result;
}


int
getKeyCode(int key)
{
	switch (key) {
	case Qt::Key_Shift:		return SL_KEY_SHIFT;
	case Qt::Key_Alt:		return SL_KEY_ALT;
	case Qt::Key_Control:	return SL_KEY_CONTROL;
	case Qt::Key_Meta:		return SL_KEY_META;
	case Qt::Key_Home:		return SL_KEY_HOME;
	case Qt::Key_End:		return SL_KEY_END;
	case Qt::Key_Insert:	return SL_KEY_INSERT;
	case Qt::Key_Delete:	return SL_KEY_DELETE;
	case Qt::Key_PageUp:	return SL_KEY_PAGE_UP;
	case Qt::Key_PageDown:	return SL_KEY_PAGE_DOWN;
	case Qt::Key_Left:		return SL_KEY_LEFT;
	case Qt::Key_Right:		return SL_KEY_RIGHT;
	case Qt::Key_Up:		return SL_KEY_UP;
	case Qt::Key_Down:		return SL_KEY_DOWN;
	case Qt::Key_F1:		return SL_KEY_F1;
	case Qt::Key_F2:		return SL_KEY_F2;
	case Qt::Key_F3:		return SL_KEY_F3;
	case Qt::Key_F4:		return SL_KEY_F4;
	case Qt::Key_F5:		return SL_KEY_F5;
	case Qt::Key_F6:		return SL_KEY_F6;
	case Qt::Key_F7:		return SL_KEY_F7;
	case Qt::Key_F8:		return SL_KEY_F8;
	case Qt::Key_F9:		return SL_KEY_F9;
	case Qt::Key_F10:		return SL_KEY_F10;
	case Qt::Key_Backspace:	return SL_KEY_BACKSPACE;
	case Qt::Key_Enter:
	case Qt::Key_Return:	return SL_KEY_RETURN;
	case Qt::Key_Backtab:
	case Qt::Key_Tab:		return SL_KEY_TAB;
	case Qt::Key_Escape:	return SL_KEY_ESCAPE;
	}
	return key < 256 ? key : 0;
}


QLocale
getLocale()
{
	return sLocale;
}


void
freeBitmapResources(QPainter *painter, QPaintDevice *device)
{
	QApplication::postEvent(qApp, new FreeBitmapResourcesEvent(painter, device));
}


Application::Application(int& argc, char **argv)
	: QApplication(argc, argv), fWID(0)
{
	fMutex = new QMutex(QMutex::Recursive);
	fShadowWindow = new QMainWindow;
	fShadowWindow->setAttribute(Qt::WA_DontShowOnScreen);
	fShadowWindow->setAttribute(Qt::WA_QuitOnClose, false);
	fShadowWindow->move(10000, 10000);
	fShadowWindow->show();
	
// 	qRegisterMetaType<SL_Widget *>();
// 	qRegisterMetaType<SL_DataItem *>();
// 	qRegisterMetaType<SL_DataIndex *>();
	
	sPyObjectMetaType = qRegisterMetaType<PyObject *>();
	qRegisterMetaType<QModelIndex>();
	qRegisterMetaType<QItemSelection>();
	qRegisterMetaType<Qt::SortOrder>();
	qRegisterMetaType<Qt::Alignment>();
	qRegisterMetaType<QMargins>();
	qRegisterMetaType<QObject *>();
	qRegisterMetaType<QEvent *>();
	qRegisterMetaType<QList<QActionGroup *> *>();
	
	sExceptionEvent = (QEvent::Type)QEvent::registerEventType();
	sFreeBitmapResourcesEvent = (QEvent::Type)QEvent::registerEventType();
	
	installEventFilter(this);
	QNetworkProxyFactory::setUseSystemConfiguration(true);
}


Application::~Application()
{
	delete fShadowWindow;
	delete fMutex;
}


bool
Application::notify(QObject *receiver, QEvent *event)
{
	QPointer<QObject> original = receiver;
	
	QWidget *target = qobject_cast<QWidget *>(receiver);
	if (target) {
		if ((sInfoBalloon) && (sInfoBalloon->isAncestorOf(target)))
			return QApplication::notify(original, event);
	
		while (target->focusProxy())
			target = target->focusProxy();
		QWidget *current = target;
		WidgetInterface *impl;
// 		qDebug() << "impl is" << impl << receiver;
		do {
			impl = dynamic_cast<WidgetInterface *>(current);
			if (!impl)
				current = current->parentWidget();
		} while ((!impl) && (current));
		
// 		qDebug() << "---" << current << event;
		if (impl) {
			if ((impl->isModifyEvent(event)) && (!impl->canModify(current)))
				return true;
			if (!original)
				return true;
		}
		
		QWidget *widget = focusWidget();
		while ((original) && (widget)) {
			if (widget->isEnabled()) {
				QWidget *oldFocus = widget;
				impl = dynamic_cast<WidgetInterface *>(widget);
				if (!impl) {
					QWidget *proxy = widget;
					while (proxy->focusProxy())
						proxy = proxy->focusProxy();
					oldFocus = proxy;
					impl = dynamic_cast<WidgetInterface *>(proxy);
				}
				if ((impl) && (impl->isFocusOutEvent(event))) {
					if (Completer::eatFocus())
						break;
					
// 					qDebug() << "--- focus out event" << event << oldFocus << target << target->focusPolicy();
					switch (event->type()) {
					case QEvent::KeyPress:
						{
							QKeyEvent *e = (QKeyEvent *)event;
// 							if (((e->key() == Qt::Key_Tab) || (e->key() == Qt::Key_Backtab)) &&
// 									((target->focusPolicy() & oldFocus->focusPolicy() & Qt::TabFocus) == Qt::TabFocus)) {
// 							if ((target->focusPolicy() & Qt::TabFocus) == Qt::TabFocus) {
							if (e->key() == Qt::Key_Tab)
								target = oldFocus->nextInFocusChain();
							else if (e->key() == Qt::Key_Backtab)
								target = oldFocus->previousInFocusChain();
							if ((!sModalBalloon) && (!impl->canFocusOut(oldFocus, target)))
								return true;
							widget = NULL;
						}
						break;
						
					case QEvent::MouseButtonPress:
					case QEvent::MouseButtonDblClick:
					case QEvent::TouchBegin:
						{
							if (Completer::isRunningOn(oldFocus)) {
								if (!Completer::underMouse()) {
									Completer::hide();
								}
								widget = NULL;
								break;
							}
//  							if (target->focusPolicy() != Qt::NoFocus) {
								if (!impl->canFocusOut(oldFocus, target))
									return true;
								widget = NULL;
// 							}
						}
						break;
						
					case QEvent::Wheel:
						{
							if (target->focusPolicy() != Qt::NoFocus) {
								if (!impl->canFocusOut(oldFocus, target))
									return true;
								widget = NULL;
							}
						}
						break;
					
					default:
						break;
					}
				}
			}
			if (widget) {
				if (widget->isWindow())
					break;
				widget = widget->parentWidget();
			}
		}
	}
	
	if (original)
		return QApplication::notify(original, event);
	
	return true;
}


bool
Application::eventFilter(QObject *obj, QEvent *event)
{
	static QPoint dragStartPos;
	static bool dragTested = false;
	
// 	if (event->type() != QEvent::Timer)
// 		qDebug() << obj << obj->objectName() << event;
	
	if (event->type() == sExceptionEvent) {
		ExceptionEvent *e = (ExceptionEvent *)event;
		e->report();
		return true;
	}
	else if (event->type() == sFreeBitmapResourcesEvent) {
		FreeBitmapResourcesEvent *e = (FreeBitmapResourcesEvent *)event;
		e->deleteResources();
		return true;
	}
	
	switch ((int)event->type()) {
	
	case QEvent::Timer:
		{
			PyAutoLocker locker;
			QTimerEvent *e = (QTimerEvent *)event;
			TimedCall *timedCall = sTimers.take(e->timerId());
			if (timedCall) {
				timedCall->execute();
				QObject *parent = timedCall->parent();
				if (!parent)
					parent = this;
				parent->killTimer(e->timerId());
				delete timedCall;
			}
		}
		break;
	
	case QEvent::Paint:
		{
			QWidget *w = qobject_cast<QWidget *>(obj);
			if ((w) && (getSafeProxy(w)) && (!qobject_cast<SceneView_Impl *>(obj->parent()))) {
				EventRunner runner(obj, "onPaint");
				if (runner.isValid()) {
					QScrollArea *area = qobject_cast<QScrollArea *>(runner.widget());
					if ((area) && (area->widget() != obj)) {
						break;
					}
					else {
						QAbstractScrollArea *area = qobject_cast<QAbstractScrollArea *>(runner.widget());
						if ((area) && (area->viewport() != obj))
							break;
					}
					QPainter painter(w);
					painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::NonCosmeticDefaultPen);
	// 				painter.setPen(QPen(QColor(rand() % 256, rand() % 256, rand() % 256)));
	// 				painter.drawRect(0, 0, w->width() - 1, w->height() - 1);
					runner.set("dc", createDCObject(&painter));
					runner.run();
				}
			}
		}
		break;
		
	case QEvent::WindowActivate:
		{
			if (qobject_cast<Frame_Impl *>(obj) || qobject_cast<Dialog_Impl *>(obj)) {
				EventRunner(obj, "onFocusIn").run();
			}
		}
		break;
		
	case QEvent::WindowDeactivate:
		{
			if (qobject_cast<Frame_Impl *>(obj) || qobject_cast<Dialog_Impl *>(obj)) {
				EventRunner(obj, "onFocusOut").run();
			}
		}
		break;
		
	case QEvent::FocusIn:
		{
			QWidget *w = qobject_cast<QWidget *>(obj);
			QWidget *ww = w;
			while ((ww) && (!qobject_cast<FoldPanel_Impl *>(ww)))
				ww = ww->parentWidget();
			if (ww)
				ww->update();
			
			if ((w) && (w->isWindow()))
				break;
			
// 			QFocusEvent *e = (QFocusEvent *)event;
// 			if ((e->reason() == Qt::ActiveWindowFocusReason) || (e->reason() == Qt::PopupFocusReason))
// 				return false;
			
			if ((!(qobject_cast<Frame_Impl *>(obj) || qobject_cast<Dialog_Impl *>(obj))) && (getSafeProxy(obj))) {
				EventRunner(obj, "onFocusIn").run();
			}
		}
		break;
	
	case QEvent::FocusOut:
		{
			QWidget *w = qobject_cast<QWidget *>(obj);
			while ((w) && (!qobject_cast<FoldPanel_Impl *>(w)))
				w = w->parentWidget();
			if (w)
				w->update();
		}
		break;
		
	case QEvent::MouseMove:
		{
			QMouseEvent *e = (QMouseEvent *)event;
			Widget_Proxy *proxy;
			if ((!dragTested) && (e->buttons() & Qt::LeftButton) && ((e->globalPos() - dragStartPos).manhattanLength() > QApplication::startDragDistance()) && (proxy = getSafeProxy(obj))) {
				dragTested = true;
				QAbstractItemView *view = qobject_cast<QAbstractItemView *>(proxy->fImpl);
				PyObject *selection;
				EventRunner runner(obj, "onDragStart");
				if (!runner.isValid())
					return false;
				runner.set("pos", e->globalPos());
				runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
				runner.set("data", Py_None, false);
				runner.set("action", SL_EVENT_DRAG_ACTION_COPY | SL_EVENT_DRAG_ACTION_CUSTOMIZABLE);
				if (view) {
					selection = getViewSelection(view);
				}
				else {
					selection = Py_None;
					Py_INCREF(selection);
				}
				runner.set("selection", selection, false);
				runner.set("bitmap", Py_None, false);
				runner.set("hotspot", Py_None, false);
				
				bool handled = false;
				do {
					if ((view) && (!QRect(view->viewport()->mapToGlobal(QPoint(0,0)), view->viewport()->size()).contains(e->globalPos())))
						break;
					
					QAbstractScrollArea *area = qobject_cast<QAbstractScrollArea *>(proxy->fImpl);
					if ((area) && (!QRect(area->viewport()->mapToGlobal(QPoint(0,0)), area->viewport()->size()).contains(e->globalPos())))
						break;
					
					if (!runner.run())
						break;
					
					PyObject *data;
					if (!runner.get("data", &data))
						break;
					
					PyObject *pixmapObject;
					QPixmap pixmap;
					QPoint hotspot;
					int flags;
					Qt::DropActions supportedActions;
					Qt::DropAction action;
					
					if ((!runner.get("bitmap", &pixmapObject)) || (!runner.get("hotspot", &hotspot)) || (!runner.get("action", &flags)))
						break;
					
					if ((pixmapObject != Py_None) && (!convertPixmap(pixmapObject, &pixmap)))
						break;
					
					QMimeData *mimeData = new QMimeData;
					if (!objectToMimeData(data, mimeData)) {
						delete mimeData;
						break;
					}
					QDrag *drag = new QDrag((QWidget *)obj);

					if ((pixmap.isNull()) && (view)) {
						QModelIndexList indexes = view->selectionModel()->selectedIndexes();
						QRect rect;
						QList<QRect> rects;
						int i;
						for (i = 0; i < indexes.count(); i++) {
							rects.append(view->visualRect(indexes.at(i)));
							rect |= rects.at(i);
						}
						if (!rect.isEmpty()) {
							QImage image(rect.size(), QImage::Format_ARGB32);
							image.fill(0);
							{
								QStyleOptionViewItem option;
								if (dynamic_cast<WidgetInterface *>(view)) {
									QMetaObject::invokeMethod(view, "initOptionView", Qt::DirectConnection, Q_RETURN_ARG(QStyleOptionViewItem, option));
								}
								else {
									option.initFrom(view);
									option.font = view->font();
								}
								option.state |= QStyle::State_Selected;
								QPainter painter(&image);
								for (i = 0; i < indexes.count(); i++) {
									option.rect = QRect(rects.at(i).topLeft() - rect.topLeft(), rects.at(i).size());
									view->itemDelegate(indexes.at(i))->paint(&painter, option, indexes.at(i));
								}
							}
							for (int y = 0; y < rect.height(); y++) {
								QRgb *pixel = (QRgb *)image.scanLine(y);
								for (int x = 0; x < rect.width(); x++) {
									uint rgb = *pixel;
									*pixel++ = (rgb & 0x00FFFFFF) | ((rgb >> 1) & 0xFF000000);
								}
							}
							pixmap = QPixmap::fromImage(image);
							hotspot = view->viewport()->mapFromGlobal(QCursor::pos()) - rect.topLeft();
						}
					}
					
					drag->setMimeData(mimeData);
					drag->setPixmap(pixmap);
					drag->setHotSpot(hotspot);
					
					if (flags & SL_EVENT_DRAG_ACTION_MOVE)
						action = Qt::MoveAction;
					else if (flags & SL_EVENT_DRAG_ACTION_COPY)
						action = Qt::CopyAction;
					else
						action = Qt::IgnoreAction;
					if (flags & SL_EVENT_DRAG_ACTION_CUSTOMIZABLE)
						supportedActions = Qt::CopyAction | Qt::MoveAction;
					else
						supportedActions = action;
					
					Py_BEGIN_ALLOW_THREADS
					
// 					flush();
					action = drag->exec(supportedActions, action);
					
					Py_END_ALLOW_THREADS
					
					{
						if (action == Qt::MoveAction)
							flags = SL_EVENT_DRAG_ACTION_MOVE;
						else if (action == Qt::CopyAction)
							flags = SL_EVENT_DRAG_ACTION_COPY;
						else
							flags = SL_EVENT_DRAG_ACTION_NONE;
						
						EventRunner runner(obj, "onDragEnd");
						if (runner.isValid()) {
							runner.set("pos", QCursor::pos());
							runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
							runner.set("data", data, false);
							runner.set("selection", selection, false);
							runner.set("action", flags);
							runner.run();
						}
					}
					handled = true;
				} while (0);
				
				if (PyErr_Occurred()) {
					PyErr_Print();
					PyErr_Clear();
				}
				Py_DECREF(selection);
				if (handled)
					return true;
			}
		}
		/* fallthrough */
		
	case QEvent::Wheel:
	case QEvent::MouseButtonDblClick:
	case QEvent::MouseButtonPress:
		{
			if (event->type() != QEvent::MouseMove) {
				hidePopupMessage();
				if (event->type() == QEvent::MouseButtonPress) {
					QMouseEvent *e = (QMouseEvent *)event;
					dragStartPos = e->globalPos();
					dragTested = false;
				}
			}
		}
		/* fallthrough */
		
	case QEvent::MouseButtonRelease:
		{
			if (event->type() == QEvent::MouseButtonRelease) {
				dragStartPos = QPoint();
			}
			EventRunner runner(obj);
			if (runner.isValid()) {
				QAbstractScrollArea *area = qobject_cast<QAbstractScrollArea *>(runner.widget());
				if (area) {
					if (((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonDblClick)) && (!area->viewport()->rect().contains(area->viewport()->mapFromGlobal(QCursor::pos()))))
						return false;
					if (area->verticalScrollBar() && area->verticalScrollBar()->isVisible() && (area->verticalScrollBar()->rect().contains(area->verticalScrollBar()->mapFromGlobal(QCursor::pos()))))
						return false;
					if (area->horizontalScrollBar() && area->horizontalScrollBar()->isVisible() && (area->horizontalScrollBar()->rect().contains(area->horizontalScrollBar()->mapFromGlobal(QCursor::pos()))))
						return false;
				}
				if (qobject_cast<SceneView_Impl *>(obj))
					return false;
				
				switch ((int)event->type()) {
// 				case QEvent::Enter:					runner.setName("onMouseEnter"); break;
// 				case QEvent::Leave:					runner.setName("onMouseLeave"); break;
				case QEvent::MouseButtonDblClick:
					{
						QMouseEvent *e = (QMouseEvent *)event;
						runner.setName("onDblClick");
						runner.set("down", (int)e->button());
					}
					break;
				case QEvent::MouseButtonPress:
					{
						QMouseEvent *e = (QMouseEvent *)event;
						runner.setName("onMouseDown");
						runner.set("down", (int)e->button());
					}
					break;
				case QEvent::MouseButtonRelease:
					{
						QMouseEvent *e = (QMouseEvent *)event;
						runner.setName("onMouseUp");
						runner.set("up", (int)e->button());
					}
					break;
				case QEvent::MouseMove:
					{
						runner.setName("onMouseMove");
					}
					break;
				case QEvent::Wheel:
					{
						QWheelEvent *e = (QWheelEvent *)event;
						runner.setName("onMouseWheel");
						runner.set("delta", e->delta() / 100);
					}
					break;
				}
				
				runner.set("pos", QCursor::pos());
				runner.set("buttons", (int)QApplication::mouseButtons());
				runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
				
				return !runner.run();
			}
		}
		break;
	
	case QEvent::DragEnter:
	case QEvent::DragMove:
		{
			QDragMoveEvent *e = (QDragMoveEvent *)event;
			QAbstractItemView *view = qobject_cast<QAbstractItemView *>(obj->parent());
			EventRunner runner(obj);
			if (runner.isValid()) {
				if (event->type() == QEvent::DragEnter)
					runner.setName("onDragEnter");
				else
					runner.setName("onDragMove");
				
				QModelIndex hover;
				PyObject *index = Py_None;
				int action, where = SL_EVENT_DRAG_ON_ITEM;
				
				e->ignore();
				if (view) {
					DataModel_Impl *model = qobject_cast<DataModel_Impl *>(view->model());
					if (model) {
						hover = view->indexAt(view->viewport()->mapFromParent(e->pos()));
						if (dynamic_cast<WidgetInterface *>(view))
							QMetaObject::invokeMethod(view, "prepareDrag", Qt::DirectConnection);
						
						if ((hover.isValid()) && (!(hover.flags() & Qt::ItemIsDropEnabled)))
							break;
						
						QModelIndexList dragging = view->selectionModel()->selectedIndexes();
						if (hover.isValid()) {
							QModelIndex child = hover;
							while (child.isValid()) {
								if ((dragging.contains(child)) && ((view == e->source()) || (view->isAncestorOf((QWidget *)e->source())))) {
									break;
								}
								child = child.parent();
							}
							if (child.isValid()) {
								if (child == hover) {
									e->setDropAction(Qt::MoveAction);
									e->accept();
								}
								view->setProperty("dragHover", QVariant::fromValue(hover));
								return true;
							}
							
							QRect rect = view->visualRect(hover);
							QPoint pos = e->pos();
							
							QListView *iconView = qobject_cast<QListView *>(view);
							if ((iconView) && (iconView->viewMode() == QListView::IconMode)) {
								int margin = qMax(4, rect.width() / 4);
								if (pos.x() - rect.left() < margin)
									where = SL_EVENT_DRAG_ABOVE_ITEM;
								else if (rect.right() - pos.x() < margin)
									where = SL_EVENT_DRAG_BELOW_ITEM;
								else
									where = SL_EVENT_DRAG_ON_ITEM;
							}
							else {
								if ((view->selectionBehavior() == QAbstractItemView::SelectRows) || (model->columnCount() == 1)) {
									if (pos.y() - rect.top() < 4)
										where = SL_EVENT_DRAG_ABOVE_ITEM;
									else if (rect.bottom() - pos.y() < 4)
										where = SL_EVENT_DRAG_BELOW_ITEM;
									else
										where = SL_EVENT_DRAG_ON_ITEM;
								}
								else {
									where = SL_EVENT_DRAG_ON_ITEM;
								}
							}
							QTreeView *treeView = qobject_cast<QTreeView *>(view);
							if ((treeView) && (treeView->isExpanded(hover)) && (where == SL_EVENT_DRAG_BELOW_ITEM))
								where = SL_EVENT_DRAG_ON_ITEM;
						}
						else {
							where = SL_EVENT_DRAG_ON_VIEWPORT;
						}
						index = model->getDataIndex(hover);
					}
				}
				runner.set("index", index, false);
				runner.set("where", where);
				runner.set("pos", QCursor::pos());
				runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
				
				PyObject *mimeData = mimeDataToObject(e->mimeData());
				if (!mimeData)
					break;
				
				runner.set("data", mimeData);
				if (e->proposedAction() == Qt::MoveAction)
					runner.set("action", SL_EVENT_DRAG_ACTION_MOVE);
				else
					runner.set("action", SL_EVENT_DRAG_ACTION_COPY);
				
				bool result = runner.run();
				if ((result) && (runner.get("action", &action))) {
					Qt::DropAction da;
					if (action & SL_EVENT_DRAG_ACTION_MOVE)
						da = Qt::MoveAction;
					else if (action & SL_EVENT_DRAG_ACTION_COPY)
						da = Qt::CopyAction;
					else
						da = Qt::IgnoreAction;
					e->setDropAction(da);
					e->acceptProposedAction();
				}
				if ((view) && (runner.get("where", &where))) {
					view->setProperty("dragWhere", QVariant::fromValue(where));
					view->setProperty("dragAccepted", QVariant::fromValue(e->isAccepted()));
					view->setProperty("dragHover", QVariant::fromValue(hover));
					view->viewport()->update();
				}
				if ((!qobject_cast<QLineEdit *>(obj)) && (!qobject_cast<QTextEdit *>(obj->parent())))
					return true;
			}
		}
		break;
	
	case QEvent::DragLeave:
		{
			EventRunner runner(obj, "onDragLeave");
			if (runner.isValid()) {
				runner.set("pos", QCursor::pos());
				runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
				runner.run();
				
				QAbstractItemView *view = qobject_cast<QAbstractItemView *>(obj->parent());
				if (view) {
					view->setProperty("dragAccepted", QVariant::fromValue(false));
					view->viewport()->update();
				}
			}
		}
		break;
	
	case QEvent::Drop:
		{
			QDropEvent *e = (QDropEvent *)event;
			EventRunner runner(obj, "onDrop");
			if (runner.isValid()) {
				QAbstractItemView *view = qobject_cast<QAbstractItemView *>(obj->parent());
				PyObject *index = Py_None;
				int where = SL_EVENT_DRAG_ON_ITEM;
	
				if (view) {
					DataModel_Impl *model = qobject_cast<DataModel_Impl *>(view->model());
					QModelIndex hover = qvariant_cast<QModelIndex>(view->property("dragHover"));
					index = model->getDataIndex(hover);
					where = qvariant_cast<int>(view->property("dragWhere"));
					view->setProperty("dragAccepted", QVariant::fromValue(false));
					if (dynamic_cast<WidgetInterface *>(view))
						QMetaObject::invokeMethod(view, "unprepareDrag", Qt::DirectConnection);
					
					QModelIndexList dragging = view->selectionModel()->selectedIndexes();
					QModelIndex child = hover;
					while (child.isValid()) {
						if ((dragging.contains(child)) && ((view == e->source()) || (view->isAncestorOf((QWidget *)e->source())))) {
							break;
						}
						child = child.parent();
					}
					if (child.isValid())
						return true;
				}
				else {
					QWidget *w = qobject_cast<QWidget *>(obj);
					if (w)
						w->update();
				}
				
				runner.set("pos", QCursor::pos());
				runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
				runner.set("index", index, false);
				runner.set("where", where);
				PyObject *mimeData = mimeDataToObject(e->mimeData());
				if (!mimeData)
					break;
				
				runner.set("data", mimeData);
				if (e->proposedAction() == Qt::MoveAction)
					runner.set("action", SL_EVENT_DRAG_ACTION_MOVE);
				else
					runner.set("action", SL_EVENT_DRAG_ACTION_COPY);
				
				bool result = runner.run();
				e->setAccepted(result);
				return !result;
			}
		}
		break;
	
	case QEvent::KeyPress:
		{
			QWidget *w = qobject_cast<QWidget *>(obj);
			if ((!w) || (w->focusProxy()))
				return false;
			
			hidePopupMessage();
		}
		/* fallthrough */
		
	case QEvent::KeyRelease:
		{
			QWidget *w = qobject_cast<QWidget *>(obj);
			if ((!w) || (w->focusProxy()))
				return false;
			
			QKeyEvent *e = (QKeyEvent *)event;
			bool sendCharEvent = false;
			
			int modifiers = getKeyModifiers(e->modifiers());
			QString text = e->text();
			int seq = SL_SEQUENCE_NONE;
			
			EventRunner runner(obj);
			if (runner.isValid()) {
				if (event->type() == QEvent::KeyPress) {
					if (text.isEmpty()) {
						if (e->matches(QKeySequence::Undo))
							seq = SL_SEQUENCE_UNDO;
						else if (e->matches(QKeySequence::Redo))
							seq = SL_SEQUENCE_REDO;
						else if (e->matches(QKeySequence::Cut))
							seq = SL_SEQUENCE_CUT;
						else if (e->matches(QKeySequence::Copy))
							seq = SL_SEQUENCE_COPY;
						else if (e->matches(QKeySequence::Paste))
							seq = SL_SEQUENCE_PASTE;
						else if (e->matches(QKeySequence::Close))
							seq = SL_SEQUENCE_CLOSE;
						else if (e->matches(QKeySequence::SelectAll))
							seq = SL_SEQUENCE_SELECT_ALL;
					}
					runner.setName("onKeyDown");
					runner.set("count", e->count());
					sendCharEvent = !text.isEmpty();
				}
				else {
					runner.setName("onKeyUp");
				}
				runner.set("modifiers", modifiers);
				runner.set("code", getKeyCode(e->key()));
				runner.set("char", text);
				runner.set("seq", seq);
				
				if (!runner.run())
					return true;
				
				if (sendCharEvent) {
					runner.setName("onChar");
					return !runner.run();
				}
			}
		}
		break;
	
	case QEvent::FileOpen:
		{
			PyAutoLocker locker;
			QFileOpenEvent *e = (QFileOpenEvent *)event;
			PyObject *file = createStringObject(e->file());
			
			if ((!sApplication) || (sApplication == Py_None)) {
				PyObject *args = PySys_GetObject("argv");
				PyList_Append(args, file);
			}
			else {
				PyObject *method = PyString_FromString("open_file");
				PyObject *result = PyObject_CallMethodObjArgs(sApplication, method, file, NULL);
				Py_DECREF(method);
				
				if (result) {
					Py_DECREF(result);
				}
				else {
					PyErr_Print();
					PyErr_Clear();
				}
			}
			Py_DECREF(file);
		}
		break;
	
	}
	
	return false;
}


void
Application::registerObject(QObject *object, Abstract_Proxy *proxy)
{
	QMutexLocker locker(fMutex);
	object->moveToThread(thread());
	QString name = QString("%1_%2").arg(object->metaObject()->className()).arg(fWID++);
	object->setObjectName(name);
	fWidgets[name] = (PyObject *)proxy;
}


void
Application::unregisterObject(QObject *object)
{
	QString name = object->objectName();
	QMutexLocker locker(fMutex);
	Abstract_Proxy *proxy = (Abstract_Proxy *)fWidgets[name];
	if (proxy) {
		fWidgets.remove(name);
		proxy->fImpl = NULL;
	}
}


void
Application::newProxy(Abstract_Proxy *proxy)
{
	QObject *object = proxy->fImpl;
	registerObject(object, proxy);
	QWidget *widget = qobject_cast<QWidget *>(object);
	if (widget) {
		widget->setAttribute(Qt::WA_LayoutUsesWidgetRect);
		widget->setAcceptDrops(true);
	}
}


void
Application::deallocProxy(Abstract_Proxy *proxy)
{
	QObject *object = proxy->fImpl;
	if (object) {
		if (fWidgets.remove(object->objectName())) {
			proxy->fImpl = NULL;
			object->deleteLater();
		}
	}
}


QObject *
Application::replaceProxyObject(Abstract_Proxy *proxy, QObject *object)
{
	QMutexLocker locker(fMutex);
	QObject *oldObject = proxy->fImpl;
	if (oldObject) {
		fWidgets.remove(oldObject->objectName());
	}
	proxy->fImpl = object;
	newProxy(proxy);
	return oldObject;
}


PyObject *
Application::getProxy(QObject *object)
{
	QMutexLocker locker(fMutex);
	return fWidgets[object->objectName()];
}


void
Application::sendTabEvent(QObject *receiver)
{
	QKeyEvent *e = new QKeyEvent(QEvent::KeyPress, (keyboardModifiers() & Qt::ShiftModifier) ? Qt::Key_Backtab : Qt::Key_Tab, 0);
	postEvent(receiver, e);
}


void
Application::startTimedCall(QObject *parent, QObject *object, int delay)
{
	int id;
	TimedCall *timedCall = (TimedCall *)object;
	
	if (parent) {
		id = parent->startTimer(delay);
		parent->setProperty("old_timed_call_id", QVariant::fromValue(id));
	}
	else
		id = qApp->startTimer(delay);
	timedCall->setId(id);
	sTimers[id] = timedCall;
}


SL_DEFINE_MODULE_METHOD(report_exception, {
	static char *kwlist[] = { "message", NULL };
	QString message;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kwlist, convertString, &message))
		return NULL;
	
	if ((sIsRunning) || (qApp->thread() != QThread::currentThread()))
		QApplication::postEvent(qApp, new ExceptionEvent(message));
	else if (qobject_cast<QApplication *>(qApp))
		ExceptionEvent(message).report();
})


SL_DEFINE_MODULE_METHOD(set_application_name, {
	static char *kwlist[] = { "name", NULL };
	QString name;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&", kwlist, convertString, &name))
		return NULL;
	
	qApp->setApplicationName(name);
})


SL_DEFINE_MODULE_METHOD(set_application_flags, {
	static char *kwlist[] = { "flags", NULL };
	int flags;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &flags))
		return NULL;
	
	qApp->setQuitOnLastWindowClosed(flags & SL_APPLICATION_QUIT_ON_WINDOWS_CLOSED ? true : false);
})


SL_DEFINE_MODULE_METHOD(run, {
	static char *kwlist[] = { "application", NULL };
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &sApplication))
		return NULL;
	
	Py_INCREF(sApplication);
	Py_BEGIN_ALLOW_THREADS
	
	sIsRunning = true;
	qApp->exec();
	
	Py_END_ALLOW_THREADS
	Py_DECREF(sApplication);
	sApplication = NULL;
	
    Py_RETURN_NONE;
})


SL_DEFINE_MODULE_METHOD(exit, {
	Application::exit(0);
	sIsRunning = false;
})


SL_DEFINE_MODULE_METHOD(process_events, {
	Py_BEGIN_ALLOW_THREADS
	
	QApplication::sendPostedEvents();
	QApplication::processEvents();
	
	Py_END_ALLOW_THREADS
})


SL_DEFINE_MODULE_METHOD(flush_events, {
	QApplication::flush();
})


SL_DEFINE_MODULE_METHOD(open_file, {
	static char *kwlist[] = { "message", "specs", "path", "multi", NULL };
	PyObject *specs, *sequence;
	QString message, path, name, ext, desc, wildcard;
	bool multi = false;
	Py_ssize_t i, size;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O|O&O&:open_file", kwlist, convertString, &message, &specs, convertString, &path, convertBool, &multi))
		return NULL;
	
	sequence = PySequence_Fast(specs, "Expected tuple object");
	if (!sequence)
		return NULL;
	
	size = PySequence_Fast_GET_SIZE(sequence);
	for (i = 0; i < size; i++) {
		PyObject *spec = PySequence_Fast_GET_ITEM(sequence, i);
		if ((i == 0) && ((PyString_Check(spec)) || (PyUnicode_Check(spec))) && (size == 2) &&
				((PyString_Check(PySequence_Fast_GET_ITEM(sequence, 1))) || (PyUnicode_Check(PySequence_Fast_GET_ITEM(sequence, 1))))) {
			convertString(spec, &ext);
			convertString(PySequence_Fast_GET_ITEM(sequence, 1), &desc);
			wildcard = desc + " (" + ext + ")";
			break;
		}
		spec = PySequence_Fast(spec, "Expected tuple object");
		if (!spec) {
			Py_DECREF(sequence);
			return NULL;
		}
		if (PySequence_Fast_GET_SIZE(spec) != 2) {
			PyErr_SetString(PyExc_TypeError, "Specification tuple object must be in the form (extension, description)");
			Py_DECREF(sequence);
			Py_DECREF(spec);
			return NULL;
		}
		if ((!convertString(PySequence_Fast_GET_ITEM(spec, 0), &ext)) || (!convertString(PySequence_Fast_GET_ITEM(spec, 1), &desc))) {
			Py_DECREF(sequence);
			Py_DECREF(spec);
			return NULL;
		}
		if (!wildcard.isEmpty())
			wildcard += ";;";
		wildcard += desc + " (" + ext + ")";
		Py_DECREF(spec);
	}
	Py_DECREF(sequence);
	
	if (wildcard.isEmpty())
		wildcard = "All files (*.*)";
	
	if (multi) {
		QStringList fileNames;
		
		Py_BEGIN_ALLOW_THREADS
		
		fileNames = QFileDialog::getOpenFileNames(NULL, message, path, wildcard);
		
		Py_END_ALLOW_THREADS
		
		int i, size = fileNames.count();
		
		if (size == 0)
			Py_RETURN_NONE;
		
		PyObject *tuple = PyTuple_New(size);
		QStringList list = fileNames;
		QStringList::Iterator it;
		for (i = 0, it = list.begin(); it != list.end(); it++, i++) {
			PyTuple_SET_ITEM(tuple, i, createStringObject(*it));
		}
		return tuple;
	}
	else {
		QString fileName;
		
		Py_BEGIN_ALLOW_THREADS
		
		fileName = QFileDialog::getOpenFileName(NULL, message, path, wildcard);
		
		Py_END_ALLOW_THREADS
		
		if (fileName.isEmpty())
			Py_RETURN_NONE;
		return createStringObject(fileName);
	}
})


SL_DEFINE_MODULE_METHOD(save_file, {
	static char *kwlist[] = { "message", "spec", "path", NULL };
	PyObject *spec;
	QString message, ext, desc, wildcard, path;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O|O&:save_file", kwlist, convertString, &message, &spec, convertString, &path))
		return NULL;
	
	spec = PySequence_Fast(spec, "Expected tuple object");
	if (!spec)
		return NULL;
	if (PySequence_Fast_GET_SIZE(spec) != 2) {
		PyErr_SetString(PyExc_TypeError, "Specification tuple object must be in the form (extension, description)");
		Py_DECREF(spec);
		return NULL;
	}
	if ((!convertString(PySequence_Fast_GET_ITEM(spec, 0), &ext)) || (!convertString(PySequence_Fast_GET_ITEM(spec, 1), &desc))) {
		Py_DECREF(spec);
		return NULL;
	}
	wildcard = desc + " (" + ext + ")";
	Py_DECREF(spec);
	
	while ((!ext.isEmpty()) && ((ext[0] == '.') || (ext[0] == '*')))
		ext = ext.mid(1);
	
	QString fileName;
	
	Py_BEGIN_ALLOW_THREADS
	
	fileName = QFileDialog::getSaveFileName(NULL, message, path, wildcard);
	
	Py_END_ALLOW_THREADS
	
	if (fileName.isEmpty())
		Py_RETURN_NONE;
	
	QFileInfo fileInfo(fileName);
	QString gotExt = fileInfo.completeSuffix();
	if ((!gotExt.isEmpty()) || (gotExt != ext))
		fileName = fileInfo.absolutePath() + QDir::separator() + fileInfo.completeBaseName() + "." + ext;
	
	fileInfo.setFile(fileName);
	bool exists = fileInfo.exists();
	if ((exists) && (!fileInfo.isWritable())) {
		QMessageBox::warning(NULL, "", QPrintDialog::tr("File %1 is not writable.\nPlease choose a different file name.").arg(fileInfo.fileName()));
		Py_RETURN_NONE;
	}
// 	else if (exists) {
// 		if (QMessageBox::question(NULL, "", QPrintDialog::tr("%1 already exists.\nDo you want to overwrite it?").arg(fileInfo.fileName()), QMessageBox::Yes|QMessageBox::No, QMessageBox::No) == QMessageBox::No)
// 			Py_RETURN_NONE;
// 	}
	
	return createStringObject(fileName);
})


SL_DEFINE_MODULE_METHOD(choose_directory, {
	static char *kwlist[] = { "message", "path", NULL };
	QString message, path;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&|O&:choose_directory", kwlist, convertString, &message, convertString, &path))
		return NULL;
	
	QString dir;
	
	Py_BEGIN_ALLOW_THREADS
	
	dir = QFileDialog::getExistingDirectory(NULL, message, path);
	
	Py_END_ALLOW_THREADS
	
	if (dir.isEmpty())
		Py_RETURN_NONE;
	
	return createStringObject(dir);
})


SL_DEFINE_MODULE_METHOD(load_resource, {
	static char *kwlist[] = { "resource", NULL };
	QString resource;
	QByteArray data;
	bool result;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:load_resource", kwlist, convertString, &resource))
		return NULL;
	
	Py_BEGIN_ALLOW_THREADS
	
	result = loadResource(resource, data);
	
	Py_END_ALLOW_THREADS
	
	if (!result) {
		PyErr_SetString(PyExc_IOError, "cannot read specified resource");
		return NULL;
	}
	
	return createBufferObject(data);
})


SL_DEFINE_MODULE_METHOD(message_box, {
	static char *kwlist[] = { "message", "title", "buttons", "icon", "callback", "userdata", NULL };
	QString message, title;
	int button = SL_BUTTON_OK, buttons = SL_BUTTON_OK, icon = SL_ICON_INFORMATION;
	PyObject *callback = NULL, *userdata = NULL;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&iiOO:message_box", kwlist, convertString, &message, convertString, &title, &buttons, &icon, &callback, &userdata))
		return NULL;
	
	messageBox(NULL, title, message, buttons, icon, callback, userdata, &button);
	
	return PyInt_FromLong(button);
})


SL_DEFINE_MODULE_METHOD(set_shortcut, {
	static char *kwlist[] = { "sequence", "action", NULL };
	QString seq;
	PyObject *action;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O:set_shortcut", kwlist, convertString, &seq, &action))
		return NULL;
	
	if ((action != Py_None) && (!PyCallable_Check(action))) {
		PyErr_SetString(PyExc_ValueError, "expected 'None' or callable object");
		return NULL;
	}
	
	setShortcut(SL_QAPP()->shadowWindow(), seq, Qt::ApplicationShortcut, action);
})


SL_DEFINE_MODULE_METHOD(page_setup, {
	PyObject *settings, *parent;
	bool accepted;
	
	if (!PyArg_ParseTuple(args, "OO", &settings, &parent))
		return NULL;
	
	if (!pageSetup(settings, parent, &accepted))
		return NULL;
	
	return createBoolObject(accepted);
})


SL_DEFINE_MODULE_METHOD(print_setup, {
	PyObject *settings, *parent;
	bool accepted;
	
	if (!PyArg_ParseTuple(args, "OO", &settings, &parent))
		return NULL;
	
	if (!printSetup(settings, parent, &accepted))
		return NULL;
	
	return createBoolObject(accepted);
})


SL_DEFINE_MODULE_METHOD(print_document, {
	static char *kwlist[] = { "type", "title", "callback", "prompt", "settings", "parent", NULL };
	int type;
	QString title;
	bool prompt;
	PyObject *callback, *settings, *parent;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO&OO&OO:print_document", kwlist, &type, convertString, &title, &callback, convertBool, &prompt, &settings, &parent))
		return NULL;
	
	return printDocument(type, title, callback, prompt, settings, parent);
})


SL_DEFINE_MODULE_METHOD(set_locale, {
	static char *kwlist[] = { "lang", NULL };
	QString lang;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:get_locale_info", kwlist, convertString, &lang))
		return NULL;
	
	sLocale = QLocale(lang);
	QLocale::setDefault(sLocale);
})


SL_DEFINE_MODULE_METHOD(get_locale_info, {
	static char *kwlist[] = { "lang", NULL };
	QString lang, format, dsep, tsep, data;
	PyObject *dict, *abbr, *full;
	bool american;
	int i;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:get_locale_info", kwlist, convertString, &lang))
		return NULL;
	
	QLocale locale(lang);
	dict = PyDict_New();
	
	format = locale.dateFormat(QLocale::NarrowFormat);
	for (i = 0; i < format.size(); i++) {
		if ((!format[i].isLetter()) && (!format[i].isSpace())) {
			dsep = format[i];
			break;
		}
	}
	american = format.indexOf('M') < format.indexOf('d');
	
	format = locale.timeFormat(QLocale::NarrowFormat);
	for (i = 0; i < format.size(); i++) {
		if ((!format[i].isLetter()) && (!format[i].isSpace())) {
			tsep = format[i];
			break;
		}
	}
	
	PyDict_SetItemString(dict, "iso_name", PyString_FromString(locale.name().mid(0,2).toUtf8()));
	PyDict_SetItemString(dict, "language", PyString_FromString(QLocale::languageToString(locale.language()).toUtf8()));
	PyDict_SetItemString(dict, "date_sep", PyString_FromString(dsep.toUtf8()));
	PyDict_SetItemString(dict, "time_sep", PyString_FromString(tsep.toUtf8()));
	PyDict_SetItemString(dict, "american_date", PyBool_FromLong(american ? 1 : 0));
	
	data = QString(locale.groupSeparator());
	PyDict_SetItemString(dict, "thousands_sep", createStringObject(data));
	PyDict_SetItemString(dict, "mon_thousands_sep", createStringObject(data));
	
	data = QString(locale.decimalPoint());
	PyDict_SetItemString(dict, "decimal_sep", createStringObject(data));
	PyDict_SetItemString(dict, "mon_decimal_sep", createStringObject(data));
	
	abbr = PyTuple_New(7);
	full = PyTuple_New(7);
	for (i = 1; i < 8; i++) {
		data = locale.dayName(i, QLocale::ShortFormat);
		PyTuple_SET_ITEM(abbr, i % 7, createStringObject(data));
		data = locale.dayName(i, QLocale::LongFormat);
		PyTuple_SET_ITEM(full, i % 7, createStringObject(data));
	}
	PyDict_SetItemString(dict, "wday", full);
	PyDict_SetItemString(dict, "wday_short", abbr);
	
	abbr = PyTuple_New(12);
	full = PyTuple_New(12);
	for (i = 0; i < 12; i++) {
		data = locale.monthName(i + 1, QLocale::ShortFormat);
		PyTuple_SET_ITEM(abbr, i, createStringObject(data));
		data = locale.monthName(i + 1, QLocale::LongFormat);
		PyTuple_SET_ITEM(full, i, createStringObject(data));
	}
	PyDict_SetItemString(dict, "month", full);
	PyDict_SetItemString(dict, "month_short", abbr);
	
	return dict;
})



#ifdef Q_OS_WIN


SL_DEFINE_MODULE_METHOD(get_computer_info, {
	PyObject *dict = PyDict_New();
	PyObject *host_name, *domain_name, *user_login_name, *user_full_name;
	WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
	WCHAR buffer2[32767];
	DWORD count = MAX_COMPUTERNAME_LENGTH + 1;
	
	if ((GetComputerNameExW(ComputerNameDnsHostname, buffer, &count)) || (GetComputerNameExW(ComputerNameNetBIOS, buffer, &count)))
		host_name = PyUnicode_FromWideChar(buffer, count);
	else
		host_name = PyString_FromString("Unknown");
	
	count = MAX_COMPUTERNAME_LENGTH + 1;
	if (!GetComputerNameExW(ComputerNameDnsDomain, buffer, &count))
		domain_name = PyString_FromString("");
	else
		domain_name = PyUnicode_FromWideChar(buffer, count);
	
	count = 32767;
	if (!GetUserNameW(buffer2, &count))
		user_login_name = PyString_FromString("Unknown");
	else
		user_login_name = PyUnicode_FromWideChar(buffer2, count);
	
	count = 32767;
	if (!GetUserNameExW(NameDisplay, buffer2, &count)) {
		user_full_name = user_login_name;
		Py_INCREF(user_full_name);
	}
	else
		user_full_name = PyUnicode_FromWideChar(buffer2, count);
	
	PyDict_SetItemString(dict, "host_name", host_name);
	PyDict_SetItemString(dict, "domain_name", domain_name);
	PyDict_SetItemString(dict, "user_login_name", user_login_name);
	PyDict_SetItemString(dict, "user_full_name", user_full_name);
	PyDict_SetItemString(dict, "cpu", PyInt_FromLong(0));
	return dict;
})


SL_DEFINE_MODULE_METHOD(get_path, {
	static char *kwlist[] = { "type", NULL };
	int spec;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:get_path", kwlist, &spec))
		return NULL;
	
	QString path;
	wchar_t buffer[1024];
	
	switch (spec) {
	case SL_EXECUTABLE_NAME:
		GetModuleFileName(NULL, buffer, 1024);
		return PyUnicode_FromWideChar(buffer, wcslen(buffer));
	case SL_USER_HOME_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_PROFILE, FALSE);
		break;
	case SL_USER_APPLICATION_SUPPORT_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_APPDATA, FALSE);
		break;
	case SL_USER_DOCUMENTS_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_MYDOCUMENTS, FALSE);
		break;
	case SL_USER_IMAGES_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_MYPICTURES, FALSE);
		break;
	case SL_USER_PREFERENCES_PATH:
	case SL_USER_LOG_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_LOCAL_APPDATA, FALSE);
		break;
	case SL_SYSTEM_APPLICATION_SUPPORT_PATH:
	case SL_SYSTEM_PREFERENCES_PATH:
	case SL_SYSTEM_LOG_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_COMMON_APPDATA, FALSE);
		break;
	case SL_SYSTEM_DOCUMENTS_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_COMMON_DOCUMENTS, FALSE);
		break;
	case SL_SYSTEM_IMAGES_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_COMMON_PICTURES, FALSE);
		break;
	case SL_SYSTEM_FONTS_PATH:
		SHGetSpecialFolderPath(NULL, buffer, CSIDL_FONTS, FALSE);
		break;
	default:
		buffer[0] = '\0';
		break;
	}
	if (buffer[0] == '\0') {
		GetModuleFileName(NULL, buffer, 1024);
		path = QString::fromWCharArray(buffer);
		path = path.mid(0, path.lastIndexOf('\\'));
	}
	else
		path = QString::fromWCharArray(buffer);

	path += QDir::separator();
	
	return createStringObject(path);
})


#elif defined(Q_OS_MAC)


SL_DEFINE_MODULE_METHOD(get_computer_info, {
	PyObject *dict = PyDict_New();
	PyObject *host_name, *domain_name, *user_login_name, *user_full_name;
	CFStringRef string;
	char buffer[1024];
	
	string = CSCopyMachineName();
	CFStringGetCString(string, buffer, sizeof(buffer), kCFStringEncodingUTF8);
	host_name = PyUnicode_DecodeUTF8(buffer, strlen(buffer), NULL);
	CFRelease(string);
	
	string = CSCopyUserName(TRUE);
	CFStringGetCString(string, buffer, sizeof(buffer), kCFStringEncodingUTF8);
	user_login_name = PyUnicode_DecodeUTF8(buffer, strlen(buffer), NULL);
	CFRelease(string);
	
	string = CSCopyUserName(FALSE);
	CFStringGetCString(string, buffer, sizeof(buffer), kCFStringEncodingUTF8);
	user_full_name = PyUnicode_DecodeUTF8(buffer, strlen(buffer), NULL);
	CFRelease(string);
	
	int mib[2] = { CTL_KERN, KERN_NISDOMAINNAME };
	size_t size = sizeof(buffer);
	if (!sysctl(mib, 2, buffer, &size, NULL, 0))
		domain_name = PyUnicode_DecodeUTF8(buffer, strlen(buffer), NULL);
	else
		domain_name = PyString_FromString("");

	PyDict_SetItemString(dict, "host_name", host_name);
	PyDict_SetItemString(dict, "domain_name", domain_name);
	PyDict_SetItemString(dict, "user_login_name", user_login_name);
	PyDict_SetItemString(dict, "user_full_name", user_full_name);
	PyDict_SetItemString(dict, "cpu", PyInt_FromLong(0));
	return dict;
})


SL_DEFINE_MODULE_METHOD(get_path, {
	static char *kwlist[] = { "type", NULL };
	int spec;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:get_path", kwlist, &spec))
		return NULL;
	
	QString path;
	CFBundleRef bundle = NULL;
	CFURLRef url = NULL;
	FSRef folderRef;
	char dir[1024];
	uint32_t size = 1024;
	QString exePath = ".";
	
	if (in_bundle())
		bundle = CFBundleGetMainBundle();
	if (_NSGetExecutablePath(dir, &size) == 0) {
		exePath = dir;
		exePath = exePath.mid(0, exePath.lastIndexOf('/'));
	}
	
	switch (spec) {
	case SL_EXECUTABLE_NAME:
		if (_NSGetExecutablePath(dir, &size) == 0)
			return PyUnicode_DecodeUTF8(dir, strlen(dir), NULL);
		return PyString_FromString("");
	case SL_EXECUTABLE_PATH:
		if (bundle)
			url = CFBundleCopyExecutableURL(bundle);
		break;
	case SL_APPLICATION_PATH:
		if (bundle)
			url = CFBundleCopyBundleURL(bundle);
		break;
	case SL_RESOURCE_PATH:
		if (bundle)
			url = CFBundleCopyResourcesDirectoryURL(bundle);
		break;
	case SL_USER_HOME_PATH:
		if (FSFindFolder(kUserDomain, kCurrentUserFolderType, kDontCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_USER_APPLICATION_SUPPORT_PATH:
		if (FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_USER_DOCUMENTS_PATH:
		if (FSFindFolder(kUserDomain, kDocumentsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_USER_IMAGES_PATH:
		if (FSFindFolder(kUserDomain, kPictureDocumentsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_USER_PREFERENCES_PATH:
		if (FSFindFolder(kUserDomain, kPreferencesFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_USER_LOG_PATH:
		if (FSFindFolder(kUserDomain, kLogsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_APPLICATION_SUPPORT_PATH:
		if (FSFindFolder(kLocalDomain, kApplicationSupportFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_DOCUMENTS_PATH:
		if (FSFindFolder(kLocalDomain, kDocumentsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_IMAGES_PATH:
		if (FSFindFolder(kLocalDomain, kPictureDocumentsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_PREFERENCES_PATH:
		if (FSFindFolder(kLocalDomain, kPreferencesFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_FONTS_PATH:
		if (FSFindFolder(kLocalDomain, kFontsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	case SL_SYSTEM_LOG_PATH:
		if (FSFindFolder(kLocalDomain, kLogsFolderType, kCreateFolder, &folderRef) == noErr)
			url = CFURLCreateFromFSRef(NULL, &folderRef);
		break;
	}
	
	if (url) {
		if (CFURLGetFileSystemRepresentation(url, true, (UInt8 *)dir, sizeof(dir) - 1))
			path = dir;
		CFRelease(url);
		if ((spec == SL_EXECUTABLE_PATH) ||
			((spec == SL_APPLICATION_PATH) && (path != exePath))) {
			path = path.mid(0, path.lastIndexOf('/'));
		}
	}
	else
		path = exePath;
	
	path += QDir::separator();
	
	return createStringObject(path);
})


#else


SL_DEFINE_MODULE_METHOD(get_computer_info, {
	PyObject *dict = PyDict_New();
	PyObject *host_name, *domain_name, *user_login_name, *user_full_name;
	struct passwd *pwd = getpwuid(geteuid());
	char buffer[256];
	
	if (pwd) {
		user_login_name = PyString_FromString(pwd->pw_name);
		user_full_name = PyString_FromString(pwd->pw_gecos);
	}
	else {
		user_login_name = PyString_FromString("Unknown");
		user_full_name = user_login_name;
		Py_INCREF(user_full_name);
	}
	if (gethostname(buffer, sizeof(buffer)) == 0)
		host_name = PyString_FromString(buffer);
	else
		host_name = PyString_FromString("");
	domain_name = NULL;
	if (getdomainname(buffer, sizeof(buffer)) == 0) {
		buffer[255] = 0;
		if (strcmp(buffer, "(none)"))
			domain_name = PyString_FromString(buffer);
	}
	if (!domain_name)
		domain_name = PyString_FromString("");
	
	PyDict_SetItemString(dict, "host_name", host_name);
	PyDict_SetItemString(dict, "domain_name", domain_name);
	PyDict_SetItemString(dict, "user_login_name", user_login_name);
	PyDict_SetItemString(dict, "user_full_name", user_full_name);
	PyDict_SetItemString(dict, "cpu", PyInt_FromLong(0));
	return dict;
})
	

SL_DEFINE_MODULE_METHOD(get_path, {
	static char *kwlist[] = { "type", NULL };
	int spec;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:get_path", kwlist, &spec))
		return NULL;
	
	QString path;
	struct stat file_stat;
	char temp[1024], temp2[1024], exe_path[1024], *p;
	const char *home = getenv("HOME"), *var = NULL, *exe_name = NULL;
	int len;
	
	if (!home)
		home = "/usr/share";
	
	sprintf(temp, "/proc/%d/exe", (int)getpid());
	if (stat(temp, &file_stat) >= 0) {
		len = readlink(temp, exe_path, 1023);
		if (len >= 0) {
			exe_path[len] = '\0';
			p = strrchr(exe_path, '/');
			*p = 0;
			strcpy(temp, p + 1);
			exe_name = temp;
		}
	}
	if (!exe_name) {
		exe_name = "slew";
		strcpy(exe_path, home);
	}
	
	switch (spec) {
	case SL_EXECUTABLE_NAME:
		return createStringObject(QString("%1/%2").arg(exe_path).arg(exe_name));
	case SL_USER_HOME_PATH:
		path = home;
		break;
	case SL_USER_APPLICATION_SUPPORT_PATH:
	case SL_USER_LOG_PATH:
		var = getenv("XDG_DATA_HOME");
		path = QString(var ? var : "");
		if (path.isEmpty())
			path = QString("%1/.local/share").arg(home);
		if (spec == SL_USER_LOG_PATH)
			path.append("/log");
		else
			path.append("/data");
		break;
	case SL_USER_PREFERENCES_PATH:
	case SL_USER_DOCUMENTS_PATH:
	case SL_USER_IMAGES_PATH:
		var = getenv("XDG_CONFIG_HOME");
		path = QString(var ? var : "");
		if (path.isEmpty())
			path = QString("%1/.config").arg(home);
		if (spec == SL_USER_PREFERENCES_PATH)
			break;
		else if (spec == SL_USER_DOCUMENTS_PATH)
			var = "XDG_DOCUMENTS_DIR=";
		else
			var = "XDG_PICTURES_DIR=";
		sprintf(temp, "%s/%s", path.toUtf8().data(), "user-dirs.dirs");
		path = "";
		if (stat(temp, &file_stat) >= 0) {
			FILE *f = fopen(temp, "rb");
			while (!feof(f)) {
				p = fgets(temp, 1023, f);
				if (!p)
					break;
				if (strncmp(p, var, strlen(var)) == 0) {
					p += strlen(var);
					len = strlen(p);
					if (len == 0)
						continue;
					if (p[len - 1] == '\n') {
						p[len - 1] = 0;
						len--;
					}
					if ((len >= 2) && (p[0] == '\"') && (p[len-1] == '\"')) {
						p[len-1] = 0;
						p++;
						len -= 2;
					}
					if (memcmp(p, "$HOME", 5) == 0) {
						sprintf(temp2, "%s%s", home, p+5);
						path = temp2;
					}
					else {
						path = p;
					}
					break;
				}
			}
			fclose(f);
		}
		if (path.isEmpty()) {
			if (spec == SL_USER_DOCUMENTS_PATH)
				path = QString("%1/Documents").arg(home);
			else
				path = QString("%1/Pictures").arg(home);
		}
		break;
	case SL_SYSTEM_APPLICATION_SUPPORT_PATH:
	case SL_SYSTEM_IMAGES_PATH:
	case SL_SYSTEM_DOCUMENTS_PATH:
		path = "/usr/share";
		break;
	case SL_SYSTEM_PREFERENCES_PATH:
		path = "/etc";
		break;
	case SL_SYSTEM_FONTS_PATH:
		path = "/usr/share/fonts/truetype";
		break;
	case SL_SYSTEM_LOG_PATH:
		path = "/var/log";
		break;
	default:
		path = exe_path;
		break;
	}
	
	if (path.isEmpty())
		path = ".";

	path += QDir::separator();
	
	return createStringObject(path);
})

#endif
	


SL_DEFINE_MODULE_METHOD(get_available_desktop_rect, {
	QRect rect = QApplication::desktop()->availableGeometry();
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(rect.topLeft()));
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(rect.bottomRight()));
	return tuple;
})


SL_DEFINE_MODULE_METHOD(get_fonts_list, {
	QStringList families = QFontDatabase().families();
	PyObject *tuple = PyTuple_New(families.size());
	Py_ssize_t i = 0;
	
	foreach (QString family, families) {
		PyTuple_SET_ITEM(tuple, i++, createStringObject(family));
	}
	return tuple;
})


SL_DEFINE_MODULE_METHOD(run_color_dialog, {
	static char *kwlist[] = { "color", "title", NULL };
	QString title;
	QColor color;
	QWidget *focus = QApplication::activeWindow();
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&:run_color_dialog", kwlist, convertColor, &color, convertString, &title))
		return NULL;
	
	Py_BEGIN_ALLOW_THREADS
	
	color = QColorDialog::getColor(color, NULL, title, QColorDialog::ShowAlphaChannel);
	
	Py_END_ALLOW_THREADS
	
	if (focus)
		focus->activateWindow();
	
	return createColorObject(color);
})


#ifndef Q_OS_MAC

SL_DEFINE_MODULE_METHOD(run_font_dialog, {
	static char *kwlist[] = { "font", "title", NULL };
	QFont font;
	QString title;
	bool ok;
	QWidget *focus = QApplication::activeWindow();
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&:run_font_dialog", kwlist, convertFont, &font, convertString, &title))
		return NULL;
	
	Py_BEGIN_ALLOW_THREADS
	
	font.resolve(QApplication::font());
	font.setPointSize(floor(double(font.pointSize()) * 1.33333333333));
	font = QFontDialog::getFont(&ok, font, NULL, title);
	
	Py_END_ALLOW_THREADS
	
	if (focus)
		focus->activateWindow();
	
	if (!ok)
		Py_RETURN_NONE;
	
	font.setPointSize(ceil(double(font.pointSize()) * 0.75));
	return createFontObject(font);
})

#else

SL_DEFINE_MODULE_METHOD(run_font_dialog, {
	static char *kwlist[] = { "font", "title", NULL };
	QFont font;
	QString title;
	bool ok;
	QWidget *focus = QApplication::activeWindow();
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&:run_font_dialog", kwlist, convertFont, &font, convertString, &title))
		return NULL;
	
	Py_BEGIN_ALLOW_THREADS
	
	font.resolve(QApplication::font());
	font = QFontDialog::getFont(&ok, font, NULL, title);
	
	Py_END_ALLOW_THREADS
	
	if (focus)
		focus->activateWindow();
	
	if (!ok)
		Py_RETURN_NONE;
	
	return createFontObject(font);
})

#endif


SL_DEFINE_MODULE_METHOD(normalize_format, {
	static char *kwlist[] = { "format", "vars", NULL };
	QString format;
	PyObject *dict, *key, *value;
	Py_ssize_t pos = 0;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O!:normalize_format", kwlist, convertString, &format, &PyDict_Type, &dict))
		return NULL;
	
	QHash<QString, QString> vars;
	
	while (PyDict_Next(dict, &pos, &key, &value)) {
		QString k, v;
		if (!convertString(key, &k))
			return NULL;
		if (!convertString(value, &v)) {
			PyErr_Clear();
			PyObject *o = PyObject_Str(value);
			if (!o)
				return NULL;
			convertString(o, &v);
			Py_DECREF(o);
		}
		
		vars[k] = v;
	}
	
	format = normalizeFormat(vars, format);
	return createStringObject(format);
})


SL_DEFINE_MODULE_METHOD(format_datatype, {
	static char *kwlist[] = { "datatype", "format", "value", NULL };
	int datatype;
	QString format, value, text;
	QColor color;
	Qt::Alignment align;
	FormatInfo info[2];
	PyObject *tuple, *v, *c, *a;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO&O&:format_datatype", kwlist, &datatype, convertString, &format, convertString, &value))
		return NULL;
	
	parseFormat(format, datatype, info);
	text = getFormattedValue(value, &color, &align, datatype, info);
	
	v = createStringObject(text);
	c = createColorObject(color);
	a = PyInt_FromLong(toAlign(align));
	tuple = PyTuple_Pack(3, v, c, a);
	Py_DECREF(v);
	Py_DECREF(c);
	Py_DECREF(a);
	
	return tuple;
})


SL_DEFINE_MODULE_METHOD(open_uri, {
	static char *kwlist[] = { "url", NULL };
	QString url;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:open_uri", kwlist, convertString, &url))
		return NULL;
	
	if (!openURI(url))
		return NULL;
})


SL_DEFINE_MODULE_METHOD(get_clipboard_data, {
	static char *kwlist[] = { "mimetype", NULL };
	QString mimeType;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:get_clipboard_data", kwlist, convertString, &mimeType))
		return NULL;
	
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();
	PyObject *object;
	
	if (!mimeData)
		Py_RETURN_NONE;
	
	object = mimeDataToObject(mimeData, mimeType);
	if (!object)
		Py_RETURN_NONE;
	return object;
})


SL_DEFINE_MODULE_METHOD(has_clipboard_data, {
	static char *kwlist[] = { "mimetype", NULL };
	QString mimeType;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&:has_clipboard_data", kwlist, convertString, &mimeType))
		return NULL;
	
	QClipboard *clipboard = QApplication::clipboard();
	const QMimeData *mimeData = clipboard->mimeData();
	if (!mimeData)
		Py_RETURN_FALSE;
	
	if (mimeType.isEmpty()) {
		if (mimeData->formats().empty())
			Py_RETURN_FALSE;
		else
			Py_RETURN_TRUE;
	}
	else {
		int pos = mimeType.lastIndexOf("*");
		if (pos >= 0) {
			mimeType = mimeType.mid(0, pos);
			if (mimeType.isEmpty()) {
				if (mimeData->formats().empty())
					Py_RETURN_FALSE;
				else
					Py_RETURN_TRUE;
			}
			foreach (QString format, mimeData->formats()) {
				if (format == "application/x-qt-image")
					format = "image/*";
				if (format.startsWith(mimeType)) {
					format = format.mid(mimeType.size());
					if (!format.isEmpty())
						Py_RETURN_TRUE;
				}
			}
			Py_RETURN_FALSE;
		}
		else {
			if (mimeData->formats().contains(mimeType))
				Py_RETURN_TRUE;
			else
				Py_RETURN_FALSE;
		}
	}
})


SL_DEFINE_MODULE_METHOD(set_clipboard_data, {
	static char *kwlist[] = { "data", "mimetype", NULL };
	PyObject *data;
	QString mimeType;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O&:set_clipboard_data", kwlist, &data, convertString, &mimeType))
		return NULL;
	
	QClipboard *clipboard = QApplication::clipboard();
	QMimeData *mimeData = new QMimeData();
	
	if (mimeType.isEmpty()) {
		if (objectToMimeData(data, mimeData)) {
			clipboard->setMimeData(mimeData);
			Py_RETURN_NONE;
		}
	}
	else {
		QByteArray buffer;
		if (!convertBuffer(data, &buffer))
			return NULL;
		mimeData->setData(mimeType, buffer);
		clipboard->setMimeData(mimeData);
		Py_RETURN_NONE;
	}
	
	delete mimeData;
	PyErr_SetString(PyExc_ValueError, "cannot set data into clipboard: unsupported data type");
	return NULL;
})


SL_DEFINE_MODULE_METHOD(add_clipboard_data, {
	static char *kwlist[] = { "data", "mimetype", NULL };
	QString mimeType;
	PyObject *data;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO&:add_clipboard_data", kwlist, &data, convertString, &mimeType))
		return NULL;
	
	QClipboard *clipboard = QApplication::clipboard();
	QMimeData *mimeData = (QMimeData *)clipboard->mimeData();
	
	QByteArray buffer;
	if (!convertBuffer(data, &buffer))
		return NULL;
	
	if (!mimeData) {
		mimeData = new QMimeData();
		clipboard->setMimeData(mimeData);
	}
	
	mimeData->setData(mimeType, buffer);
})


SL_DEFINE_MODULE_METHOD(call_later_timeout, {
	Py_ssize_t size = PyTuple_Size(args);
	if (PyErr_Occurred())
		return NULL;
	
	if (size < 2) {
		PyErr_SetString(PyExc_ValueError, "missing parameter(s)");
		return NULL;
	}
	
	int timeout = PyInt_AsLong(PyTuple_GetItem(args, 0));
	if (PyErr_Occurred())
		return NULL;
	
	PyObject *func = PyTuple_GetItem(args, 1);
	if (!PyCallable_Check(func)) {
		PyErr_SetString(PyExc_ValueError, "'func' parameter must be a callable");
		return NULL;
	}
	PyObject *func_args = PyTuple_GetSlice(args, 2, size);
	
	setTimeout(NULL, timeout, func, func_args);
	
	Py_DECREF(func_args);
})


SL_DEFINE_MODULE_METHOD(get_mouse_buttons, {
	return PyInt_FromLong((long)QApplication::mouseButtons());
})


SL_DEFINE_MODULE_METHOD(get_mouse_pos, {
	return createVectorObject(QCursor::pos());
})


SL_DEFINE_MODULE_METHOD(get_keyboard_modifiers, {
	return PyInt_FromLong((long)getKeyModifiers(QApplication::queryKeyboardModifiers()));
})


SL_DEFINE_MODULE_METHOD(get_keyboard_repeat_rate, {
	if (qobject_cast<QApplication *>(qApp))
		return PyInt_FromLong((long)qApp->keyboardInputInterval());
	return PyInt_FromLong(0);
})


SL_DEFINE_MODULE_METHOD(set_keyboard_repeat_rate, {
	if (qobject_cast<QApplication *>(qApp)) {
		static char *kwlist[] = { "rate", NULL };
		int rate;
		if (!PyArg_ParseTupleAndKeywords(args, kwds, "i:set_keyboard_repeat_rate", kwlist, &rate))
			return NULL;
		qApp->setKeyboardInputInterval(rate);
	}
})


SL_DEFINE_MODULE_METHOD(get_standard_bitmap, {
	static char *kwlist[] = { "bitmap", "size", NULL };
	int type;
	QSize size;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "iO&:get_standard_bitmap", kwlist, &type, convertSize, &size))
		return NULL;
	
	QStyle::StandardPixmap qtype;
	switch (type) {
	case SL_ICON_ERROR:			qtype = QStyle::SP_MessageBoxCritical; break;
	case SL_ICON_QUESTION:		qtype = QStyle::SP_MessageBoxQuestion; break;
	case SL_ICON_WARNING:		qtype = QStyle::SP_MessageBoxWarning; break;
	case SL_ICON_INFORMATION:	qtype = QStyle::SP_MessageBoxInformation; break;
	default:
		Py_RETURN_NONE;
	}
	
	return createBitmapObject(QApplication::style()->standardIcon(qtype).pixmap(size));
})


#ifdef Q_OS_MAC

SL_DEFINE_MODULE_METHOD(get_screen_dpi, {
	QDesktopWidget *desktop = QApplication::desktop();
	CGSize size = CGDisplayScreenSize(CGMainDisplayID());
	if ((size.width > 0) && (size.height > 0))
		return createVectorObject(QSizeF((25.4 * (double)desktop->width()) / size.width, (25.4 * (double)desktop->height()) / size.height));
	return createVectorObject(QSizeF(desktop->physicalDpiX(), desktop->physicalDpiY()));
})

#elif defined(Q_OS_WIN)

SL_DEFINE_MODULE_METHOD(get_screen_dpi, {
	QDesktopWidget *desktop = QApplication::desktop();
	QSize size;
	DISPLAY_DEVICE ddAdapter;
	int adapter = 0;
	
	memset(&ddAdapter, 0, sizeof(ddAdapter));
	ddAdapter.cb = sizeof(ddAdapter);
	
	while ((EnumDisplayDevices(NULL, adapter, &ddAdapter, 0)) && (adapter < 20) && (!size.isValid())) {
		if (!(ddAdapter.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && (ddAdapter.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
			DISPLAY_DEVICE ddMon;
			memset(&ddMon, 0, sizeof(ddMon));
			ddMon.cb = sizeof(ddMon);
			int screen = 0;
			while (EnumDisplayDevices(ddAdapter.DeviceName, screen, &ddMon, 0)) {
				if ((ddMon.StateFlags & DISPLAY_DEVICE_ACTIVE) &&
					(ddMon.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)) {
					
					QString model = QString::fromWCharArray(ddMon.DeviceID);
					QString driver;
					model = model.mid(model.indexOf('\\') + 1);
					driver = model.mid(model.indexOf('\\') + 1);
					model = model.mid(0, model.indexOf('\\'));
					
					QSettings displays(QString("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\DISPLAY\\%1").arg(model), QSettings::NativeFormat);
					if (displays.status() == QSettings::NoError) {
						foreach (QString path, displays.childGroups()) {
							if (displays.value(QString("%1/Driver").arg(path)) == driver) {
								QString data = qvariant_cast<QString>(displays.value(QString("%1/Device Parameters/EDID").arg(path)));
								QByteArray EDID((const char *)data.utf16(), data.length() * 2);
								char byte1 = EDID[8];
								char byte2 = EDID[9];
								char temp[9];
								temp[0] = ((byte1 & 0x7C) >> 2) + 64;
								temp[1] = ((byte1 & 3) << 3) + ((byte2 & 0xE0) >> 5) + 64;
								temp[2] = (byte2 & 0x1F) + 64;
								sprintf_s(temp + 3, 5, "%X%X%X%X", (EDID[11] & 0xf0) >> 4, EDID[11] & 0xf, (EDID[10] & 0xf0) >> 4, EDID[10] & 0x0f);
								if (model == temp)
									size = QSize(EDID[21], EDID[22]);
							}
						}
					}
					
				}
				screen++;
			}
		}
		adapter++;
	}
	
	if (size.isValid())
		return createVectorObject(QSizeF((2.54 * (double)desktop->width()) / size.width(), (2.54 * (double)desktop->height()) / size.height()));
	return createVectorObject(QSizeF(desktop->physicalDpiX(), desktop->physicalDpiY()));
})


#else

SL_DEFINE_MODULE_METHOD(get_screen_dpi, {
	QDesktopWidget *desktop = QApplication::desktop();
	return createVectorObject(QSizeF(desktop->physicalDpiX(), desktop->physicalDpiY()));
})

#endif


SL_DEFINE_MODULE_METHOD(get_screen_bitmap, {
	QDesktopWidget *desktop = QApplication::desktop();
	return createBitmapObject(QPixmap::grabWindow(desktop->winId()));
})


SL_DEFINE_MODULE_METHOD(find_focus, {
	PyObject *focus = getObject(QApplication::focusWidget());
	if (!focus)
		Py_RETURN_NONE;
	
	return focus;
})


SL_DEFINE_MODULE_METHOD(beep, {
	QApplication::beep();
})


SL_DEFINE_MODULE_METHOD(get_backend_info, {
	return createStringObject(QString("Qt %1").arg(qVersion()));
})


SL_DEFINE_MODULE_METHOD(get_font_text_extent, {
	static char *kwlist[] = { "font", "text", "max_width", NULL };
	QFont font;
	QString text;
	int max_width;
	
	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O&O&i:get_font_text_extent", kwlist, convertFont, &font, convertString, &text, &max_width))
		return NULL;
	
	QFontMetrics fm(font);
	QRect rect = fm.boundingRect(0, 0, max_width <= 0 ? 1000000000 : max_width, 1000000000, Qt::AlignTop | Qt::AlignLeft | (max_width <= 0 ? 0 : Qt::TextWordWrap), text);
	return createVectorObject(rect.size());
})


#ifdef Q_OS_MAC

SL_DEFINE_MODULE_METHOD(mac_set_dock_menu, {
	PyObject *object;
	QMenu *menu;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		Py_XDECREF(sCurrentDockMenu);
		sCurrentDockMenu = NULL;
	}
	else {
		Py_INCREF(object);
		menu = (QMenu *)getImpl(object);
		Py_XDECREF(sCurrentDockMenu);
		sCurrentDockMenu = object;
		helper_set_dock_menu(menu);
	}
})

#endif


SL_START_METHODS(slew)
SL_METHOD(exit)
SL_METHOD(report_exception)
SL_METHOD(set_application_name)
SL_METHOD(set_application_flags)
SL_METHOD(run)
SL_METHOD(process_events)
SL_METHOD(flush_events)
SL_METHOD(open_file)
SL_METHOD(save_file)
SL_METHOD(choose_directory)
SL_METHOD(load_resource)
SL_METHOD(message_box)
SL_METHOD(set_shortcut)
SL_METHOD(page_setup)
SL_METHOD(print_setup)
SL_METHOD(print_document)
SL_METHOD(set_locale)
SL_METHOD(get_locale_info)
SL_METHOD(get_computer_info)
SL_METHOD(get_path)
SL_METHOD(get_available_desktop_rect)
SL_METHOD(get_fonts_list)
SL_METHOD(run_color_dialog)
SL_METHOD(run_font_dialog)
SL_METHOD(normalize_format)
SL_METHOD(format_datatype)
SL_METHOD(open_uri)
SL_METHOD(get_clipboard_data)
SL_METHOD(has_clipboard_data)
SL_METHOD(set_clipboard_data)
SL_METHOD(add_clipboard_data)
SL_METHOD(call_later_timeout)
SL_METHOD(get_mouse_buttons)
SL_METHOD(get_mouse_pos)
SL_METHOD(get_keyboard_modifiers)
SL_METHOD(get_keyboard_repeat_rate)
SL_METHOD(set_keyboard_repeat_rate)
SL_METHOD(get_standard_bitmap)
SL_METHOD(get_screen_dpi)
SL_METHOD(get_screen_bitmap)
SL_METHOD(find_focus)
SL_METHOD(beep)
SL_METHOD(get_backend_info)
SL_METHOD(get_font_text_extent)
#ifdef Q_OS_MAC
SL_METHOD(mac_set_dock_menu)
#endif
SL_END_METHODS()



static void
cleanup()
{
	if (qApp) {
		QApplication::flush();
		Application::exit(0);
		QApplication::sendPostedEvents();
		QApplication::processEvents();
		delete qApp;
	}
	delete sResourceReader;
	for (int i = 0; i < sArgc; i++)
		free(sArgv[i]);
	free(sArgv);
}


PyMODINIT_FUNC EXPORT
init_slew()
{
	PyObject *module, *dict = NULL, *sysargv, *executable;
	bool gui = true;
	QString archivePath;
	
	PyDateTime_IMPORT;
	PyEval_InitThreads();
	
	module = Py_InitModule3("slew._slew", slew::methods, "Slew Qt implementation module.\n");
	
	if ((!Abstract_type_setup(module)) ||
		(!Widget_type_setup(module)) ||
		(!Window_type_setup(module)) ||
		(!Frame_type_setup(module)) ||
		(!Dialog_type_setup(module)) ||
		(!PopupWindow_type_setup(module)) ||
		(!MenuBar_type_setup(module)) ||
		(!Menu_type_setup(module)) ||
		(!MenuItem_type_setup(module)) ||
		(!StatusBar_type_setup(module)) ||
		(!ToolBar_type_setup(module)) ||
		(!ToolBarItem_type_setup(module)) ||
		(!Sizer_type_setup(module)) ||
		(!Panel_type_setup(module)) ||
		(!FoldPanel_type_setup(module)) ||
		(!ScrollView_type_setup(module)) ||
		(!SplitView_type_setup(module)) ||
		(!TabView_type_setup(module)) ||
		(!TabViewPage_type_setup(module)) ||
		(!StackView_type_setup(module)) ||
		(!Label_type_setup(module)) ||
		(!Button_type_setup(module)) ||
		(!ToolButton_type_setup(module)) ||
		(!GroupBox_type_setup(module)) ||
		(!CheckBox_type_setup(module)) ||
		(!Radio_type_setup(module)) ||
		(!Line_type_setup(module)) ||
		(!Image_type_setup(module)) ||
		(!Ranged_type_setup(module)) ||
		(!Slider_type_setup(module)) ||
		(!SpinField_type_setup(module)) ||
		(!Progress_type_setup(module)) ||
		(!TextField_type_setup(module)) ||
		(!TextView_type_setup(module)) ||
		(!SearchField_type_setup(module)) ||
		(!Calendar_type_setup(module)) ||
		(!ComboBox_type_setup(module)) ||
		(!ListView_type_setup(module)) ||
		(!Grid_type_setup(module)) ||
		(!TreeView_type_setup(module)) ||
		(!SceneView_type_setup(module)) ||
		(!Wizard_type_setup(module)) ||
		(!WizardPage_type_setup(module)) ||
		
		(!DC_type_setup(module)) ||
		(!PrintDC_type_setup(module)) ||
		(!SceneItemDC_type_setup(module)) ||
		(!Bitmap_type_setup(module)) ||
		(!DataModel_type_setup(module)) ||
		(!SceneItem_type_setup(module)) ||
		(!WebView_type_setup(module)) ||
		
		(!SystrayIcon_type_setup(module)))
		return;
	
	module = PyImport_AddModule("slew");
	if (module) {
		dict = PyModule_GetDict(module);
		PyEvent_Type = PyDict_GetItemString(dict, "Event");
		PyPaper_Type = PyDict_GetItemString(dict, "Paper");
		sVectorType = PyDict_GetItemString(dict, "Vector");
		sColorType = PyDict_GetItemString(dict, "Color");
		sFontType = PyDict_GetItemString(dict, "Font");
		PyDC_Type = PyDict_GetItemString(dict, "DC");
		PyPrintDC_Type = PyDict_GetItemString(dict, "PrintDC");
		sBitmapType = PyDict_GetItemString(dict, "Bitmap");
		sIconType = PyDict_GetItemString(dict, "Icon");
		PyDataIndex_Type = PyDict_GetItemString(dict, "DataIndex");
		PyDataSpecifier_Type = PyDict_GetItemString(dict, "DataSpecifier");
		PyDataModel_Type = PyDict_GetItemString(dict, "DataModel");
		sSerializeData = PyDict_GetItemString(dict, "_serialize_data");
		sUnserializeData = PyDict_GetItemString(dict, "_unserialize_data");
	}
	
	sysargv = PySys_GetObject("argv");
	executable = PySys_GetObject("executable");
	
	if ((sysargv) && (executable)) {
		sArgc = 1;
		sArgv = (char **)malloc(sizeof(char *) * 2);
		sArgv[0] = strdup(PyString_AsString(executable));
		for (Py_ssize_t i = 0; i < PyList_Size(sysargv); i++) {
			char *arg = PyString_AsString(PyList_GetItem(sysargv, i));
			if (arg[0]) {
				if (!strcmp(arg, "--slew-no-gui")) {
					gui = false;
					PyList_SetSlice(sysargv, i, i + 1, NULL);
				}
				else {
					sArgv = (char **)realloc(sArgv, sizeof(char *) * (sArgc + 2));
					sArgv[sArgc++] = strdup(arg);
				}
			}
		}
		sArgv[sArgc] = NULL;
	}
	else {
		sArgc = 1;
		sArgv = (char **)malloc(sizeof(char *));
		sArgv[0] = strdup("__main__");
	}
	
	module = PyImport_AddModule("__main__");
	if (module)
		dict = PyModule_GetDict(module);
	
#ifdef Q_OS_WIN
	{
		QString temp;
		wchar_t buffer[32767];
		if (GetEnvironmentVariable(L"SLEW_GUI", buffer, 32767)) {
			temp = QString::fromWCharArray(buffer);
			if ((temp == "0") || (temp.toLower() == "false"))
				gui = false;
		}
		if (GetEnvironmentVariable(L"SLEW_ARCHIVE_PATH", buffer, 32767))
			archivePath = QString::fromWCharArray(buffer);
	}
#else
	{
		QString temp;
		char *buffer;
		buffer = getenv("SLEW_GUI");
		if (buffer) {
			temp = buffer;
			if ((temp == "0") || (temp.toLower() == "false"))
				gui = false;
		}
		buffer = getenv("SLEW_ARCHIVE_PATH");
		if (buffer)
			archivePath = buffer;
	}
#endif
	
	if (gui) {
		//QGL::setPreferredPaintEngine(QPaintEngine::OpenGL);
		QPixmapCache::setCacheLimit(32768);
#ifdef Q_OS_MAC
		if (QSysInfo::MacintoshVersion > QSysInfo::MV_10_8)
			QFont::insertSubstitution(".Lucida Grande UI", "Lucida Grande");
#endif
		new Application(sArgc, sArgv);
#ifdef Q_OS_MAC
		bool forceFront = true;
		QString temp;
		char *buffer;
		buffer = getenv("SLEW_FORCE_FRONT");
		if (buffer) {
			temp = buffer;
			if ((temp == "0") || (temp.toLower() == "false"))
				forceFront = false;
		}
		if (forceFront)
			helper_bring_application_to_front();
#endif
	}
	else
		new QCoreApplication(sArgc, sArgv);
	
	sResourceReader = NULL;
	if ((archivePath.isEmpty()) && (dict)) {
		PyObject *file = PyDict_GetItemString(dict, "__file__");
		if (file) {
			archivePath = QDir(PyString_AS_STRING(file)).absolutePath();
			archivePath = QDir::toNativeSeparators(QDir::cleanPath(archivePath));
		}
	}
	
	if (!archivePath.isEmpty()) {
		for (;;) {
			int pos = archivePath.lastIndexOf(QDir::separator());
			if (pos == -1)
				break;
			
			if (ZipReader::initCheck(archivePath)) {
				sResourceReader = new ZipReader(archivePath);
				break;
			}
			else if (DirReader::initCheck(archivePath)) {
				sResourceReader = new DirReader(archivePath);
				break;
			}
			
			archivePath = archivePath.mid(0, pos);
		}
	}
	
	sTranslator.load(kQT_IT_QM_data, sizeof(kQT_IT_QM_data));
	qApp->installTranslator(&sTranslator);
	
	Py_AtExit(cleanup);
}



#include "slew.moc"
#include "slew_h.moc"
#include "objects_h.moc"
