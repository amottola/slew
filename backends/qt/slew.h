#ifndef __slew_h__
#define __slew_h__

#include <QDebug>
#include <QCoreApplication>
#include <QApplication>
#include <QModelIndex>
#include <QItemSelection>
#include <QActionGroup>
#include <QRegExp>
#include <QRegExpValidator>
#include <QLineEdit>
#include <QKeySequence>
#include <QLatin1Char>
#include <QScrollBar>
#include <QHeaderView>

#include <QMimeData>
#include <QMargins>
#include <QPoint>
#include <QSize>
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QIcon>
#include <QPainter>
#include <QStyleOptionViewItem>

#include <QList>
#include <QHash>
#include <QString>
#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QThread>
#include <QMutex>

class QWidget;
class QAbstractButton;
class QButtonGroup;
class QMainWindow;
class QClipboard;
class QRadioButton;
class QAbstractItemView;
class QAbstractItemModel;


#ifdef Q_OS_LINUX
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#endif

#include "Python.h"
#include "structmember.h"
#include "marshal.h"

#include "proxy.h"
#include "constants/__init__.h"
#include "constants/widget.h"

#if defined(__GNUC__) || defined(__clang__)
	#define EXPORT									__attribute__((__visibility__("default")))
#else
	#define EXPORT
#endif


#define SL_QAPP()									((Application *)qApp)

#define SL_PYOBJECT_MIME_TYPE						"application/x-slew-object"

#define SL_RETURN_CANNOT_ATTACH						{ PyErr_Format(PyExc_RuntimeError, "cannot attach widget"); return NULL; }
#define SL_RETURN_CANNOT_DETACH						{ PyErr_Format(PyExc_RuntimeError, "cannot detach widget"); return NULL; }
#define SL_RETURN_NO_IMPL							{ PyErr_Format(PyExc_RuntimeError, "object has no attached implementation"); return NULL; }
#define SL_RETURN_DUP_SIZER							{ PyErr_Format(PyExc_RuntimeError, "widget already has an attached sizer"); return NULL; }

#define SL_ACCEL_KEY(k)								(QKeySequence(k).toString().isEmpty() ? QString() : QLatin1Char('\t') + QKeySequence(k).toString())


#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#define QT_SET_SECTION_RESIZE_MODE		setSectionResizeMode
#define QT_SECTION_RESIZE_MODE			sectionResizeMode
#define QT_SET_SECTIONS_CLICKABLE		setSectionsClickable
#define QT_SET_SECTIONS_MOVABLE			setSectionsMovable
#else
#define QT_SET_SECTION_RESIZE_MODE		setResizeMode
#define QT_SECTION_RESIZE_MODE			resizeMode
#define QT_SET_SECTIONS_CLICKABLE		setClickable
#define QT_SET_SECTIONS_MOVABLE			setMovable
#endif


struct Abstract_Proxy;
struct Widget_Proxy;
struct Window_Proxy;
struct DC_Proxy;
struct FormatInfo;

class Widget_Impl;
class Sizer_Impl;
class DataModel_Impl;
class Completer;
class TimedCall;


int convertBuffer(PyObject *object, QByteArray *value);
int convertPoint(PyObject *object, QPoint *value);
int convertPointF(PyObject *object, QPointF *value);
int convertSize(PyObject *object, QSize *value);
int convertSizeF(PyObject *object, QSizeF *value);
int convertBool(PyObject *object, bool *value);
int convertInt(PyObject *object, int *value);
int convertIntList(PyObject *object, QList<int> *value);
int convertString(PyObject *object, QString *value);
int convertColor(PyObject *object, QColor *value);
int convertFont(PyObject *object, QFont *value);
int convertPixmap(PyObject *object, QPixmap *value);
int convertPicture(PyObject *object, QPicture *value);
int convertIcon(PyObject *object, QIcon *value);
int convertDate(PyObject *object, QDate *value);
int convertDateTime(PyObject *object, QDateTime *value);
int convertTransform(PyObject *object, QTransform *value);

