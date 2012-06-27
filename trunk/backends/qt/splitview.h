#ifndef __splitview_h__
#define __splitview_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/splitview.h"

#include <QSplitter>


class SplitView_Impl : public QSplitter, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(SplitView)
	
	SL_DECLARE_SET_VISIBLE(QSplitter)
	SL_DECLARE_SIZE_HINT(QSplitter)
	
	void setSizes(const QList<int>& sizes) { QSplitter::setSizes(sizes); fSizes = QSplitter::sizes(); }
	QList<int> storedSizes() { return fSizes; }
	
	void setPolicies(bool restore = false);
	
	virtual QSplitterHandle *createHandle();
	
public slots:
	void handleSplitterMoved(int pos, int index);
	
private:
	QList<int>	fSizes;
};


#endif
