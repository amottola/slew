#include "slew.h"

#include "toolbutton.h"

#include <QMenu>


ToolButton_Impl::ToolButton_Impl()
	: QToolButton(), WidgetInterface(), fFlat(false)
{
	setPopupMode(InstantPopup);
	setAutoRaise(true);
	
	connect(this, SIGNAL(clicked()), this, SLOT(handleClicked()), Qt::QueuedConnection);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(handleToggled(bool)));
	connect(SL_QAPP(), SIGNAL(endDrag(QObject *)), this, SLOT(handleEndDrag(QObject *)));
}


bool
ToolButton_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Space:
			case Qt::Key_Select:
				return true;
			default:
				break;
			}
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
		return true;
	default:
		break;
	}
	return false;
}


void
ToolButton_Impl::handleClicked()
{
	EventRunner(this, "onClick").run();
}


void
ToolButton_Impl::handleToggled(bool toggled)
{
	EventRunner runner(this, "onCheck");
	if (runner.isValid()) {
		runner.set("value", toggled);
		runner.run();
	}
}


void
ToolButton_Impl::handleEndDrag(QObject *object)
{
	if ((object == this) && (!isCheckable()))
		setDown(false);
}


void
ToolButton_Impl::paintEvent(QPaintEvent *event)
{
	QIcon icon = this->icon();
	
	if ((isFlat()) && (!icon.isNull())) {
		QPainter painter(this);
		QIcon::Mode mode;
		
		if (!isEnabled())
			mode = QIcon::Disabled;
		else if ((isDown()) || (isChecked()))
			mode = QIcon::Selected;
		else if (underMouse())
			mode = QIcon::Active;
		else
			mode = QIcon::Normal;
		
		QPixmap pixmap = icon.pixmap(size(), mode, QIcon::Off);
		painter.drawPixmap((size().width() - pixmap.size().width()) / 2, (size().height() - pixmap.size().height()) / 2, pixmap);
	}
	else {
		QToolButton::paintEvent(event);
	}
}


SL_DEFINE_METHOD(ToolButton, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isCheckable())
		style |= SL_BUTTON_STYLE_TOGGLE;
	if (impl->isFlat())
		style |= SL_BUTTON_STYLE_FLAT;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(ToolButton, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setCheckable(style & SL_BUTTON_STYLE_TOGGLE ? true : false);
	impl->setFlat(style & SL_BUTTON_STYLE_FLAT ? true : false);
})


SL_DEFINE_METHOD(ToolButton, is_default, {
	Py_RETURN_FALSE;
})


SL_DEFINE_METHOD(ToolButton, set_default, {
})


SL_DEFINE_METHOD(ToolButton, get_menu, {
	QMenu *menu = impl->menu();
	if (!menu)
		Py_RETURN_NONE;
	
	PyObject *object = getObject(menu);
	if (!object) {
		PyErr_Clear();
		Py_RETURN_NONE;
	}
	return object;
})


SL_DEFINE_METHOD(ToolButton, set_menu, {
	PyObject *object;
	QMenu *menu;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		impl->setMenu(NULL);
	}
	else {
		menu = (QMenu *)getImpl(object);
		if (!menu)
			return NULL;
		impl->setMenu(menu);
	}
})


SL_START_PROXY_DERIVED(ToolButton, Button)
SL_PROPERTY(style)
SL_BOOL_PROPERTY(default)
SL_PROPERTY(menu)
SL_END_PROXY_DERIVED(ToolButton, Button)


#include "toolbutton.moc"
#include "toolbutton_h.moc"

