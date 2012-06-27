#ifndef __toolbar_h__
#define __toolbar_h__


#include "slew.h"
#include "constants/toolbar.h"

#include <QToolBar>


class ToolBar_Impl : public QToolBar, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ToolBar, {
		while (!fActionGroups.isEmpty())
			delete fActionGroups.takeFirst();
	})
	
	void setArea(Qt::ToolBarArea area) { fArea = area; }
	Qt::ToolBarArea area() { return fArea; }
	
protected:
	virtual void actionEvent(QActionEvent *event);
	
private:
	QList<QActionGroup *>	fActionGroups;
	Qt::ToolBarArea			fArea;
};


#endif
