#include "slew.h"

#include "stackview.h"
#include "sizer.h"
#include "flowbox.h"



StackView_Impl::StackView_Impl()
	: QStackedWidget(), WidgetInterface()
{
}


SL_DEFINE_METHOD(StackView, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QWidget *widget = NULL;
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		widget = (QWidget *)child;
	}
	else if ((isSizer(object)) || (isFlowBox(object))) {
		AbstractSizer_Impl *sizer = (AbstractSizer_Impl *)child;
		QLayout *layout = (QLayout *)child;
		widget = new QWidget(impl);
		widget->setLayout(layout);
		sizer->doReparentChildren(layout, layout);
		layout->invalidate();
	}
	else
		SL_RETURN_CANNOT_ATTACH;
	
	index = impl->insertWidget(index, widget);
})


SL_DEFINE_METHOD(StackView, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		impl->removeWidget((QWidget *)child);
	}
	else if ((isSizer(object)) || (isFlowBox(object))) {
		AbstractSizer_Impl *widget = (AbstractSizer_Impl *)child;
		Sizer_Proxy *proxy = (Sizer_Proxy *)getProxy(object);
		int i;
		
		SL_QAPP()->replaceProxyObject((Abstract_Proxy *)proxy, widget->clone());
		
		for (i = 0; i < impl->count(); i++) {
			if (impl->widget(i)->layout() == (QLayout *)widget) {
				QWidget *panel = impl->widget(i);
				impl->removeWidget(panel);
				delete panel;
				break;
			}
		}
	}
	else
		SL_RETURN_CANNOT_DETACH;
})


SL_DEFINE_METHOD(StackView, get_page, {
	return PyInt_FromLong(impl->currentIndex());
})


SL_DEFINE_METHOD(StackView, set_page, {
	int page;
	
	if (!PyArg_ParseTuple(args, "i", &page))
		return NULL;
	
	if ((page < 0) || (page >= impl->count())) {
		PyErr_SetString(PyExc_ValueError, "invalid page number");
		return NULL;
	}
	
	impl->setCurrentIndex(page);
})



SL_START_PROXY_DERIVED(StackView, Window)
SL_METHOD(insert)
SL_METHOD(remove)

SL_PROPERTY(page)
SL_END_PROXY_DERIVED(StackView, Window)


#include "stackview.moc"
#include "stackview_h.moc"

