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
	SL_DECLARE_OBJECT(MenuItem, {
#ifdef Q_WS_MAC
		QList<QWidget *> list = associatedWidgets();
		if (!list.isEmpty()) {
			QMenu *menu = qobject_cast<QMenu *>(list.first());
			if (menu)
				helper_clear_menu_previous_action(menu);
		}
#endif
	})
	
public slots:
	void handleActionTriggered();
};


#endif
