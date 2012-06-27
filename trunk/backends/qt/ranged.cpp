#include "slew.h"

#include "sizer.h"

#include "constants/window.h"

#include <QAbstractSlider>


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, get_min, {
	return PyInt_FromLong(impl->minimum());
})


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, set_min, {
	int min;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &min))
		return NULL;
	
	impl->setMinimum(min);
	impl->setPageStep((impl->maximum() - impl->minimum()) / 10);
})


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, get_max, {
	return PyInt_FromLong(impl->maximum());
})


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, set_max, {
	int max;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &max))
		return NULL;
	
	impl->setMaximum(max);
	impl->setPageStep((impl->maximum() - impl->minimum()) / 10);
})


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, get_value, {
	return PyInt_FromLong(impl->value());
})


SL_DEFINE_ABSTRACT_METHOD(Ranged, QAbstractSlider, set_value, {
	int value;
	
	if (!PyArg_ParseTuple(args, "O&", convertInt, &value))
		return NULL;
	
	impl->setValue(value);
})



SL_START_ABSTRACT_PROXY_DERIVED(Ranged, Window)
SL_PROPERTY(min)
SL_PROPERTY(max)
SL_PROPERTY(value)
SL_END_ABSTRACT_PROXY_DERIVED(Ranged, Window)


#include "ranged.moc"

