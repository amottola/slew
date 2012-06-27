#include "slew.h"

#include "spinfield.h"

#include <QKeyEvent>


SpinField_Impl::SpinField_Impl()
	: QSpinBox(), WidgetInterface()
{
	connect(this, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
}


bool
SpinField_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				return false;
			}
			return true;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::Wheel:
		return true;
	default:
		break;
	}
	return false;
}


void
SpinField_Impl::handleValueChanged(int value)
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", value);
		runner.run();
	}
}


SL_DEFINE_METHOD(SpinField, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->wrapping())
		style |= SL_SPINFIELD_STYLE_WRAP;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(SpinField, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setWrapping(style & SL_SPINFIELD_STYLE_WRAP ? true : false);
})


SL_DEFINE_METHOD(SpinField, get_min, {
	return PyInt_FromLong(impl->minimum());
})


SL_DEFINE_METHOD(SpinField, set_min, {
	int min;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &min))
		return NULL;
	
	impl->setMinimum(min);
})


SL_DEFINE_METHOD(SpinField, get_max, {
	return PyInt_FromLong(impl->maximum());
})


SL_DEFINE_METHOD(SpinField, set_max, {
	int max;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &max))
		return NULL;
	
	impl->setMaximum(max);
})


SL_DEFINE_METHOD(SpinField, get_value, {
	return PyInt_FromLong(impl->value());
})


SL_DEFINE_METHOD(SpinField, set_value, {
	int value;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &value))
		return NULL;
	
	impl->setValue(value);
})



SL_START_PROXY_DERIVED(SpinField, Ranged)
SL_PROPERTY(style)
SL_PROPERTY(min)
SL_PROPERTY(max)
SL_PROPERTY(value)
SL_END_PROXY_DERIVED(SpinField, Ranged)


#include "spinfield.moc"
#include "spinfield_h.moc"

