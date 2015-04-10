#include "slew.h"

#include "splitview.h"
#include "sizer.h"
#include "flowbox.h"

#include <QCursor>
#include <QSplitterHandle>
#include <QPaintEvent>
#include <QPainter>
#include <QPalette>
#include <QSizePolicy>


class SplitterHandle : public QSplitterHandle
{
public:
	SplitterHandle(Qt::Orientation orientation, SplitView_Impl *splitter)
		: QSplitterHandle(orientation, splitter), fSplitter(splitter)
	{
	}
	
	void paintEvent(QPaintEvent *event)
	{
		if (fSplitter->handleWidth() != 1) {
			QSplitterHandle::paintEvent(event);
		}
		else {
			QPainter painter(this);
			QColor color = palette().windowText().color();
			if (color == QApplication::palette(this).windowText().color()) {
#ifdef Q_OS_MAC
				color = QColor(0x51, 0x51, 0x51);
#else
				color = QColor(0x80, 0x80, 0x80);
#endif
			}
			painter.fillRect(contentsRect(), color);
		}
	}
	
	void moveSplitter(int pos)
	{
		fSplitter->setPolicies(true);
		QSplitterHandle::moveSplitter(pos);
		fSplitter->setPolicies();
	}
	
private:
	SplitView_Impl		*fSplitter;
};



SplitView_Impl::SplitView_Impl()
	: QSplitter(), WidgetInterface()
{
	setChildrenCollapsible(false);
	connect(this, SIGNAL(splitterMoved(int, int)), this, SLOT(handleSplitterMoved(int, int)));
}


QSplitterHandle *
SplitView_Impl::createHandle()
{
	return new SplitterHandle(orientation(), this);
}


void
SplitView_Impl::handleSplitterMoved(int pos, int index)
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", pos);
		runner.set("index", index);
		runner.run();
	}
}


void
SplitView_Impl::setPolicies(bool restore)
{
	QSizePolicy policy;
	int count = this->count();
	
	while (fProps.size() < count) {
		if (fProps.empty())
			fProps.append(0);
		else
			fProps.append(1);
	}
	
	if (restore) {
		for (int i = 0; i < count; i++) {
			QWidget *widget = this->widget(i);
			policy = widget->property("old_sp").value<QSizePolicy>();
			widget->setSizePolicy(policy);
		}
	}
	else {
		for (int i = 0; i < count; i++) {
			int prop = fProps.value(i);
			if (prop == 0) {
				policy.setHorizontalPolicy(QSizePolicy::Preferred);
				policy.setVerticalPolicy(QSizePolicy::Preferred);
			}
			else {
				policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
				policy.setVerticalPolicy(QSizePolicy::MinimumExpanding);
			}
			policy.setHorizontalStretch(prop);
			policy.setVerticalStretch(prop);
			widget(i)->setSizePolicy(policy);
		}
	}
}



SL_DEFINE_METHOD(SplitView, insert, {
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
		
	if (widget) {
		impl->insertWidget(index, widget);
		widget->show();

		widget->setProperty("old_sp", QVariant(widget->sizePolicy()));
		impl->setPolicies();
		
		QList<int> sizes = impl->storedSizes();
		if (sizes.count() > 0)
			impl->QSplitter::setSizes(sizes);
		
		Py_RETURN_NONE;
	}
	SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(SplitView, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QWidget *widget = NULL;
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		widget = (QWidget *)child;
		widget->releaseMouse();
		widget->releaseKeyboard();
		widget->hide();
		widget->setParent(NULL);
		widget->setSizePolicy(widget->property("old_sp").value<QSizePolicy>());
		impl->setPolicies();
		Py_RETURN_NONE;
	}
	else if ((isSizer(object)) || (isFlowBox(object))) {
		AbstractSizer_Impl *widget = (AbstractSizer_Impl *)child;
		Sizer_Proxy *proxy = (Sizer_Proxy *)getProxy(object);
		int i;
		
		SL_QAPP()->replaceProxyObject((Abstract_Proxy *)proxy, widget->clone());
		
		for (i = 0; i < impl->count(); i++) {
			if (impl->widget(i)->layout() == (QLayout *)widget) {
				impl->widget(i)->hide();
				delete impl->widget(i);
				break;
			}
		}
		impl->setPolicies();
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_DETACH;
})


SL_DEFINE_METHOD(SplitView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->handleWidth() == 1)
		style |= SL_WINDOW_STYLE_THIN;
	if (impl->orientation() == Qt::Vertical)
		style |= SL_SPLITVIEW_STYLE_HORIZONTAL;
	else
		style |= SL_SPLITVIEW_STYLE_VERTICAL;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(SplitView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setHandleWidth(style & SL_WINDOW_STYLE_THIN ? 1 : 3);
	impl->setOrientation(style & SL_SPLITVIEW_STYLE_HORIZONTAL ? Qt::Vertical : Qt::Horizontal);
})


SL_DEFINE_METHOD(SplitView, get_sizes, {
	return createIntListObject(impl->sizes());
})


SL_DEFINE_METHOD(SplitView, set_sizes, {
	QList<int> sizes;
	
	if (!PyArg_ParseTuple(args, "O&", convertIntList, &sizes))
		return NULL;
	
	impl->setSizes(sizes);
})


SL_DEFINE_METHOD(SplitView, get_props, {
	return createIntListObject(impl->props());
})


SL_DEFINE_METHOD(SplitView, set_props, {
	QList<int> props;
	
	if (!PyArg_ParseTuple(args, "O&", convertIntList, &props))
		return NULL;
	
	impl->setProps(props);
})


SL_START_PROXY_DERIVED(SplitView, Window)
SL_METHOD(insert)
SL_METHOD(remove)

SL_PROPERTY(style)
SL_PROPERTY(sizes)
SL_PROPERTY(props)
SL_END_PROXY_DERIVED(SplitView, Window)


#include "splitview.moc"
#include "splitview_h.moc"

