#ifndef __progress_h__
#define __progress_h__


#include "slew.h"

#include "constants/ranged.h"
#include "constants/progress.h"

#include <QProgressBar>


class Progress_Impl : public QProgressBar, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Progress)
	
	SL_DECLARE_SET_VISIBLE(QProgressBar)
	SL_DECLARE_SIZE_HINT(QProgressBar)
	
	void setIndeterminate(bool indeterminate);
	
	void setMinimum(int min) { fMinimum = min; QProgressBar::setMinimum(min); }
	void setMaximum(int max) { fMaximum = max; QProgressBar::setMaximum(max); }
	void setValue(int value) { fValue = value; QProgressBar::setValue(value); }
	
private:
	int		fMinimum;
	int		fMaximum;
	int		fValue;
};


#endif
