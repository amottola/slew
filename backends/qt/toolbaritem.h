#ifndef __toolbaritem_h__
#define __toolbaritem_h__


#include "slew.h"
#include "constants/toolbaritem.h"

#include <QWidgetAction>
#include <QMenu>
#include <QToolButton>


class ToolBarItem_Impl : public QWidgetAction, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ToolBarItem)
	
	void update();
	
	void setSeparator(bool separator);
	
	void setFlexible(bool flexible);
	bool isFlexible() { return fFlexible; }
	
	void setMenu(QMenu *menu, QToolButton::ToolButtonPopupMode mode);
	QMenu *menu() { return fMenu; }
	
	virtual QWidget *createWidget(QWidget *parent);
	
public slots:
	void handleActionTriggered();

private:
	void reinsert();
	
	bool								fSeparator;
	bool								fFlexible;
	QMenu								*fMenu;
	QToolButton::ToolButtonPopupMode	fMenuMode;
};


#endif
