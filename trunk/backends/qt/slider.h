#ifndef __slider_h__
#define __slider_h__


#include "slew.h"

#include "constants/ranged.h"
#include "constants/slider.h"

#include <QSlider>


class Slider_Impl : public QSlider, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Slider)
	
	SL_DECLARE_SET_VISIBLE(QSlider)
	SL_DECLARE_SIZE_HINT(QSlider)
	
	virtual bool isModifyEvent(QEvent *event);
	
public slots:
	void handleValueChanged(int value);
};


#endif
