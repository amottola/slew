#include "slew.h"

#include "statusbar.h"
#include "sizer.h"

#include <QBoxLayout>
#include <QChildEvent>
#include <QMainWindow>


StatusBar_Impl::StatusBar_Impl()
	: QStatusBar(), WidgetInterface()
{
}


#ifdef Q_OS_MAC

static void
set_font_helper(QObject *object, const QFont& font)
{
	QWidget *w = qobject_cast<QWidget *>(object);
	if (w) {
		w->setFont(font);
	}
	foreach (QObject *child, object->children())
		set_font_helper(child, font);
}

#endif


bool
StatusBar_Impl::event(QEvent *event)
{
#ifdef Q_OS_MAC
	QMainWindow *win = qobject_cast<QMainWindow *>(window());
	if ((win) && (win->unifiedTitleAndToolBarOnMac())) {
		if (event->type() == QEvent::ChildAdded) {
			QChildEvent *e = (QChildEvent *)event;
			set_font_helper(e->child(), font());
		}
		else if (event->type() == QEvent::FontChange) {
			foreach (QObject *child, children()) {
				set_font_helper(child, font());
			}
		}
		else if (event->type() == QEvent::ParentChange) {
			QFont font = QApplication::font(this);
			font.setPointSize(11);
			setFont(font);
			foreach (QObject *child, children()) {
				set_font_helper(child, font);
			}
		}
	}
#endif
	return QStatusBar::event(event);
}


QList<int>
StatusBar_Impl::getProps()
{
	QList<int> props;
	
	foreach (Field field, fFields)
		props.append(field.fProp);
	
	return props;
}


void
StatusBar_Impl::setProps(const QList<int>& props)
{
	for (int i = 0; i < props.size(); i++) {
		if (i >= fFields.size())
			fFields.append(Field());
		int prop = props[i];
		if (prop != fFields[i].fProp) {
			fFields[i].fProp = prop;
			QWidget *widget = fFields[i].fWidget;
			if (widget) {
				bool visible = !(widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide));
				removeWidget(widget);
				insertWidget(i, widget, prop);
				if (visible)
					widget->setVisible(visible);
			}
		}
	}
}


int
StatusBar_Impl::getProp(int index)
{
	Field field = fFields.value(index);
	return field.fProp;
}


void
StatusBar_Impl::setProp(int index, int prop)
{
	while (fFields.size() <= index) {
		fFields.append(Field());
	}
	fFields[index].fProp = prop;
	setProps(getProps());
}


StatusBar_Impl *
StatusBar_Impl::clone()
{
	StatusBar_Impl *copy = new StatusBar_Impl();
	
	foreach (Field field, fFields) {
		QWidget *widget = field.fWidget;
		if (widget) {
			bool visible = !(widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide));
			removeWidget(widget);
			copy->addWidget(widget, field.fProp);
			if (visible)
				widget->setVisible(visible);
		}
		copy->fFields.append(field);
	}
	fFields.clear();
	
	return copy;
}


SL_DEFINE_METHOD(StatusBar, insert, {
	int index;
	PyObject *object;
	QWidget *panel = NULL;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	int prop = impl->getProp(index);
	
	if (isWindow(object)) {
		panel = (QWidget *)child;
		impl->insertWidget(index, panel, prop);
	}
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		panel = new QWidget;
		panel->setLayout(widget);
		Sizer_Impl::reparentChildren(widget, widget);
		impl->insertWidget(index, panel, prop);
		panel->show();
	}
	if (panel) {
		QList<Field> *fields = impl->fields();
		while (fields->size() <= index)
			fields->append(Field());
		fields->replace(index, Field(panel, prop));
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(StatusBar, remove, {
	PyObject *object;
	QWidget *panel = NULL;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		panel = (QWidget *)child;
		
		impl->removeWidget(panel);
	}
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		Sizer_Proxy *proxy = (Sizer_Proxy *)getProxy(object);
		panel = widget->parentWidget();
		
		SL_QAPP()->replaceProxyObject((Abstract_Proxy *)proxy, widget->clone());
		
		impl->removeWidget(panel);
		
		delete panel;
	}
	if (panel) {
		QList<Field> *fields = impl->fields();
		for (int i = 0; i < fields->size(); i++) {
			if (fields->at(i).fWidget == panel) {
				fields->replace(i, Field(NULL, impl->getProp(i)));
				break;
			}
		}
		Py_RETURN_NONE;
	}
	
	PyErr_Format(PyExc_RuntimeError, "cannot detach widget");
	return NULL;
})


SL_DEFINE_METHOD(StatusBar, get_prop, {
	int index;
	
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	return PyInt_FromLong(impl->getProp(index));
})


SL_DEFINE_METHOD(StatusBar, set_prop, {
	int index, prop;
	
	if (!PyArg_ParseTuple(args, "ii", &index, &prop))
		return NULL;
	
	impl->setProp(index, prop);
})


SL_DEFINE_METHOD(StatusBar, get_props, {
	QList<int> props = impl->getProps();
	return createIntListObject(props);
})


SL_DEFINE_METHOD(StatusBar, set_props, {
	QList<int> props;
	
	if (!PyArg_ParseTuple(args, "O&", convertIntList, &props))
		return NULL;
	
	impl->setProps(props);
})


SL_DEFINE_METHOD(StatusBar, get_text, {
	return createStringObject(impl->currentMessage());
})


SL_DEFINE_METHOD(StatusBar, set_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	if (text.isEmpty())
		impl->clearMessage();
	else
		impl->showMessage(text);
})


SL_START_PROXY_DERIVED(StatusBar, Window)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(get_prop)
SL_METHOD(set_prop)

SL_PROPERTY(props)
SL_PROPERTY(text)
SL_END_PROXY_DERIVED(StatusBar, Window)


#include "statusbar.moc"
#include "statusbar_h.moc"

