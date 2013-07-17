#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QDesktopWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QBitmap>
#include <QCursor>
#include <QAbstractButton>
#include <QTimeLine>

#include "slew.h"

#include "systrayicon.h"


class Notification;


static int sVisibleCount = 0;
static QList<Notification *> sNotifications;

#define NOTIFICATION_MARGIN			8
#define NOTIFICATION_WIDTH			300
#define NOTIFICATION_BUTTON_SIZE	27
#define NOTIFICATION_SPACING		16



class NotificationButton : public QAbstractButton
{
	Q_OBJECT
	
public:
	NotificationButton(Notification *parent) : QAbstractButton((QWidget *)parent) { resize(NOTIFICATION_BUTTON_SIZE, NOTIFICATION_BUTTON_SIZE); }
	
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		QPen pen;
		
		painter.setRenderHints(QPainter::RenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing));
		
		for (int i = 0; i < 2; i++) {
			if (i == 0) {
				pen.setColor(Qt::black);
				pen.setWidth(5);
			}
			else {
				pen.setColor(Qt::white);
				pen.setWidth(3);
			}
			painter.setPen(pen);
			QRect rect = this->rect().adjusted(2,2,-2,-2);
			painter.drawEllipse(rect);
			rect = this->rect().adjusted(NOTIFICATION_BUTTON_SIZE / 3, NOTIFICATION_BUTTON_SIZE / 3, -NOTIFICATION_BUTTON_SIZE / 3 + 1, -NOTIFICATION_BUTTON_SIZE / 3 + 1);
			painter.drawLine(rect.topLeft(), rect.bottomRight());
			painter.drawLine(rect.topRight(), rect.bottomLeft());
		}
	}
};


class Notification : public QWidget
{
	Q_OBJECT
	
public:
	Notification(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon)
		: QWidget(NULL, Qt::SubWindow | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowStaysOnTopHint)
	{
#ifdef Q_WS_MAC
		helper_init_notification(this);
#endif
		QPalette palette = this->palette();
		palette.setColor(QPalette::WindowText, Qt::white);
		
		QVBoxLayout *vbox = new QVBoxLayout;
		QLabel *label = new QLabel(title);
		QFont font = label->font();
		font.setBold(true);
		label->setFont(font);
		label->setPalette(palette);
		vbox->addWidget(label);
		vbox->addSpacing(5);
		label = new QLabel(message);
		font = label->font();
		font.setPointSizeF(font.pointSizeF() * 0.9);
		label->setFont(font);
		label->setPalette(palette);
		vbox->addWidget(label, 1, Qt::AlignTop | Qt::AlignLeft);
		
		QLayout *layout;
		
		if (icon == QSystemTrayIcon::NoIcon) {
			vbox->setContentsMargins(20, 20, 20, 20);
			layout = vbox;
		}
		else {
			QStyle::StandardPixmap type;
			switch (icon) {
			case QSystemTrayIcon::Critical:		type = QStyle::SP_MessageBoxCritical; break;
			case QSystemTrayIcon::Warning:		type = QStyle::SP_MessageBoxWarning; break;
			case QSystemTrayIcon::Information:
			default:							type = QStyle::SP_MessageBoxInformation; break;
			}
			QHBoxLayout *hbox = new QHBoxLayout;
			hbox->setContentsMargins(20, 20, 20, 20);
			label = new QLabel;
			label->setPixmap(QApplication::style()->standardIcon(type).pixmap(QSize(32,32)));
			label->setAlignment(Qt::AlignTop);
			hbox->addWidget(label);
			hbox->addSpacing(10);
			hbox->addLayout(vbox, 1);
			layout = hbox;
		}
		
		setMinimumWidth(NOTIFICATION_WIDTH);
		setMaximumWidth(NOTIFICATION_WIDTH);
		setLayout(layout);
		
		QSize size = minimumSize();
		resize(size);
		
		setAttribute(Qt::WA_TranslucentBackground, true);
		
		fButton = new NotificationButton(this);
		fButton->move(NOTIFICATION_BUTTON_SIZE / 4 + 1, NOTIFICATION_BUTTON_SIZE / 4 + 1);
		fButton->hide();
		fUnderMouse = false;
		fClosed = false;
		
		connect(&fFader, SIGNAL(valueChanged(qreal)), this, SLOT(handleFading(qreal)));
		connect(&fFader, SIGNAL(finished()), this, SLOT(handleFinished()));
		fFader.setDuration(1000);
		
		connect(fButton, SIGNAL(clicked()), this, SLOT(handleClose()));
	}
	
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		QRect rect = this->rect().adjusted(2,2,-2,-2);
		
