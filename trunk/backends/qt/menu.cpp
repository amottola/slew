#include "slew.h"

#include "menu.h"
#include "menuitem.h"

#include <QWidgetAction>
#include <QPalette>



class WidgetAction : public QWidgetAction
{
	Q_OBJECT
	
public:
	WidgetAction(Menu_Impl *parent, QWidget *widget)
		: QWidgetAction(parent), fWidget(widget)
	{
	}
	
	virtual QWidget *createWidget(QWidget *parent)
	{
		fWidget->setParent(parent);
		return fWidget;
	}
	
	virtual void deleteWidget(QWidget *widget)
	{
		widget->hide();
	}

private:
	QWidget		*fWidget;
};



Menu_Impl::Menu_Impl()
	: QMenu(), WidgetInterface()
{
	QVariant value;
	value.setValue(&fActionGroups);
	setProperty("action_groups", value);
	connect(this, SIGNAL(aboutToHide()), this, SLOT(handleAboutToHide()), Qt::QueuedConnection);
	connect(this, SIGNAL(aboutToShow()), this, SLOT(handleAboutToShow()));
}


void
Menu_Impl::handleAboutToHide()
{
	PyAutoLocker locker;
	EventRunner(this, "onCollapse").run();
	PyObject *object = getObject(this, false);
	Py_DECREF(object);
}


void
Menu_Impl::handleAboutToShow()
{
	PyAutoLocker locker;
	PyObject *object = getObject(this, false);
	Py_INCREF(object);
	EventRunner(this, "onExpand").run();
}


void
Menu_Impl::actionEvent(QActionEvent *event)
{
	QMenu::actionEvent(event);
	
	if ((event->type() == QEvent::ActionAdded) || (event->type() == QEvent::ActionRemoved)) {
		relinkActions(this);
	}
}


SL_DEFINE_METHOD(Menu, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isMenu(object)) {
		Menu_Impl *widget = (Menu_Impl *)child;
		QList<QAction *> list = impl->actions();
		QAction *before;
		
		if ((signed)index < list.count())
			before = list.at(index);
		else
			before = NULL;
		impl->insertAction(before, widget->menuAction());
	}
	else if (isMenuItem(object)) {
		MenuItem_Impl *widget = (MenuItem_Impl *)child;
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
		QAction *before, *action;
		
		if ((signed)index < list.count())
			before = list.at(index);
		else
			before = NULL;
		action = new WidgetAction(impl, widget);
		impl->insertAction(before, action);
	}
	else
		SL_RETURN_CANNOT_ATTACH
})


SL_DEFINE_METHOD(Menu, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isMenu(object)) {
		Menu_Impl *widget = (Menu_Impl *)child;
		impl->removeAction(widget->menuAction());
	}
	else if (isMenuItem(object)) {
		MenuItem_Impl *widget = (MenuItem_Impl *)child;
		impl->removeAction(widget);
	}
	else if (isWindow(object)) {
		QList<QAction *> list = impl->actions();
		
		foreach(QAction *action, list) {
			WidgetAction *wa = qobject_cast<WidgetAction *>(action);
			if ((wa) && (wa->defaultWidget() == child)) {
				impl->removeAction(action);
				action->deleteLater();
				break;
			}
		}
	}
	else
		SL_RETURN_CANNOT_DETACH
})


SL_DEFINE_METHOD(Menu, popup, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	if (pos.isNull())
		pos = QCursor::pos();
	
	Py_BEGIN_ALLOW_THREADS
	
	impl->exec(pos);
	
	Py_END_ALLOW_THREADS
})


SL_DEFINE_METHOD(Menu, close, {
	impl->close();
})


SL_DEFINE_METHOD(Menu, get_title, {
	return createStringObject(impl->title());
})


SL_DEFINE_METHOD(Menu, set_title, {
	QString title;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &title))
		return NULL;
	
	impl->setTitle(title);
})


SL_DEFINE_METHOD(Menu, is_enabled, {
	QAction *action = (QAction *)impl->menuAction();
	if (action)
		return createBoolObject(action->isEnabled());
})


SL_DEFINE_METHOD(Menu, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	QAction *action = (QAction *)impl->menuAction();
	if (action)
		action->setEnabled(enabled);
})


SL_START_PROXY_DERIVED(Menu, Widget)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(popup)
SL_METHOD(close)

SL_PROPERTY(title)
SL_BOOL_PROPERTY(enabled)
SL_END_PROXY_DERIVED(Menu, Widget)


#include "menu.moc"
#include "menu_h.moc"

