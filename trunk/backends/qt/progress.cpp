#include "slew.h"

#include "progress.h"


Progress_Impl::Progress_Impl()
	: QProgressBar(), WidgetInterface()
{
	fMinimum = minimum();
	fMaximum = maximum();
	setIndeterminate((fMinimum == 0) && (fMaximum == 0));
	setTextVisible(false);
}


void
Progress_Impl::setIndeterminate(bool indeterminate)
{
	if (indeterminate) {
		QProgressBar::setMinimum(0);
		QProgressBar::setMaximum(0);
	}
	else {
		QProgressBar::setMinimum(fMinimum);
		QProgressBar::setMaximum(fMaximum);
		QProgressBar::setValue(fValue);
	}
}


SL_DEFINE_METHOD(Progress, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if ((impl->minimum() == 0) && (impl->maximum() == 0))
		style |= SL_PROGRESS_STYLE_INDETERMINATE;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Progress, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setIndeterminate(style & SL_PROGRESS_STYLE_INDETERMINATE ? true : false);
})


SL_DEFINE_METHOD(Progress, get_min, {
	return PyInt_FromLong(impl->minimum());
})


SL_DEFINE_METHOD(Progress, set_min, {
	int min;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &min))
		return NULL;
	
	impl->setMinimum(min);
})


SL_DEFINE_METHOD(Progress, get_max, {
	return PyInt_FromLong(impl->maximum());
})


SL_DEFINE_METHOD(Progress, set_max, {
	int max;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &max))
		return NULL;
	
	impl->setMaximum(max);
})


SL_DEFINE_METHOD(Progress, get_value, {
	return PyInt_FromLong(impl->value());
})


SL_DEFINE_METHOD(Progress, set_value, {
	int value;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &value))
		return NULL;
	
	impl->setValue(value);
})



SL_START_PROXY_DERIVED(Progress, Ranged)
SL_PROPERTY(style)
SL_PROPERTY(min)
SL_PROPERTY(max)
SL_PROPERTY(value)
SL_END_PROXY_DERIVED(Progress, Ranged)


#include "progress.moc"
#include "progress_h.moc"

