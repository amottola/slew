#include "slew.h"

#include "menu.h"
#include "menuitem.h"


MenuItem_Impl::MenuItem_Impl()
	: QAction(NULL), WidgetInterface()
{
	setCheckable(true);
}


void
MenuItem_Impl::handleActionTriggered()
{
	EventRunner(this, "onSelect").run();
}


SL_DEFINE_METHOD(MenuItem, get_type, {
	return PyInt_FromLong(qvariant_cast<int>(impl->property("type")));
})


SL_DEFINE_METHOD(MenuItem, set_type, {
	int type;
	
	if (!PyArg_ParseTuple(args, "i", &type))
		return NULL;
	
	impl->setProperty("type", type);
	QAction::MenuRole role = QAction::NoRole;
	
	switch (type) {
	case SL_MENU_ITEM_TYPE_QUIT:		role = QAction::QuitRole; break;
	case SL_MENU_ITEM_TYPE_ABOUT:		role = QAction::AboutRole; break;
	case SL_MENU_ITEM_TYPE_PREFERENCES:	role = QAction::PreferencesRole; break;
	case SL_MENU_ITEM_TYPE_APPLICATION:	role = QAction::ApplicationSpecificRole; break;
	}
	
	impl->setMenuRole(role);
	impl->setSeparator(type == SL_MENU_ITEM_TYPE_SEPARATOR ? true : false);
	
	foreach (QWidget *parent, impl->associatedWidgets()) {
		relinkActions(parent);
	}
})


SL_DEFINE_METHOD(MenuItem, get_text, {
	return createStringObject(impl->text());
})


SL_DEFINE_METHOD(MenuItem, set_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	if (!impl->isSeparator()) {
		int pos = text.indexOf('|');
		if ((pos != -1) && (pos + 1 < text.length())) {
			impl->setShortcut(QKeySequence(text.mid(pos + 1)));
			text = text.mid(0, pos);
		}
		impl->setText(text);
	}
})


SL_DEFINE_METHOD(MenuItem, get_description, {
	return createStringObject(impl->statusTip());
})


SL_DEFINE_METHOD(MenuItem, set_description, {
	QString description;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &description))
		return NULL;
	
	if (!impl->isSeparator()) {
		impl->setStatusTip(description);
	}
})


SL_DEFINE_METHOD(MenuItem, is_checked, {
	return createBoolObject(impl->isChecked());
})


SL_DEFINE_METHOD(MenuItem, set_checked, {
	bool checked;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &checked))
		return NULL;
	
	impl->setChecked(checked);
})


SL_DEFINE_METHOD(MenuItem, is_enabled, {
	return createBoolObject(impl->isEnabled());
})


SL_DEFINE_METHOD(MenuItem, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	impl->setEnabled(enabled);
})


SL_DEFINE_METHOD(MenuItem, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_METHOD(MenuItem, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
})


SL_START_PROXY_DERIVED(MenuItem, Widget)
SL_PROPERTY(type)
SL_PROPERTY(text)
SL_PROPERTY(description)
SL_BOOL_PROPERTY(checked)
SL_BOOL_PROPERTY(enabled)
SL_PROPERTY(icon)
SL_END_PROXY_DERIVED(MenuItem, Widget)


#include "menuitem.moc"
#include "menuitem_h.moc"

