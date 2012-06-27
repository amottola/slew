#ifndef __tabview_h__
#define __tabview_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/tabview.h"

#include <QTabWidget>


class TabView_Impl : public QTabWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(TabView)
	
	SL_DECLARE_SET_VISIBLE(QTabWidget)
	SL_DECLARE_SIZE_HINT(QTabWidget)
};


#endif
