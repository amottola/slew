#ifndef __popupwindow_h__
#define __popupwindow_h__


#include "slew.h"
#include "constants/window.h"
#include "constants/frame.h"

#include <QWidget>


class PopupWindow_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(PopupWindow)
	
	SL_DECLARE_SIZE_HINT(QWidget)
	
protected:
	virtual void moveEvent(QMoveEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);
	virtual void closeEvent(QCloseEvent *event);
};


#endif