bool getObjectAttr(PyObject *object, const char *name, QByteArray *value);
bool getObjectAttr(PyObject *object, const char *name, QPoint *value);
bool getObjectAttr(PyObject *object, const char *name, QPointF *value);
bool getObjectAttr(PyObject *object, const char *name, QSize *value);
bool getObjectAttr(PyObject *object, const char *name, QSizeF *value);
bool getObjectAttr(PyObject *object, const char *name, bool *value);
bool getObjectAttr(PyObject *object, const char *name, int *value);
bool getObjectAttr(PyObject *object, const char *name, short *value);
bool getObjectAttr(PyObject *object, const char *name, double *value);
bool getObjectAttr(PyObject *object, const char *name, QString *value);
bool getObjectAttr(PyObject *object, const char *name, QColor *value);
bool getObjectAttr(PyObject *object, const char *name, QFont *value);
bool getObjectAttr(PyObject *object, const char *name, QPixmap *value);
bool getObjectAttr(PyObject *object, const char *name, QPicture *value);
bool getObjectAttr(PyObject *object, const char *name, QIcon *value);
bool getObjectAttr(PyObject *object, const char *name, QDate *value);
bool getObjectAttr(PyObject *object, const char *name, QDateTime *value);

PyObject *createBufferObject(const QByteArray& data);
PyObject *createVectorObject(const QPoint& point);
PyObject *createVectorObject(const QPointF& point);
PyObject *createVectorObject(const QSize& size);
PyObject *createVectorObject(const QSizeF& size);
PyObject *createBoolObject(bool boolean);
PyObject *createIntListObject(const QList<int>& list);
PyObject *createStringObject(const QString& string);
PyObject *createColorObject(const QColor& color);
PyObject *createFontObject(const QFont& font);
PyObject *createDCObject(QPainter *painter, PyObject *objectType=NULL, PyObject *proxyType=NULL, QPaintDevice *device=NULL, QTransform *baseTransform=NULL);
PyObject *createBitmapObject(const QPixmap& pixmap);
PyObject *createPictureObject(const QPicture& picture);
PyObject *createIconObject(const QIcon& icon);
PyObject *createDateObject(const QDate& date);
PyObject *createDateTimeObject(const QDateTime& dateTime);
PyObject *createTransformObject(const QTransform& transform);


Abstract_Proxy *getProxy(PyObject *object);
Abstract_Proxy *getProxy(QObject *object);
Widget_Proxy *getSafeProxy(QObject *object);

PyObject *getObject(QObject *object, bool incref=true);
PyObject *getDataModel(QAbstractItemModel *model);
QObject *getImpl(PyObject *object);

QString normalizeFormat(const QHash<QString, QString>& vars, const QString& format);
void centerWindow(QWidget *window, QWidget *parent = NULL);
void setShortcut(QWidget *widget, const QString& sequence, Qt::ShortcutContext context, PyObject *callback);
void setTimeout(QObject *parent, int delay, PyObject *func, PyObject *args);
bool loadResource(const QString& resource, QByteArray& data);
bool openURI(const QString& uri);

void grabMouse(QWidget *window, bool grab);
bool messageBox(QWidget *window, const QString& title, const QString& message, PyObject *buttons, int icon, int modality, PyObject *callback, PyObject *userdata, int *button);
int showPopupMessage(QWidget *parent, QWidget *editor, const QString& text, const QRect& rect, const QPoint& hotspot, int where = SL_TOP, int buttons = 0);
void hidePopupMessage(QEvent *event = NULL);
bool isPopupMessageShown();

PyObject *mimeDataToObject(const QMimeData *mimeData, const QString& mimeType="");
bool objectToMimeData(PyObject *object, QMimeData *mimeData);

void relinkActions(QWidget *widget);

int getKeyModifiers(Qt::KeyboardModifiers modifiers);
int getKeyCode(int code);

QLocale getLocale();
void parseFormat(const QString& format, int dataType, FormatInfo *formatInfo, QString *humanFormat = NULL, QRegExp *regExp = NULL);
QString getFormattedValue(const QString& value, QColor *color, Qt::Alignment *align, int dataType, FormatInfo *formatInfo, bool editMode = false);

Qt::Alignment fromAlign(int align);
int toAlign(Qt::Alignment align);

void setWindowStyle(QWidget *window, int style);
void getWindowStyle(QWidget *window, int& style);
PyObject *insertWindowChild(QWidget *window, PyObject *child);
PyObject *removeWindowChild(QWidget *window, PyObject *child);

bool setViewSelection(QAbstractItemView *view, PyObject *selection);
PyObject *getViewSelection(QAbstractItemView *view);

bool pageSetup(PyObject *settings, PyObject *parent, bool *accepted);
bool printSetup(PyObject *settings, PyObject *parent, bool *accepted);
PyObject *printDocument(int type, const QString& title, PyObject *callback, bool prompt, PyObject *settings, PyObject *parent, QObject *handler = NULL);

void freeBitmapResources(QPainter *painter, QPaintDevice *device);

#ifdef Q_OS_MAC

