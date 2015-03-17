#ifndef __menu_h__
#define __menu_h__


#include "slew.h"
#include "constants/window.h"

#include <QMenu>
#include <QActionEvent>
#include <QActionGroup>


class Menu_Impl : public QMenu, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Menu, {
		while (!fActionGroups.isEmpty())
			fActionGroups.takeFirst()->deleteLater();
	})
	
protected:
	virtual bool event(QEvent *e);
	virtual void actionEvent(QActionEvent *event);
	
private:
	QList<QActionGroup *>	fActionGroups;
};


#endif
