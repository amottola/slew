#ifndef __checkbox_h__
#define __checkbox_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/checkbox.h"

#include <QCheckBox>


class CheckBox_Impl : public QCheckBox, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(CheckBox)
	
	SL_DECLARE_SET_VISIBLE(QCheckBox)
	SL_DECLARE_SIZE_HINT(QCheckBox)

	virtual bool isModifyEvent(QEvent *event);
	
public slots:
	void handleToggled(bool toggled);
};


#endif
