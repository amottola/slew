#include "slew.h"

#include "popupwindow.h"

#include <QResizeEvent>
#include <QHideEvent>


PopupWindow_Impl::PopupWindow_Impl()
	: QWidget(NULL, Qt::Popup), WidgetInterface()
{
}


void
PopupWindow_Impl::moveEvent(QMoveEvent *event)
{
	hidePopupMessage();
	Completer::hide();
}


void
PopupWindow_Impl::resizeEvent(QResizeEvent *event)
{
	hidePopupMessage();
	Completer::hide();
	
	if (event->size() != event->oldSize()) {
		EventRunner runner(this, "onResize");
		if (runner.isValid()) {
			runner.set("size", event->size());
			runner.run();
		}
	}
}


void
PopupWindow_Impl::showEvent(QShowEvent *event)
{
	PyAutoLocker locker;
	getObject(this);	// increases refcount
}


void
PopupWindow_Impl::hideEvent(QHideEvent *event)
{
	setParent(NULL, Qt::Popup);
	PyAutoLocker locker;
	PyObject *object = getObject(this, false);
	Py_XDECREF(object);
	PyErr_Clear();
}


void
PopupWindow_Impl::closeEvent(QCloseEvent *event)
{
	EventRunner runner(this, "onClose");
	if (runner.isValid()) {
		runner.set("accepted", true);
		runner.run();
	}
}


SL_DEFINE_METHOD(PopupWindow, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	return insertWindowChild(impl, object);
})


SL_DEFINE_METHOD(PopupWindow, remove, {
	(void)impl;
	
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	return removeWindowChild(impl, object);
})


SL_DEFINE_METHOD(PopupWindow, popup, {
	(void)impl;
	
	PyObject *object;
	Window_Impl *parent;
	QPoint pos;
	QSize size;
	
	if (!PyArg_ParseTuple(args, "OO&O&", &object, convertPoint, &pos, convertSize, &size))
		return NULL;
	
	if (object == Py_None) {
		parent = NULL;
	}
	else {
		parent = (Window_Impl *)getImpl(object);
		if (!parent) {
			PyErr_SetString(PyExc_ValueError, "expected 'Widget' or 'None' object");
			return NULL;
		}
	}
	impl->setParent(parent, Qt::Popup);
	if (parent)
		pos = parent->mapToGlobal(pos);
	if (!size.isValid())
		size = impl->sizeHint();
	impl->setGeometry(QRect(pos, size));
	impl->show();
})


SL_DEFINE_METHOD(PopupWindow, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(PopupWindow, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
})



SL_START_PROXY_DERIVED(PopupWindow, Frame)
SL_METHOD(insert)
SL_METHOD(remove)

SL_METHOD(popup)

SL_PROPERTY(style)
SL_END_PROXY_DERIVED(PopupWindow, Frame)


#include "popupwindow.moc"
#include "popupwindow_h.moc"

