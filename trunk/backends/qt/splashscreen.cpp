#include "slew.h"

#include "splashscreen.h"

#include <QResizeEvent>
#include <QHideEvent>


SplashScreen_Impl::SplashScreen_Impl()
	: QWidget(NULL, Qt::SplashScreen), WidgetInterface()
{
}


void
SplashScreen_Impl::moveEvent(QMoveEvent *event)
{
	hidePopupMessage();
	Completer::hide();
}


void
SplashScreen_Impl::resizeEvent(QResizeEvent *event)
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
SplashScreen_Impl::showEvent(QShowEvent *event)
{
	PyAutoLocker locker;
	getObject(this);	// increases refcount
}


void
SplashScreen_Impl::hideEvent(QHideEvent *event)
{
	setParent(NULL, Qt::SplashScreen);
	PyAutoLocker locker;
	PyObject *object = getObject(this, false);
	Py_XDECREF(object);
	PyErr_Clear();
}


void
SplashScreen_Impl::closeEvent(QCloseEvent *event)
{
	EventRunner runner(this, "onClose");
	if (runner.isValid()) {
		runner.set("accepted", true);
		runner.run();
	}
}


SL_DEFINE_METHOD(SplashScreen, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	return insertWindowChild(impl, object);
})


SL_DEFINE_METHOD(SplashScreen, remove, {
	(void)impl;
	
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	return removeWindowChild(impl, object);
})


SL_DEFINE_METHOD(SplashScreen, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(SplashScreen, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
})



SL_START_PROXY_DERIVED(SplashScreen, Frame)
SL_METHOD(insert)
SL_METHOD(remove)

SL_PROPERTY(style)
SL_END_PROXY_DERIVED(SplashScreen, Frame)


#include "splashscreen.moc"
#include "splashscreen_h.moc"