void helper_bring_application_to_front();
void helper_set_resizeable(QWidget *widget, bool enabled);
void helper_init_notification(QWidget *widget);
void helper_clear_menu_previous_action(QMenu *menu);
bool helper_notify_center(const QString &title, const QString &text);
void helper_set_dock_menu(QMenu *menu);

#elif defined(Q_OS_LINUX)
void helper_set_resizeable(QWidget *widget, bool enabled);

#endif


// Q_DECLARE_METATYPE(SL_Widget *)
// Q_DECLARE_METATYPE(SL_DataItem *)
// Q_DECLARE_METATYPE(SL_DataIndex *);
Q_DECLARE_METATYPE(QList<QActionGroup *> *)
Q_DECLARE_METATYPE(PyObject *)
Q_DECLARE_METATYPE(QModelIndex)
Q_DECLARE_METATYPE(QItemSelection)
Q_DECLARE_METATYPE(Qt::SortOrder)
Q_DECLARE_METATYPE(Qt::Alignment)
Q_DECLARE_METATYPE(QMargins)
Q_DECLARE_METATYPE(QObject *)
Q_DECLARE_METATYPE(QEvent *)


SL_DECLARE_ABSTRACT_PROXY(Widget)
SL_DECLARE_ABSTRACT_PROXY(Window)

SL_DECLARE_PROXY(Frame)
SL_DECLARE_PROXY(Dialog)
SL_DECLARE_PROXY(PopupWindow)
SL_DECLARE_PROXY(SplashScreen)
SL_DECLARE_PROXY(MenuBar)
SL_DECLARE_PROXY(Menu)
SL_DECLARE_PROXY(MenuItem)
SL_DECLARE_PROXY(StatusBar)
SL_DECLARE_PROXY(ToolBar)
SL_DECLARE_PROXY(ToolBarItem)
SL_DECLARE_PROXY(Sizer)
SL_DECLARE_PROXY(Panel)
SL_DECLARE_PROXY(FoldPanel)
SL_DECLARE_PROXY(ScrollView)
SL_DECLARE_PROXY(SplitView)
SL_DECLARE_PROXY(TabView)
SL_DECLARE_PROXY(TabViewPage)
SL_DECLARE_PROXY(StackView)
SL_DECLARE_PROXY(Label)
SL_DECLARE_PROXY(Button)
SL_DECLARE_PROXY(ToolButton)
SL_DECLARE_PROXY(GroupBox)
SL_DECLARE_PROXY(CheckBox)
SL_DECLARE_PROXY(Radio)
SL_DECLARE_PROXY(Line)
SL_DECLARE_PROXY(Image)
SL_DECLARE_ABSTRACT_PROXY(Ranged)
SL_DECLARE_PROXY(Slider)
SL_DECLARE_PROXY(SpinField)
SL_DECLARE_PROXY(Progress)
SL_DECLARE_PROXY(TextField)
SL_DECLARE_PROXY(TextView)
SL_DECLARE_PROXY(SearchField)
SL_DECLARE_PROXY(Calendar)
SL_DECLARE_PROXY(ComboBox)
SL_DECLARE_PROXY(ListView)
SL_DECLARE_PROXY(Grid)
SL_DECLARE_PROXY(TreeView)
SL_DECLARE_PROXY(SceneItem)
SL_DECLARE_PROXY(SceneView)
SL_DECLARE_PROXY(Wizard)
SL_DECLARE_PROXY(WizardPage)
SL_DECLARE_PROXY(SystrayIcon)
SL_DECLARE_PROXY(WebView)



class PyAutoLocker
{
public:
	PyAutoLocker() { fIsValid = Py_IsInitialized(); if (fIsValid) { fState = PyGILState_Ensure(); PyErr_Clear(); } }
	~PyAutoLocker() { if (fIsValid) PyGILState_Release(fState); }
	
	bool isValid() { return fIsValid; }
	
private:
	PyGILState_STATE	fState;
	bool				fIsValid;
};



class EventRunner
{
public:
	EventRunner(Widget_Proxy *proxy, const QString& name = "")
		: fProxy(proxy), fName(name), fEvent(NULL), fParams(NULL) { if (fLocker.isValid()) { Py_XINCREF(proxy); fParams = PyDict_New(); } }
	EventRunner(QObject *object, const QString& name = "")
		: fProxy(NULL), fName(name), fEvent(NULL), fParams(NULL) { if (fLocker.isValid()) { fProxy = getSafeProxy(object); Py_XINCREF(fProxy); fParams = PyDict_New(); } }
	~EventRunner() { Py_XDECREF(fParams); Py_XDECREF(fEvent); Py_XDECREF(fProxy); }
	
	void setName(const QString& name) { fName = name; }
	QString name() { return fName; }
	
