#ifndef __button_h__
#define __button_h__


#include "slew.h"
#include "constants/window.h"
#include "constants/button.h"

#include <QPushButton>


class Button_Impl : public QPushButton, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Button)
	
	SL_DECLARE_SET_VISIBLE(QPushButton)
	SL_DECLARE_SIZE_HINT(QPushButton)
	
	virtual bool isModifyEvent(QEvent *event);
	
public slots:
	void handleClicked();
	void handleToggled(bool toggled);
	void handleEndDrag(QObject *object);
};


#endif
