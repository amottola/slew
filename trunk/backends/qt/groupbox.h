#ifndef __groupbox_h__
#define __groupbox_h__


#include "slew.h"

#include <QGroupBox>



class GroupBox_Impl : public QGroupBox, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(GroupBox)
	
	SL_DECLARE_SET_VISIBLE(QGroupBox)
	SL_DECLARE_SIZE_HINT(QGroupBox)
};


#endif
