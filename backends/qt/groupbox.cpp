#include "slew.h"

#include "groupbox.h"


GroupBox_Impl::GroupBox_Impl()
	: QGroupBox(), WidgetInterface()
{
}


SL_DEFINE_METHOD(GroupBox, get_label, {
	return createStringObject(impl->title());
})


SL_DEFINE_METHOD(GroupBox, set_label, {
	QString label;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &label))
		return NULL;
	
	impl->setTitle(label);
})


SL_START_PROXY_DERIVED(GroupBox, Window)
SL_PROPERTY(label)
SL_END_PROXY_DERIVED(GroupBox, Window)


#include "groupbox.moc"
#include "groupbox_h.moc"

