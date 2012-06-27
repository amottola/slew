#ifndef __line_h__
#define __line_h__


#include "slew.h"

#include "constants/line.h"

#include <QFrame>


class Line_Impl : public QFrame, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Line)
	
	SL_DECLARE_SET_VISIBLE(QFrame)
	SL_DECLARE_SIZE_HINT(QFrame)
};


#endif
