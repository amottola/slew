#ifndef __menubar_h__
#define __menubar_h__


#include "slew.h"
#include "constants/window.h"

#include <QMenuBar>


class MenuBar_Impl : public QMenuBar, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(MenuBar)
};


#endif
