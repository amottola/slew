#ifndef __sizer_h__
#define __sizer_h__


#include "slew.h"

#include <QGridLayout>
#include <QLayoutItem>


class WidgetItem : public QLayoutItem
{
public:
	WidgetItem(Window_Impl *widget);
	virtual QWidget *widget() { return fWidget; }
	
	virtual bool isEmpty() const { return fWidget->isHidden() || fWidget->isWindow(); }
	
	virtual QRect geometry() const;
	virtual void setGeometry(const QRect& rect);
	
	virtual bool hasHeightForWidth() const { return false; }
	
	virtual QSize minimumSize() const;
	virtual QSize maximumSize() const;
	virtual QSize sizeHint() const;
	virtual Qt::Orientations expandingDirections() const;
	
	virtual void invalidate() { fMargins = qvariant_cast<QMargins>(fWidget->property("boxMargins")); }
	
private:
	Window_Impl		*fWidget;
	QMargins		fMargins;
};



class AbstractSizerInterface
{
public:
	AbstractSizerInterface() {}
	virtual ~AbstractSizerInterface() {}

	virtual QLayout *clone() { return NULL; }
	virtual void doReparentChildren(QLayoutItem *item, QLayout *parent) {}
};

Q_DECLARE_INTERFACE(AbstractSizerInterface, "slew.abstractsizerinterface/1.0");


class AbstractSizer_Impl : public QLayout, public WidgetInterface, public AbstractSizerInterface
{
	Q_OBJECT
	Q_INTERFACES(AbstractSizerInterface)
};



class Sizer_Impl : public QGridLayout, public WidgetInterface, public AbstractSizerInterface
{
	Q_OBJECT
	Q_INTERFACES(AbstractSizerInterface)
	
public:
	SL_DECLARE_OBJECT(Sizer)
	
	void setOrientation(Qt::Orientation orient) { fOrientation = orient; }
	Qt::Orientation orientation() { return fOrientation; }
	
	virtual Qt::Orientations expandingDirections() const;
	
	void reinsert(QObject *child, bool reparent=true);
	virtual QLayout *clone();
	
	void ensurePosition(QPoint& cell, const QSize& span);
	
	virtual QLayoutItem *takeAt(int index);
	
	virtual void doReparentChildren(QLayoutItem *item, QLayout *parent);
	
private:
	Qt::Orientation		fOrientation;
};


#endif
