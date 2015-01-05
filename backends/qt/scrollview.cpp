#include "slew.h"

#include "scrollview.h"
#include "sizer.h"

#include <QContextMenuEvent>
#include <QScrollBar>


class Content : public QWidget
{
	Q_OBJECT
	
public:
	Content(QWidget *parent) : QWidget(parent) { setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding); }
	
	void contextMenuEvent(QContextMenuEvent *event) { emit contextMenu(); }
	
signals:
	void contextMenu();
};



ScrollView_Impl::ScrollView_Impl()
	: QScrollArea(), WidgetInterface()
{
	Content *content = new Content(this);
	connect(content, SIGNAL(contextMenu()), this, SLOT(handleContextMenu()));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(handleScrollBars()));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(handleScrollBars()));
	
	QPalette palette = this->palette();
	palette.setColor(QPalette::Window, Qt::transparent);
	setPalette(palette);

	setWidget(content);
	setWidgetResizable(true);
}


void
ScrollView_Impl::handleContextMenu()
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("pos", QCursor::pos());
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.run();
	}
}


void
ScrollView_Impl::handleScrollBars()
{
	EventRunner runner(this, "onScroll");
	if (runner.isValid()) {
		QScrollBar *hsb = horizontalScrollBar();
		QScrollBar *vsb = verticalScrollBar();
		
		runner.set("minimum", QPoint(hsb->minimum(), vsb->minimum()));
		runner.set("maximum", QPoint(hsb->maximum(), vsb->maximum()));
		runner.set("value", QPoint(hsb->value(), vsb->value()));
		runner.run();
	}
}


SL_DEFINE_METHOD(ScrollView, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	PyObject *result = insertWindowChild(impl->widget(), object);
	if ((result) && (isSizer(object))) {
		Sizer_Impl *sizer = (Sizer_Impl *)getImpl(object);
		if (sizer)
			sizer->setSizeConstraint(QLayout::SetMinAndMaxSize);
	}
	return result;
})


SL_DEFINE_METHOD(ScrollView, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (isSizer(object)) {
		Sizer_Impl *sizer = (Sizer_Impl *)getImpl(object);
		if (sizer)
			sizer->setSizeConstraint(QLayout::SetDefaultConstraint);
	}
	return removeWindowChild(impl->widget(), object);
})


SL_DEFINE_METHOD(ScrollView, get_bgcolor, {
	QColor color;
	color = impl->widget()->palette().window().color();
	if (color == QApplication::palette(impl).window().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(ScrollView, set_bgcolor, {
	QPalette palette(impl->widget()->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		impl->widget()->setAutoFillBackground(false);
		color = QApplication::palette(impl).color(QPalette::Window);
	}
	else {
		impl->widget()->setAutoFillBackground(true);
	}
	
	palette.setColor(QPalette::Window, color);
	impl->widget()->setPalette(palette);
})


SL_DEFINE_METHOD(ScrollView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
		style |= SL_SCROLLVIEW_STYLE_HORIZONTAL;
	if (impl->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
		style |= SL_SCROLLVIEW_STYLE_VERTICAL;
	if (impl->frameStyle() == QFrame::NoFrame)
		style |= SL_WINDOW_STYLE_FRAMELESS | SL_WINDOW_STYLE_THIN;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(ScrollView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setHorizontalScrollBarPolicy(style & SL_SCROLLVIEW_STYLE_HORIZONTAL ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	impl->setVerticalScrollBarPolicy(style & SL_SCROLLVIEW_STYLE_VERTICAL ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
	impl->setFocusPolicy(style & SL_WINDOW_STYLE_NOFOCUS ? Qt::NoFocus : Qt::StrongFocus);
	if (style & (SL_WINDOW_STYLE_FRAMELESS | SL_WINDOW_STYLE_THIN)) {
		impl->setFrameStyle(QFrame::NoFrame);
		impl->setAutoFillBackground(false);
		impl->setAttribute(Qt::WA_NoSystemBackground, true);
	}
	else {
		impl->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
		impl->setAutoFillBackground(true);
		impl->setAttribute(Qt::WA_NoSystemBackground, false);
	}
})


SL_DEFINE_METHOD(ScrollView, get_rate, {
	return createVectorObject(QPoint(impl->horizontalScrollBar()->singleStep(), impl->verticalScrollBar()->singleStep()));
})


SL_DEFINE_METHOD(ScrollView, set_rate, {
	QPoint rate;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &rate))
		return NULL;
	
	impl->horizontalScrollBar()->setSingleStep(rate.x());
	impl->verticalScrollBar()->setSingleStep(rate.y());
})


SL_DEFINE_METHOD(ScrollView, get_scroll, {
	return createVectorObject(QPoint(impl->horizontalScrollBar()->value(), impl->verticalScrollBar()->value()));
})


SL_DEFINE_METHOD(ScrollView, set_scroll, {
	QPoint scroll;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &scroll))
		return NULL;
	
	impl->ensureVisible(scroll.x(), scroll.y(), 0, 0);
})


SL_DEFINE_METHOD(ScrollView, get_viewsize, {
	return createVectorObject(impl->widget()->size());
})


SL_DEFINE_METHOD(ScrollView, set_viewsize, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	impl->widget()->resize(size);
})



SL_START_PROXY_DERIVED(ScrollView, Window)
SL_METHOD(insert)
SL_METHOD(remove)
SL_PROPERTY(bgcolor)

SL_PROPERTY(style)
SL_PROPERTY(rate)
SL_PROPERTY(scroll)
SL_PROPERTY(viewsize)
SL_END_PROXY_DERIVED(ScrollView, Window)


#include "scrollview.moc"
#include "scrollview_h.moc"

