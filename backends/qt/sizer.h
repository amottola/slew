#ifndef __sizer_h__
#define __sizer_h__


#include "slew.h"

#include <QGridLayout>
#include <QLayoutItem>


class Sizer_Impl : public QGridLayout, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Sizer)
	
	void setOrientation(Qt::Orientation orient) { fOrientation = orient; }
	Qt::Orientation orientation() { return fOrientation; }
	
	virtual Qt::Orientations expandingDirections() const;
	
	void reinsert(QObject *child, bool reparent=true);
	Sizer_Impl *clone();
	
	void ensurePosition(QPoint& cell, const QSize& span);
	
	virtual QLayoutItem *takeAt(int index);
	
	static void reparentChildren(QLayoutItem *item, Sizer_Impl *parent);
	
private:
	Qt::Orientation		fOrientation;
};


#endif
