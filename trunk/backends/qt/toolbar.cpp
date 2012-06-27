#include "slew.h"

#include "toolbar.h"
#include "toolbaritem.h"
#include "constants/widget.h"

#include <QMainWindow>
#include <QActionEvent>


ToolBar_Impl::ToolBar_Impl()
	: QToolBar(), WidgetInterface(), fArea(Qt::TopToolBarArea)
{
	QVariant value;
	value.setValue(&fActionGroups);
	setProperty("action_groups", value);
	setIconSize(QSize(32, 32));
	setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
}


void
ToolBar_Impl::actionEvent(QActionEvent *event)
{
	QToolBar::actionEvent(event);
	
	if ((event->type() == QEvent::ActionAdded) || (event->type() == QEvent::ActionRemoved)) {
		relinkActions(this);
		if (event->type() == QEvent::ActionAdded) {
			ToolBarItem_Impl *item = qobject_cast<ToolBarItem_Impl *>(event->action());
			if (item)
				item->update();
		}
	}
}


SL_DEFINE_METHOD(ToolBar, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isToolBarItem(object)) {
		ToolBarItem_Impl *widget = (ToolBarItem_Impl *)child;
		QList<QAction *> list = impl->actions();
		QAction *before;
		
		if ((signed)index < list.count())
			before = list.at(index);
		else
			before = NULL;
		impl->insertAction(before, widget);
	}
	else if (isWindow(object)) {
		QWidget *widget = (QWidget *)child;
		QList<QAction *> list = impl->actions();
		QAction *before;
		
		if ((signed)index < list.count())
			before = list.at(index);
		else
			before = NULL;
		impl->insertWidget(before, widget);
	}
	else
		SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(ToolBar, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isToolBarItem(object)) {
		ToolBarItem_Impl *widget = (ToolBarItem_Impl *)child;
		impl->removeAction(widget);
	}
	else if (isWindow(object)) {
		QList<QAction *> list = impl->actions();
		
		foreach(QAction *action, list) {
			QWidgetAction *wa = qobject_cast<QWidgetAction *>(action);
			if ((wa) && (wa->defaultWidget() == child)) {
				impl->removeAction(action);
				break;
			}
		}
	}
	else
		SL_RETURN_CANNOT_DETACH;
})


SL_DEFINE_METHOD(ToolBar, get_align, {
	int align = SL_ALIGN_TOP;
	int area = impl->area();
	
	if (area == Qt::LeftToolBarArea)
		align = SL_ALIGN_LEFT;
	if (area == Qt::RightToolBarArea)
		align = SL_ALIGN_RIGHT;
	if (area == Qt::BottomToolBarArea)
		align = SL_ALIGN_BOTTOM;
	
	return PyInt_FromLong(align);
})


SL_DEFINE_METHOD(ToolBar, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "i", &align))
		return NULL;
	
	Qt::ToolBarArea area = Qt::TopToolBarArea;
	if (align & SL_ALIGN_BOTTOM)
		area = Qt::BottomToolBarArea;
	if (align & SL_ALIGN_LEFT)
		area = Qt::LeftToolBarArea;
	if (align & SL_ALIGN_RIGHT)
		area = Qt::RightToolBarArea;
	
	impl->setArea(area);
	QMainWindow *window = qobject_cast<QMainWindow *>(impl->parentWidget());
	if (window)
		window->addToolBar(area, impl);
})


SL_DEFINE_METHOD(ToolBar, get_style, {
	int style = 0;
	
	switch (impl->toolButtonStyle()) {
	case Qt::ToolButtonTextOnly:	style = SL_TOOLBAR_STYLE_TEXT; break;
	case Qt::ToolButtonIconOnly:	style = SL_TOOLBAR_STYLE_ICON; break;
	default:						style = SL_TOOLBAR_STYLE_ICON | SL_TOOLBAR_STYLE_TEXT; break;
	}
	if (impl->orientation() == Qt::Vertical)
		style |= SL_TOOLBAR_STYLE_VERTICAL;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(ToolBar, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	Qt::ToolButtonStyle qstyle;
	
	switch (style & (SL_TOOLBAR_STYLE_TEXT | SL_TOOLBAR_STYLE_ICON)) {
	case SL_TOOLBAR_STYLE_TEXT:	qstyle = Qt::ToolButtonTextOnly; break;
	case SL_TOOLBAR_STYLE_ICON:	qstyle = Qt::ToolButtonIconOnly; break;
	default:					qstyle = Qt::ToolButtonTextUnderIcon; break;
	}
	impl->setToolButtonStyle(qstyle);
	impl->setOrientation(style & SL_TOOLBAR_STYLE_VERTICAL ? Qt::Vertical : Qt::Horizontal);
})



SL_START_PROXY_DERIVED(ToolBar, Window)
SL_METHOD(insert)
SL_METHOD(remove)

SL_PROPERTY(align)
SL_PROPERTY(style)
SL_END_PROXY_DERIVED(ToolBar, Window)


#include "toolbar.moc"
#include "toolbar_h.moc"

