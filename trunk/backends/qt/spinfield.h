#ifndef __spinfield_h__
#define __spinfield_h__


#include "slew.h"

#include "constants/ranged.h"
#include "constants/spinfield.h"

#include <QSpinBox>


class SpinField_Impl : public QSpinBox, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(SpinField)
	
	SL_DECLARE_SET_VISIBLE(QSpinBox)
	SL_DECLARE_SIZE_HINT(QSpinBox)
	
	virtual bool isModifyEvent(QEvent *event);
	
public slots:
	void handleValueChanged(int value);
};


#endif
