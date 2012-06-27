#ifndef __tabview_h__
#define __tabview_h__


#include "slew.h"

#include "constants/window.h"

#include <QStackedWidget>


class StackView_Impl : public QStackedWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(StackView)
	
	SL_DECLARE_SET_VISIBLE(QStackedWidget)
	SL_DECLARE_SIZE_HINT(QStackedWidget)
};


#endif