	QWidget *widget() { return fProxy ? (QWidget *)fProxy->fImpl : NULL; }
	bool isValid() { return (fLocker.isValid()) && (fProxy != NULL); }
	
	void set(const char *param, PyObject *value, bool decref=true) { PyDict_SetItemString(fParams, param, value); if (decref) Py_DECREF(value); }
	void set(const char *param, bool value) { set(param, createBoolObject(value)); }
	void set(const char *param, int value) { set(param, PyInt_FromLong(value)); }
	void set(const char *param, double value) { set(param, PyFloat_FromDouble(value)); }
	void set(const char *param, const QString& value) { set(param, createStringObject(value)); }
	void set(const char *param, const QPoint& value) { set(param, createVectorObject(value)); }
	void set(const char *param, const QPointF& value) { set(param, createVectorObject(value)); }
	void set(const char *param, const QSize& value) { set(param, createVectorObject(value)); }
	void set(const char *param, const QSizeF& value) { set(param, createVectorObject(value)); }
	void set(const char *param, const QColor& value) { set(param, createColorObject(value)); }
	void set(const char *param, const QFont& value) { set(param, createFontObject(value)); }
	void set(const char *param, const QByteArray& value) { set(param, createBufferObject(value)); }
	void set(const char *param, const QDate& value) { set(param, createDateObject(value)); }
	void set(const char *param, const QDateTime& value) { set(param, createDateTimeObject(value)); }
	void set(const char *param, const QPixmap& value) { set(param, createBitmapObject(value)); }
	void set(const char *param, const QIcon& value) { set(param, createIconObject(value)); }
	
	bool get(const char *param, PyObject **value) { if (!fEvent) return false; PyObject *o = PyObject_GetAttrString(fEvent, param); if (o) { Py_DECREF(o); *value = o; return true; } PyErr_Clear(); return false; }
	bool get(const char *param, bool *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, int *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, double *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QString *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QPoint *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QSize *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QColor *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QFont *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QByteArray *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QDate *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QDateTime *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QPixmap *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	bool get(const char *param, QIcon *value) { return fEvent ? getObjectAttr(fEvent, param, value) : false; }
	
	bool run();
	
private:
	PyAutoLocker					fLocker;
	Widget_Proxy					*fProxy;
	QString							fName;
	PyObject						*fEvent;
	PyObject						*fParams;
};



class WidgetInterface
{
public:
	WidgetInterface() {}
	virtual ~WidgetInterface() {}
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool canFocusOut(QWidget *oldFocus, QWidget *newFocus);
	
	virtual bool isModifyEvent(QEvent *event) { return false; }
	virtual bool canModify(QWidget *widget);
};



class Widget_Impl : public QObject, public WidgetInterface
{
	Q_OBJECT
};



class Window_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT
};



class Application : public QApplication
{
	Q_OBJECT
	
public:
	Application(int& argc, char **argv);
	virtual ~Application();
	
	QMainWindow *shadowWindow() { return fShadowWindow; }
	
	void lock() { fMutex->lock(); }
	void unlock() { fMutex->unlock(); }
	QMutex *getLock() { return fMutex; }
	
	virtual bool notify(QObject *receiver, QEvent *event);
	virtual bool eventFilter(QObject *obj, QEvent *event);
	
	PyObject *getProxy(QObject *object);
	
	void registerObject(QObject *object, Abstract_Proxy *proxy);
	void unregisterObject(QObject *object);
	
	void newProxy(Abstract_Proxy *proxy);
	void deallocProxy(Abstract_Proxy *proxy);
	QObject *replaceProxyObject(Abstract_Proxy *proxy, QObject *object);

	bool canQuit();
	
public slots:
	void sendTabEvent(QObject *receiver);
	void commitData(QSessionManager& manager);

private:
	QMainWindow						*fShadowWindow;
	QMutex							*fMutex;
	QHash<QString, PyObject *>		fWidgets;
	qulonglong						fWID;
};



typedef struct FormatInfo
{
	int				fFlags;
	Qt::Alignment	fAlign;
	int				fLen;
	int				fDecLen;
	QString			fDTFormat;
	QChar			fMonSymbol;
} FormatInfo;



class FormattedLineEdit : public QLineEdit
{
	Q_OBJECT

public:
	enum State { Invalid, Intermediate, Acceptable };

	FormattedLineEdit(QWidget *parent = NULL);
	~FormattedLineEdit();
	
	virtual QSize minimumSizeHint() const { return QLineEdit::minimumSizeHint().expandedTo(QSize(0, 20)); }
	
