#include "slew.h"

#include "checkbox.h"

#include <QKeyEvent>
#include <QMouseEvent>


CheckBox_Impl::CheckBox_Impl()
	: QCheckBox(), WidgetInterface()
{
	connect(this, SIGNAL(toggled(bool)), this, SLOT(handleToggled(bool)));
}


bool
CheckBox_Impl::isModifyEvent(QEvent *event)
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
		{
			QMouseEvent *e = (QMouseEvent *)event;
			return hitButton(e->pos());
		}
		break;
	default:
		break;
	}
	return false;
}


void
CheckBox_Impl::handleToggled(bool toggled)
{
	EventRunner runner(this, "onCheck");
	if (runner.isValid()) {
		runner.set("value", toggled);
		runner.run();
	}
}


SL_DEFINE_METHOD(CheckBox, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	if (!impl->isCheckable())
		style |= SL_CHECKBOX_STYLE_READONLY;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(CheckBox, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	impl->setCheckable(style & SL_CHECKBOX_STYLE_READONLY ? false : true);
})


SL_DEFINE_METHOD(CheckBox, get_label, {
	return createStringObject(impl->text());
})


SL_DEFINE_METHOD(CheckBox, set_label, {
	QString label;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &label))
		return NULL;
	
	impl->setText(label);
})


SL_DEFINE_METHOD(CheckBox, is_checked, {
	return createBoolObject(impl->isChecked());
})


SL_DEFINE_METHOD(CheckBox, set_checked, {
	bool checked;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &checked))
		return NULL;
	
	impl->setChecked(checked);
})



SL_START_PROXY_DERIVED(CheckBox, Window)
SL_PROPERTY(style)
SL_PROPERTY(label)
SL_BOOL_PROPERTY(checked)
SL_END_PROXY_DERIVED(CheckBox, Window)


#include "checkbox.moc"
#include "checkbox_h.moc"

