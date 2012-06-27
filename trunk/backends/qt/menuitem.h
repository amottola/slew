#ifndef __menuitem_h__
#define __menuitem_h__


#include "slew.h"
#include "constants/menuitem.h"

#include <QAction>


class MenuItem_Impl : public QAction, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(MenuItem)
	
public slots:
	void handleActionTriggered();
};


#endif
