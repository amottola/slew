#ifndef __menuitem_h__
#define __menuitem_h__


#include "slew.h"
#include "constants/menuitem.h"

#include <QAction>
#include <QMenu>


class MenuItem_Impl : public QAction, public WidgetInterface
{
	Q_OBJECT
	
public:
#ifdef Q_OS_MAC
	SL_DECLARE_OBJECT(MenuItem, {
		foreach (QWidget *parent, associatedWidgets()) {
			QMenu *menu = qobject_cast<QMenu *>(parent);
			if (menu)
				helper_clear_menu_previous_action(menu);
		}
	})
#else
	SL_DECLARE_OBJECT(MenuItem)
#endif
	
public slots:
	void handleActionTriggered();
};


#endif