	void setFormat(const QString& format);
	QString format() { return fFormat; }
	QString humanFormat() { return fHumanFormat; }
	
	virtual void setDataType(int dataType);
	virtual int dataType() { return fDataType; }
	
	Qt::Alignment alignment() { return fAlign; }
	QColor color() { return fColor; }
	QString value() { return fIntValue; }
	QString value(bool controlText) { return controlText ? QLineEdit::text() : fIntValue; }
	void setInternalValue(const QString& value);
	
	State state() { return fState; }
	
	void setSelectedOnFocus(bool select) { fSelectedOnFocus = select; }
	bool isSelectedOnFocus() { return fSelectedOnFocus; }
	
	void setCapsOnly(bool on) { fCapsOnly = on; }
	bool isCapsOnly() { return fCapsOnly; }
	
	void setEnterTabs(bool on) { fEnterTabs = on; }
	bool isEnterTabs() { return fEnterTabs; }
	
	void setEmptyText(const QString& text) { fEmptyText = text; update(); }
	QString emptyText() { return fEmptyText; }
	
	void setCompleter(DataModel_Impl *model, int column, const QColor& color, const QColor& bgcolor, const QColor& hicolor, const QColor& hibgcolor);
	
	virtual QAbstractButton *createIconButton(const QIcon& icon);
	void setIcon(const QIcon& icon);
	QIcon icon();
	
	QRegExpValidator *internalValidator() { return fValidator; }
	bool isValidText(const QString& text);
	bool isValidInput(QKeyEvent *event, QString *output = NULL);
	bool isValid() { return fState == Acceptable; }
	
	bool canCut();
	bool canPaste();
	virtual bool canModify(QWidget *widget) { return true; }

	void handlePasteClipboard();
	virtual bool modifyClipboardOnPaste(QString& text) { return false; }
	
	void complete() { emit doComplete(); }
	
signals:
	void textModified(const QString& text, int completion=-1);
	void iconClicked();
	void contextMenu();
	void doComplete();
	
public slots:
	void setText(const QString& value);
	void setColor(const QColor& color);
	void setAlignment(Qt::Alignment align);
	void handleComplete(int index);
	void handleUndo();
	void handleRedo();
	void handleCut();
	void handlePaste();
	void handleDelete();
	
protected:
	void setState(State state);
	void setInternalValueFromEditValue(const QString& value);
	void updateDisplay(bool editMode);
	
	QMenu *createContextMenu();
	
	virtual void resizeEvent(QResizeEvent *event);
	virtual void changeEvent(QEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void focusInEvent(QFocusEvent *event);
	virtual void focusOutEvent(QFocusEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void dropEvent(QDropEvent *event);
	virtual void paintEvent(QPaintEvent *event);

protected:
	State						fState;
	QString						fFormat;
	QString						fHumanFormat;
	QString						fIntValue;
	int							fDataType;
	Qt::Alignment				fAlign;
	QColor						fColor;
	FormatInfo					fFormatInfo[2];
	QRegExp						fRegExp;
	bool						fSelectedOnFocus;
	bool						fCapsOnly;
	bool						fEnterTabs;
	QString						fEmptyText;
	QRegExpValidator			*fValidator;
	QAbstractButton				*fIcon;
	Completer					*fCompleter;
	int							fCursorPosition;
	int							fCursorEndPosition;
};



class Completer : public QObject
{
	Q_OBJECT
	
public:
	Completer(FormattedLineEdit *parent, DataModel_Impl *model, int column, const QColor& color, const QColor& bgcolor, const QColor& hicolor, const QColor& hibgcolor);
	~Completer();
	
	FormattedLineEdit *lineEdit() { return fLineEdit; }
	DataModel_Impl *model() { return fModel; }
	int column() { return fColumn; }
	QColor color() { return fColor; }
	QColor bgcolor() { return fBGColor; }
	QColor hicolor() { return fHIColor; }
	QColor hibgcolor() { return fHIBGColor; }
	
	static bool eatFocus();
	static bool isRunningOn(QWidget *widget);
	static bool underMouse();
	static void hide();
	static void complete();
	static int completion();

signals:
	void complete(int index);

public slots:
	void handleComplete();
	
protected:
	bool eventFilter(QObject *obj, QEvent *event);
	
private slots:
	void completionActivated(bool modified = true);
	void completionSelected(const QItemSelection& selection);

private:
	FormattedLineEdit				*fLineEdit;
	DataModel_Impl					*fModel;
	int								fColumn;
	QColor							fColor;
	QColor							fBGColor;
	QColor							fHIColor;
	QColor							fHIBGColor;
	QString							fBaseString;
};



#endif
