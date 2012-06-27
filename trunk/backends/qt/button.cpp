#include "slew.h"

#include "button.h"

#include <QMenu>
#include <QKeyEvent>


Button_Impl::Button_Impl()
	: QPushButton(), WidgetInterface()
{
#ifdef Q_WS_MAC
	setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));
#endif
	
	connect(this, SIGNAL(clicked()), this, SLOT(handleClicked()), Qt::QueuedConnection);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(handleToggled(bool)));
}


bool
Button_Impl::isModifyEvent(QEvent *event)
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
				return isEnabled();
			default:
				break;
			}
		}
		break;
	case QEvent::MouseButtonRelease:
		return isEnabled();
	default:
		break;
	}
	return false;
}


void
Button_Impl::handleClicked()
{
	EventRunner(this, "onClick").run();
}


void
Button_Impl::handleToggled(bool toggled)
{
	EventRunner runner(this, "onCheck");
	if (runner.isValid()) {
		runner.set("value", toggled);
		runner.run();
	}
}


SL_DEFINE_METHOD(Button, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isCheckable())
		style |= SL_BUTTON_STYLE_TOGGLE;
	if (impl->isFlat())
		style |= SL_BUTTON_STYLE_FLAT;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Button, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setCheckable(style & SL_BUTTON_STYLE_TOGGLE ? true : false);
	impl->setFlat(style & SL_BUTTON_STYLE_FLAT ? true : false);
})


SL_DEFINE_METHOD(Button, get_color, {
	QColor color = impl->palette().buttonText().color();
	if (color == QApplication::palette(impl).buttonText().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(Button, set_color, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(impl->isEnabled() ? QPalette::Normal : QPalette::Disabled, QPalette::ButtonText);
	}
	
	palette.setColor(QPalette::ButtonText, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(Button, get_bgcolor, {
	QColor color = impl->palette().button().color();
	if (color == QApplication::palette(impl).button().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(Button, set_bgcolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		impl->setAutoFillBackground(false);
		color = QApplication::palette(impl).color(QPalette::Button);
	}
	else {
		impl->setAutoFillBackground(true);
	}
	
	palette.setColor(QPalette::Button, color);
	impl->setPalette(palette);
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, get_label, {
	return createStringObject(impl->text());
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, set_label, {
	QString label;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &label))
		return NULL;
	
	impl->setText(label);
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, is_toggled, {
	return createBoolObject(impl->isChecked());
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, set_toggled, {
	bool checked;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &checked))
		return NULL;
	
	impl->setCheckable(true);
	impl->setChecked(checked);
})


SL_DEFINE_METHOD(Button, is_default, {
	return createBoolObject(impl->isDefault());
})


SL_DEFINE_METHOD(Button, set_default, {
	bool def;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &def))
		return NULL;
	
	impl->setAutoDefault(false);
	impl->setDefault(def);
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_ABSTRACT_METHOD(Button, QAbstractButton, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
	QList<QSize> sizes = icon.availableSizes();
	if (!sizes.isEmpty())
		impl->setIconSize(sizes.first());
})


SL_DEFINE_METHOD(Button, get_menu, {
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


SL_DEFINE_METHOD(Button, set_menu, {
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


SL_START_PROXY_DERIVED(Button, Window)
SL_PROPERTY(style)
SL_PROPERTY(color)
SL_PROPERTY(bgcolor)
SL_PROPERTY(label)
SL_BOOL_PROPERTY(toggled)
SL_BOOL_PROPERTY(default)
SL_PROPERTY(icon)
SL_PROPERTY(menu)
SL_END_PROXY_DERIVED(Button, Window)


#include "button.moc"
#include "button_h.moc"

