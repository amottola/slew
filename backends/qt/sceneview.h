#ifndef __sceneview_h__
#define __sceneview_h__


#include "slew.h"

#include <QGraphicsObject>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPicture>
#include <QPainterPath>


class SceneItem_Impl : public QGraphicsTextItem
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(SceneItem)
	
	virtual QRectF boundingRect() const;
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual bool contains(const QPointF& point) const { return QGraphicsItem::contains(point); }
	virtual QPainterPath shape() const { return QGraphicsItem::shape(); }
	
	void setSize(const QSize& size) { fSize = size; setTextWidth(size.width()); }
	QSize size() { return fSize; }
	
	void setOrigin(const QPoint& origin) { fOrigin = origin; }
	QPoint origin() { return fOrigin; }
	
	void invalidate() { fRepaint = true; prepareGeometryChange(); }
	
public slots:
	void detach();
	
protected:
	virtual bool sceneEvent(QEvent *event);
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
	virtual QVariant itemChange(GraphicsItemChange change, const QVariant& value);
	
private:
	QPicture	fPicture;
	QPoint		fOrigin;
	QSize		fSize;
	bool		fRepaint;
};



class SceneView_Impl : public QGraphicsView, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(SceneView, {
		foreach (QGraphicsItem *item, items())
			QMetaObject::invokeMethod((QGraphicsObject *)item, "detach", Qt::AutoConnection);
	})
	
	SL_DECLARE_SET_VISIBLE(QGraphicsView)
	SL_DECLARE_SIZE_HINT(QGraphicsView)
	
	PyObject *selection();

public slots:
	void handleScrollBars();
	void handleSelectionChanged();
	
protected:
	virtual void drawBackground(QPainter *painter, const QRectF& rect);
	virtual void drawForeground(QPainter *painter, const QRectF& rect);
	
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void focusOutEvent(QFocusEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
};


#endif
