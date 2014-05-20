#include "slew.h"

#include "menubar.h"
#include "menu.h"

#include <QMainWindow>


MenuBar_Impl::MenuBar_Impl()
	: QMenuBar(NULL), WidgetInterface()
{
}


SL_DEFINE_METHOD(MenuBar, insert, {
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
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(MenuBar, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isMenu(object)) {
		Menu_Impl *widget = (Menu_Impl *)child;
		impl->removeAction(widget->menuAction());
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_DETACH;
})


SL_START_PROXY_DERIVED(MenuBar, Widget)
SL_METHOD(insert)
SL_METHOD(remove)
SL_END_PROXY_DERIVED(MenuBar, Widget)


#include "menubar.moc"
#include "menubar_h.moc"

