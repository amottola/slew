#ifndef __flowbox_h__
#define __flowbox_h__


#include "slew.h"
#include "sizer.h"

#include <QLayout>
#include <QLayoutItem>


class FlowBox_Impl : public QLayout, public WidgetInterface, public AbstractSizerInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(FlowBox, {
		QLayoutItem *item;
		while ((item = takeAt(0)))
			delete item;
	})
	
	void setOrientation(Qt::Orientation orient) { fOrientation = orient; }
	Qt::Orientation orientation() { return fOrientation; }

	void setSpacing(const QPoint& spacing) { fSpacing = spacing; }
	QPoint spacing() { return fSpacing; }
	
	virtual Qt::Orientations expandingDirections() const { return 0; }
	virtual void addItem(QLayoutItem *item) { fItems.append(item); }
	virtual QLayoutItem *takeAt(int index);
	virtual QLayoutItem *itemAt(int index) const { return fItems.value(index); }
	virtual int count() const { return fItems.count(); }

	virtual bool hasHeightForWidth() const { return true; }
	virtual int heightForWidth(int width) const { return doLayout(QRect(0, 0, width, 0), true); }
	virtual QSize sizeHint() const { return minimumSize(); }
	virtual QSize minimumSize() const;
	virtual void setGeometry(const QRect &rect);

	virtual QLayout *clone();
	
	virtual void doReparentChildren(QLayoutItem *item, QLayout *parent);
	
private:
	int doLayout(const QRect& rect, bool testOnly) const;

	Qt::Orientation			fOrientation;
    QList<QLayoutItem *>	fItems;
    QPoint					fSpacing;
};


#endif
