#include "slew.h"
#include "objects.h"

#include "constants/widget.h"

#include <QAbstractItemView>
#include <QFocusFrame>
#include <QKeyEvent>



Qt::Alignment
fromAlign(int align)
{
	Qt::Alignment alignment = (Qt::Alignment)0;
	
	if (align & SL_ALIGN_LEFT)
		alignment |= Qt::AlignLeft;
	if (align & SL_ALIGN_RIGHT)
		alignment |= Qt::AlignRight;
	if (align & SL_ALIGN_HCENTER)
		alignment |= Qt::AlignHCenter;

	if (align & SL_ALIGN_TOP)
		alignment |= Qt::AlignTop;
	if (align & SL_ALIGN_BOTTOM)
		alignment |= Qt::AlignBottom;
	if (align & SL_ALIGN_VCENTER)
		alignment |= Qt::AlignVCenter;
	
	return alignment;
}


int
toAlign(Qt::Alignment align)
{
	int alignment = 0;
	
	if (align & Qt::AlignLeft)
		alignment |= SL_ALIGN_LEFT;
	if (align & Qt::AlignRight)
		alignment |= SL_ALIGN_RIGHT;
	if (align & Qt::AlignHCenter)
		alignment |= SL_ALIGN_HCENTER;
	if (align & Qt::AlignTop)
		alignment |= SL_ALIGN_TOP;
	if (align & Qt::AlignBottom)
		alignment |= SL_ALIGN_BOTTOM;
	if (align & Qt::AlignVCenter)
		alignment |= SL_ALIGN_VCENTER;
	
	return alignment;
}


bool
WidgetInterface::isFocusOutEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Tab) || (e->key() == Qt::Key_Backtab))
				return true;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::TouchBegin:
	case QEvent::Wheel:
		return true;
	default:
		break;
	}
	return false;
}


bool
WidgetInterface::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	if (newFocus == oldFocus)
		return true;
	return EventRunner(oldFocus, "onFocusOut").run() || (oldFocus->focusPolicy() == Qt::NoFocus);
}


bool
WidgetInterface::canModify(QWidget *widget)
{
	return EventRunner(widget, "onModify").run();
}


bool
EventRunner::run()
{
	bool success = true;
	
	if ((!fProxy) || (!fProxy->fWidget))
		return true;
	
	PyObject *widget = PyWeakref_GetObject(fProxy->fWidget);
	if ((!widget) || (widget == Py_None)) {
		PyErr_Clear();
		return true;
	}
	
	PyObject *handler = PyObject_CallMethod(widget, "get_handler", NULL);
	if ((!handler) || (handler == Py_None)) {
		handler = widget;
		Py_INCREF(handler);
	}
	
	PyObject *method = PyObject_GetAttrString(handler, fName.toUtf8());
	if (method) {
		PyObject *result = NULL;
		Py_XDECREF(fEvent);
		
		PyObject *now = createDateTimeObject(QDateTime::currentDateTime());
		PyObject *posArgs = PyTuple_New(0);
		PyDict_SetItemString(fParams, "widget", widget);
		PyDict_SetItemString(fParams, "time", now);
		fEvent = PyObject_Call(PyEvent_Type, posArgs, fParams);
		Py_DECREF(posArgs);
		Py_DECREF(now);
		
		if (fEvent) {
			result = PyObject_CallFunctionObjArgs(method, fEvent, NULL);
		}
		Py_DECREF(method);
		
		if (!result) {
			PyErr_Print();
			PyErr_Clear();
			success = false;
		}
		else {
			success = ((result == Py_None) || (PyObject_IsTrue(result)));
			Py_DECREF(result);
		}
	}
	else {
		PyErr_Clear();
	}
	Py_DECREF(handler);
	
	return success;
}


Abstract_Proxy *
getProxy(PyObject *object)
{
	if (!PyObject_TypeCheck(object, &Widget_Type)) {
		object = PyObject_GetAttrString(object, "_impl");
		if ((!object) || (!PyObject_TypeCheck(object, &Abstract_Type))) {
			PyErr_Clear();
			Py_XDECREF(object);
			SL_RETURN_NO_IMPL;
		}
		Py_DECREF(object);		// safe as we own _impl
	}
	return (Abstract_Proxy *)object;
}


Abstract_Proxy *
getProxy(QObject *object)
{
	return (Abstract_Proxy *)(SL_QAPP()->getProxy(object));
}


Widget_Proxy *
getSafeProxy(QObject *object)
{
	QObject *o = object;
	Abstract_Proxy *proxy = NULL;
	
	while ((o) && (!proxy)) {
		if (qobject_cast<QFocusFrame *>(o))
			break;
		proxy = getProxy(o);
		if (proxy) {
			QWidget *w = qobject_cast<QWidget *>(o);
			if ((w) && (w->isWindow()) && (w != object))
				proxy = NULL;
		}
		o = o->parent();
	}
	return (Widget_Proxy *)proxy;
}


PyObject *
getObject(QObject *object, bool incref)
{
	if (!object)
		return NULL;
	Widget_Proxy *proxy = getSafeProxy(object);
	if (!proxy)
		return NULL;
	PyObject *widget = PyWeakref_GetObject(proxy->fWidget);
	if (widget == Py_None)
		widget = NULL;
	else if (incref)
		Py_INCREF(widget);
	return widget;
}


PyObject *
getDataModel(QAbstractItemModel *model)
{
	DataModel_Proxy *proxy = (DataModel_Proxy *)getProxy(model);
	if (!proxy)
		return NULL;
	PyObject *object = PyWeakref_GetObject(proxy->fModel);
	Py_INCREF(object);
	return object;
}


QObject *
getImpl(PyObject *object)
{
	if (!object)
		return NULL;
	Abstract_Proxy *proxy = getProxy(object);
	if (!proxy)
		return NULL;
	return proxy->fImpl;
}


PyTypeObject Abstract_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"_slew.Abstract",							/* tp_name */
	sizeof(Abstract_Proxy),						/* tp_basicsize */
	0,											/* tp_itemsize */
	0,											/* tp_dealloc */
	0,											/* tp_print */
	0,											/* tp_getattr */
	0,											/* tp_setattr */
	0,											/* tp_compare */
	0,											/* tp_repr */
	0,											/* tp_as_number */
	0,											/* tp_as_sequence */
	0,											/* tp_as_mapping */
	0,											/* tp_hash */
	0,											/* tp_call */
	0,											/* tp_str */
	0,											/* tp_getattro */
	0,											/* tp_setattro */
	0,											/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,							/* tp_flags */
	"Abstract base objects",					/* tp_doc */
};


bool
Abstract_type_setup(PyObject *module)
{
	if (PyType_Ready(&Abstract_Type) < 0)
		return false;
	Py_INCREF(&Abstract_Type);
	PyModule_AddObject(module, "Abstract", (PyObject *)&Abstract_Type);
	return true;
}


SL_START_ABSTRACT_PROXY(Widget)
SL_END_ABSTRACT_PROXY(Widget)


#include "widget.moc"

