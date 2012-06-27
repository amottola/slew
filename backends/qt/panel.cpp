#include "slew.h"

#include "panel.h"

#include <QSizePolicy>



Panel_Impl::Panel_Impl()
	: QWidget(), WidgetInterface()
{
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
}


SL_START_PROXY_DERIVED(Panel, Window)
SL_END_PROXY_DERIVED(Panel, Window)


#include "panel.moc"
#include "panel_h.moc"

