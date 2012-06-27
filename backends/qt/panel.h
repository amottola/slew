#ifndef __panel_h__
#define __panel_h__


#include "slew.h"


class Panel_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Panel)
	
	SL_DECLARE_SET_VISIBLE(QWidget)
	SL_DECLARE_SIZE_HINT(QWidget)
};


#endif
