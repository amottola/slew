#ifndef __scrollview_h__
#define __scrollview_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/scrollview.h"

#include <QScrollArea>
#include <QScrollBar>


class ScrollView_Impl : public QScrollArea, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ScrollView)
	
	SL_DECLARE_SET_VISIBLE(QScrollArea)
	SL_DECLARE_SIZE_HINT(QScrollArea)

public slots:
	void handleContextMenu();
	void handleScrollBars();
};


#endif
