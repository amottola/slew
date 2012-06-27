#include "slew.h"

#include "tabview.h"
#include "tabviewpage.h"



TabViewPage_Impl::TabViewPage_Impl()
	: QWidget(), WidgetInterface(), fTabView(NULL), fEnabled(true)
{
}


void
TabViewPage_Impl::setTabView(TabView_Impl *tabView, int index)
{
	fTabView = tabView;
	if (fTabView) {
		fTabView->setTabText(index, fTitle);
		fTabView->setTabIcon(index, fIcon);
		fTabView->setTabEnabled(index, fEnabled);
	}
}

 
void
TabViewPage_Impl::setTitle(const QString& title)
{
	fTitle = title;
	if (fTabView)
		fTabView->setTabText(fTabView->indexOf(this), title);
}


void
TabViewPage_Impl::setIcon(const QIcon& icon)
{
	fIcon = icon;
	if (fTabView)
		fTabView->setTabIcon(fTabView->indexOf(this), icon);
}


void
TabViewPage_Impl::_setEnabled(bool enabled)
{
	fEnabled = enabled;
	if (fTabView)
		fTabView->setTabEnabled(fTabView->indexOf(this), enabled);
}


SL_DEFINE_METHOD(TabViewPage, is_enabled, {
	return createBoolObject(impl->_isEnabled());
})


SL_DEFINE_METHOD(TabViewPage, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	impl->_setEnabled(enabled);
})


SL_DEFINE_METHOD(TabViewPage, get_title, {
	return createStringObject(impl->title());
})


SL_DEFINE_METHOD(TabViewPage, set_title, {
	QString title;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &title))
		return NULL;
	
	impl->setTitle(title);
})


SL_DEFINE_METHOD(TabViewPage, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_METHOD(TabViewPage, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
})



SL_START_PROXY_DERIVED(TabViewPage, Window)
SL_BOOL_PROPERTY(enabled)
SL_PROPERTY(title)
SL_PROPERTY(icon)
SL_END_PROXY_DERIVED(TabViewPage, Window)


#include "tabviewpage.moc"
#include "tabviewpage_h.moc"