		painter.setRenderHints(QPainter::RenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing));
		
		if (fUnderMouse) {
			QPen pen(Qt::white);
			pen.setWidth(3);
			painter.setPen(pen);
		}
		
		painter.setBrush(QColor(0, 0, 0, 200));
		painter.drawRoundedRect(rect, 20, 20);
		
		fButton->setVisible(fUnderMouse);
	}
	
	virtual void timerEvent(QTimerEvent *event)
	{
		if (event->timerId() == fUpdateTimerID) {
			bool underMouse = frameGeometry().contains(QCursor::pos());
			if (underMouse != fUnderMouse) {
				if ((underMouse) && (!fClosed)) {
					fFader.stop();
					handleFading(1);
				}
				fUnderMouse = underMouse;
				repaint();
			}
		}
		else if (event->timerId() == fCloseTimerID) {
			killTimer(fCloseTimerID);
			fCloseTimerID = startTimer(30);
			if ((fUnderMouse) && (!fClosed)) {
				fFader.stop();
				handleFading(1);
			}
			else {
				if (fFader.state() != QTimeLine::Running) {
					fFader.setDirection(QTimeLine::Backward);
					fFader.start();
				}
			}
		}
	}
	
	void popup()
	{
		QRect rect = QApplication::desktop()->availableGeometry();
		int y = rect.top() + NOTIFICATION_MARGIN;
		
		foreach (Notification *n, sNotifications) {
			y = qMax(y, n->frameGeometry().bottom() + NOTIFICATION_SPACING);
		}
		move(rect.right() - NOTIFICATION_WIDTH - NOTIFICATION_MARGIN, y);
		sNotifications.append(this);
		
		fFader.start();
		fUpdateTimerID = startTimer(30);
		fCloseTimerID = startTimer(5000);
		setWindowOpacity(0);
		show();
	}
	
	static void showMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon)
	{
		if (!helper_notify_center(title, message)) {
			Notification *n = new Notification(title, message, icon);
			n->popup();
		}
	}

public slots:
	void handleFading(qreal value)
	{
		qreal opaque = style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0;
		setWindowOpacity(value * opaque);
	}
	
	void handleFinished()
	{
		if (fFader.direction() == QTimeLine::Backward) {
			deleteLater();
			sNotifications.removeAll(this);
		}
	}
	
	void handleClose()
	{
		fClosed = true;
		fFader.setDirection(QTimeLine::Backward);
		fFader.setDuration(300);
		fFader.start();
	}
	
private:
	NotificationButton	*fButton;
	int					fUpdateTimerID;
	int					fCloseTimerID;
	bool				fUnderMouse;
	bool				fClosed;
	QTimeLine			fFader;
};



SystrayIcon_Impl::SystrayIcon_Impl()
	: QSystemTrayIcon(), WidgetInterface()
{
	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(handleActivated(QSystemTrayIcon::ActivationReason)));
}


void
SystrayIcon_Impl::setVisible(bool visible)
{
	if (visible)
		sVisibleCount++;
	else
		sVisibleCount--;
	
	qApp->setQuitOnLastWindowClosed(sVisibleCount == 0);
	
	QSystemTrayIcon::setVisible(visible);
}


void
SystrayIcon_Impl::handleActivated(QSystemTrayIcon::ActivationReason reason)
{
	if (reason == QSystemTrayIcon::Trigger) {
		EventRunner(this, "onActivate").run();
	}
	else if (reason == QSystemTrayIcon::DoubleClick) {
		EventRunner(this, "onDblClick").run();
	}
}


SL_DEFINE_METHOD(SystrayIcon, set_visible, {
	bool visible;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &visible))
		return NULL;
	
	impl->setVisible(visible);
})


#ifdef Q_WS_MAC
#define SHOW_MESSAGE	Notification::showMessage
#else
#define SHOW_MESSAGE	impl->showMessage
#endif

SL_DEFINE_METHOD(SystrayIcon, popup_message, {
	QString message, title;
	int icon;
	QSystemTrayIcon::MessageIcon micon = QSystemTrayIcon::NoIcon;
	
	if (!PyArg_ParseTuple(args, "O&O&i", convertString, &message, convertString, &title, &icon))
		return NULL;
	
	switch (icon) {
	case SL_ICON_ERROR:			micon = QSystemTrayIcon::Critical; break;
	case SL_ICON_WARNING:		micon = QSystemTrayIcon::Warning; break;
	case SL_ICON_INFORMATION:	micon = QSystemTrayIcon::Information; break;
	}
	
	SHOW_MESSAGE(title, message, micon);
})


SL_DEFINE_METHOD(SystrayIcon, get_tip, {
	return createStringObject(impl->toolTip());
})


SL_DEFINE_METHOD(SystrayIcon, set_tip, {
	QString tip;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &tip))
		return NULL;
	
	impl->setToolTip(tip);
})


SL_DEFINE_METHOD(SystrayIcon, set_menu, {
	PyObject *object;
	QMenu *menu;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		impl->setContextMenu(NULL);
	}
	else {
		menu = (QMenu *)getImpl(object);
		if (!menu)
			return NULL;
		impl->setContextMenu(menu);
	}
})


SL_DEFINE_METHOD(SystrayIcon, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
})



SL_START_PROXY_DERIVED(SystrayIcon, Widget)
SL_METHOD(popup_message)
SL_METHOD(set_visible)
SL_METHOD(set_menu)
SL_METHOD(set_icon)
SL_PROPERTY(tip)
SL_END_PROXY_DERIVED(SystrayIcon, Widget)


#include "systrayicon.moc"
#include "systrayicon_h.moc"

