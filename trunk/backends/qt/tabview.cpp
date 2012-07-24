#include "slew.h"

#include "tabview.h"
#include "tabviewpage.h"

#include <QTabBar>
#include <QWheelEvent>
#include <QMouseEvent>


class TabBar : public QTabBar
{
	Q_OBJECT
	
public:
	TabBar(QWidget *parent) : QTabBar(parent) { setElideMode(Qt::ElideNone); }
	
	virtual void wheelEvent(QWheelEvent *event)
	{
		/* nothing to do */
		
		QWidget::wheelEvent(event);
	}
	
	virtual void mousePressEvent(QMouseEvent *event)
	{
		QPoint pos = event->pos();
		TabView_Impl *impl = (TabView_Impl *)parent();
		for (int i = 0; i < impl->count(); i++) {
			if ((impl->isTabEnabled(i)) && (tabRect(i).contains(pos)) && (i != impl->currentIndex())) {
				
				EventRunner runner(impl, "onChange");
				if (runner.isValid()) {
					runner.set("value", i);
					if (!runner.run()) {
						event->ignore();
						return;
					}
				}
				break;
			}
		}
		QTabBar::mousePressEvent(event);
	}
};


TabView_Impl::TabView_Impl()
	: QTabWidget(), WidgetInterface()
{
	setTabBar(new TabBar(this));
}


SL_DEFINE_METHOD(TabView, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	if (!isTabViewPage(object))
		SL_RETURN_CANNOT_ATTACH;
	
	TabViewPage_Impl *child = (TabViewPage_Impl *)getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	index = impl->insertTab(index, child, child->title());
	child->setTabView(impl, index);
})


SL_DEFINE_METHOD(TabView, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (!isTabViewPage(object))
		SL_RETURN_CANNOT_DETACH;
	
	TabViewPage_Impl *child = (TabViewPage_Impl *)getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	int index = impl->indexOf(child);
	if (index >= 0) {
		impl->removeTab(index);
		child->setParent(NULL);
	}
	child->setTabView(NULL, -1);
})


SL_DEFINE_METHOD(TabView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->documentMode())
		style |= SL_TABVIEW_STYLE_DOCUMENT;
	if (impl->usesScrollButtons())
		style |= SL_TABVIEW_STYLE_SCROLL;
	if (impl->tabShape() == QTabWidget::Triangular)
		style |= SL_TABVIEW_STYLE_TRIANGULAR;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(TabView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setDocumentMode(style & SL_TABVIEW_STYLE_DOCUMENT ? true : false);
	impl->setUsesScrollButtons(style & SL_TABVIEW_STYLE_SCROLL ? true : false);
	impl->setTabShape(style & SL_TABVIEW_STYLE_TRIANGULAR ? QTabWidget::Triangular : QTabWidget::Rounded);
})


SL_DEFINE_METHOD(TabView, get_page, {
	return PyInt_FromLong(impl->currentIndex());
})


SL_DEFINE_METHOD(TabView, set_page, {
	int page;
	
	if (!PyArg_ParseTuple(args, "i", &page))
		return NULL;
	
	if ((page < 0) || (page >= impl->count())) {
		PyErr_SetString(PyExc_ValueError, "invalid page number");
		return NULL;
	}
	
	impl->blockSignals(true);
	impl->setCurrentIndex(page);
	impl->blockSignals(false);
})


SL_DEFINE_METHOD(TabView, get_position, {
	int position;
	switch (impl->tabPosition()) {
	case QTabWidget::West:		position = SL_LEFT; break;
	case QTabWidget::East:		position = SL_RIGHT; break;
	case QTabWidget::South:		position = SL_BOTTOM; break;
	default:					position = SL_TOP; break;
	}
	return PyInt_FromLong(position);
})


SL_DEFINE_METHOD(TabView, set_position, {
	int pos;
	
	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	QTabWidget::TabPosition position;
	switch (pos) {
	case SL_LEFT:		position = QTabWidget::West; break;
	case SL_RIGHT:		position = QTabWidget::East; break;
	case SL_BOTTOM:		position = QTabWidget::South; break;
	default:			position = QTabWidget::North; break;
	}
	impl->setTabPosition(position);
})



SL_START_PROXY_DERIVED(TabView, Window)
SL_METHOD(insert)
SL_METHOD(remove)

SL_PROPERTY(style)
SL_PROPERTY(page)
SL_PROPERTY(position)
SL_END_PROXY_DERIVED(TabView, Window)


#include "tabview.moc"
#include "tabview_h.moc"

