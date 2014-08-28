#include "slew.h"
#include "objects.h"

#include "sizer.h"

#include "constants/window.h"

#include <QDialog>
#include <QMainWindow>
#include <QFrame>
#include <QAbstractScrollArea>
#include <QGraphicsOpacityEffect>



void
setWindowStyle(QWidget *window, int style)
{
	if (style & SL_WINDOW_STYLE_SMALL)
		window->setAttribute(Qt::WA_MacSmallSize);
	else
		window->setAttribute(Qt::WA_MacNormalSize);
	
	if (QFrame *frame = qobject_cast<QFrame *>(window)) {
		if (style & (SL_WINDOW_STYLE_FRAMELESS | SL_WINDOW_STYLE_THIN)) {
			frame->setFrameStyle(QFrame::NoFrame);
		}
		else {
			int qstyle = 0;
			if (style & SL_WINDOW_STYLE_SUNKEN)
				qstyle = QFrame::Sunken;
			if (style & SL_WINDOW_STYLE_THICK) {
				frame->setMidLineWidth(1);
				qstyle = QFrame::Sunken;
			}
			if (style & SL_WINDOW_STYLE_RAISED)
				qstyle = QFrame::Raised;
			if (qstyle != 0)
				frame->setFrameStyle(qstyle);
		}
	}
	if (window->focusPolicy() != Qt::NoFocus)
		window->setAttribute(Qt::WA_MacShowFocusRect, style & SL_WINDOW_STYLE_NOFOCUS ? false : true);
	window->setAttribute(Qt::WA_TranslucentBackground, style & SL_WINDOW_STYLE_TRANSLUCENT ? true : false);
	window->setAttribute(Qt::WA_TransparentForMouseEvents, style & SL_WINDOW_STYLE_NOMOUSE ? true : false);
}


void
getWindowStyle(QWidget *window, int& style)
{
	if (window->testAttribute(Qt::WA_MacSmallSize))
		style |= SL_WINDOW_STYLE_SMALL;
	
	if (QFrame *frame = qobject_cast<QFrame *>(window)) {
		int qstyle = frame->frameStyle();
		if (!(qstyle & QFrame::StyledPanel)) {
			if (qstyle & QFrame::Sunken) {
				style |= SL_WINDOW_STYLE_SUNKEN;
				if (frame->midLineWidth() == 1)
					style |= SL_WINDOW_STYLE_THICK;
			}
			if (qstyle & QFrame::Raised)
				style |= SL_WINDOW_STYLE_RAISED;
		}
	}
	if (!window->testAttribute(Qt::WA_MacShowFocusRect))
		style |= SL_WINDOW_STYLE_NOFOCUS;
	if (window->testAttribute(Qt::WA_TranslucentBackground))
		style |= SL_WINDOW_STYLE_TRANSLUCENT;
	if (window->testAttribute(Qt::WA_TransparentForMouseEvents))
		style |= SL_WINDOW_STYLE_NOMOUSE;
}


PyObject *
insertWindowChild(QWidget *window, PyObject *object)
{
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		QWidget *widget = (QWidget *)child;
		Qt::WindowFlags flags = widget->windowFlags();
		
		if ((flags & Qt::WindowType_Mask) == Qt::Window)
// 		if ((!qobject_cast<QDialog *>(widget)) && (!qobject_cast<QMainWindow *>(widget)))
			flags &= ~Qt::WindowType_Mask;
		
// 		bool needsShow = !(widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide));
		widget->setParent(window, flags);
// 		if (needsShow)
// 			widget->setVisible(true);
		Py_RETURN_NONE;
	}
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		if (window->layout()) {
			SL_RETURN_DUP_SIZER;
		}
		window->setLayout(widget);
		Sizer_Impl::reparentChildren(widget, widget);
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_ATTACH;
}


PyObject *
removeWindowChild(QWidget *window, PyObject *object)
{
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		QWidget *widget = (QWidget *)child;
		widget->releaseMouse();
		widget->releaseKeyboard();
		widget->hide();
		widget->setParent(NULL);
		Py_RETURN_NONE;
	}
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		Sizer_Proxy *proxy = (Sizer_Proxy *)getProxy(object);
		
		SL_QAPP()->replaceProxyObject((Abstract_Proxy *)proxy, widget->clone());
		
		delete widget;
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_DETACH;
}


bool
setViewSelection(QAbstractItemView *view, PyObject *selection)
{
	PyAutoLocker locker;
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	QItemSelection selected;
	QModelIndex index;
	
	PyObject *seq = PySequence_Fast(selection, "expected sequence object");
	if (!seq)
		return false;
	Py_ssize_t i, size = PySequence_Fast_GET_SIZE(seq);
	for (i = 0; i < size; i++) {
		index = model->index(PySequence_Fast_GET_ITEM(seq, i));
		selected.select(index, index);
	}
	
	QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current;
	if (view->selectionBehavior() == QAbstractItemView::SelectRows)
		flags |= QItemSelectionModel::Rows;
	view->selectionModel()->select(selected, flags);
	
	Py_DECREF(seq);
	return true;
}


PyObject *
getViewSelection(QAbstractItemView *view)
{
	PyAutoLocker locker;
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	QItemSelection selected = view->selectionModel()->selection();
	QItemSelectionRange range;
	QModelIndexList list;
	QModelIndex index;
	PyObject *dataIndex, *selection;
	
	if (view->selectionMode() == QAbstractItemView::SingleSelection) {
		range = selected.value(0);
		if (range.isValid()) {
			selection = PyTuple_New(1);
			dataIndex = model->getDataIndex(range.topLeft());
			Py_INCREF(dataIndex);
			PyTuple_SET_ITEM(selection, 0, dataIndex);
		}
		else
			selection = PyTuple_New(0);
	}
	else {
		Py_ssize_t pos = 0;
		if (view->selectionBehavior() == QAbstractItemView::SelectItems)
			list = selected.indexes();
		else
			list = view->selectionModel()->selectedRows();
		
		selection = PyTuple_New((Py_ssize_t)list.size());
		foreach (index, list) {
			dataIndex = model->getDataIndex(index);
			Py_INCREF(dataIndex);
			PyTuple_SET_ITEM(selection, pos, dataIndex);
			pos++;
		}
	}
	return selection;
}


SL_DEFINE_METHOD(Window, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	return insertWindowChild(impl, object);
})


SL_DEFINE_METHOD(Window, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	return removeWindowChild(impl, object);
})


SL_DEFINE_METHOD(Window, set_updates_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	int count = impl->property("updates_disable_count").value<int>();
	
	if (enabled) {
		if (count > 0) {
			count--;
			if (count == 0)
				impl->setUpdatesEnabled(true);
		}
	}
	else {
		if (count == 0) {
			impl->setUpdatesEnabled(false);
		}
		count++;
	}
	impl->setProperty("updates_disable_count", QVariant::fromValue(count));
})


SL_DEFINE_METHOD(Window, set_visible, {
	bool visible;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &visible))
		return NULL;
	
	impl->setVisible(visible);
})


SL_DEFINE_METHOD(Window, is_visible, {
	return createBoolObject(impl->isVisible());
})


SL_DEFINE_METHOD(Window, set_focus, {
	bool focus;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &focus))
		return NULL;
	
	if (focus)
		impl->setFocus();
	else
		impl->clearFocus();
})


SL_DEFINE_METHOD(Window, has_focus, {
	return createBoolObject(impl->hasFocus());
})


SL_DEFINE_METHOD(Window, repaint, {
	QPoint tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPoint, &tl, convertPoint, &br))
		return NULL;
	
	if (impl->layout())
		impl->layout()->activate();
	
	QWidget *view;
	QAbstractScrollArea *scrollArea = qobject_cast<QAbstractScrollArea *>(impl);
	if (scrollArea)
		view = scrollArea->viewport();
	else
		view = impl;
	
	if ((tl.isNull()) && (br.isNull())) {
		view->update();
	}
	else {
		view->update(QRect(tl, br));
	}
// 	QApplication::sendPostedEvents();
})


SL_DEFINE_METHOD(Window, message_box, {
	QString message, title;
	int button, icon = SL_ICON_INFORMATION;
	PyObject *callback = NULL, *userdata = NULL, *buttons = NULL;
	
	if (!PyArg_ParseTuple(args, "O&O&OiOO", convertString, &message, convertString, &title, &buttons, &icon, &callback, &userdata))
		return NULL;
	
	messageBox(impl, title, message, buttons, icon, callback, userdata, &button);
	
	return PyInt_FromLong(button);
})


SL_DEFINE_METHOD(Window, map_to_local, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	return createVectorObject(impl->mapFromGlobal(pos));
})


SL_DEFINE_METHOD(Window, map_to_global, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	return createVectorObject(impl->mapToGlobal(pos));
})


SL_DEFINE_METHOD(Window, grab_mouse, {
	bool grab;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &grab))
		return NULL;
	
	grabMouse(impl, grab);
})


SL_DEFINE_METHOD(Window, set_opacity, {
	double opacity;
	
	if (!PyArg_ParseTuple(args, "d", &opacity))
		return NULL;
	
	if (impl->isWindow()) {
		impl->setWindowOpacity(opacity);
	}
	else {
		QGraphicsOpacityEffect *effect = (QGraphicsOpacityEffect *)impl->graphicsEffect();
		if ((!effect) && (opacity < 1.0))
			effect = new QGraphicsOpacityEffect;
		else if (opacity >= 1.0)
			effect = NULL;
		if (effect)
			effect->setOpacity(opacity);
		impl->setGraphicsEffect(effect);
	}
})


SL_DEFINE_METHOD(Window, popup_message, {
	QString text;
	int align, buttons;
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&ii", convertString, &text, &align, &buttons))
		return NULL;
	
	if ((Completer::isRunningOn(impl)) && (align == SL_BOTTOM))
		align = SL_TOP;
	
	switch (align) {
	case SL_LEFT:	pos = QPoint(0, impl->height() / 2); break;
	case SL_RIGHT:	pos = QPoint(impl->width() - 1, impl->height() / 2); break;
	case SL_TOP:	pos = QPoint(impl->width() / 2, 0); break;
	default:		pos = QPoint(impl->width() / 2, impl->height() - 1); break;
	}
	
	return PyInt_FromLong(showPopupMessage(impl, NULL, text, impl->mapToGlobal(pos), align, buttons));
})


SL_DEFINE_METHOD(Window, find_focus, {
	PyObject *focus = getObject(impl->focusWidget());
	if (!focus)
		Py_RETURN_NONE;
	
	return focus;
})


SL_DEFINE_METHOD(Window, set_shortcut, {
	QString key;
	PyObject *action;
	
	if (!PyArg_ParseTuple(args, "O&O", convertString, &key, &action))
		return NULL;
	
	QWidget *widget = impl->focusProxy();
	if (!widget)
		widget = impl;
	if (widget->isWindow())
		setShortcut(widget, key, Qt::WindowShortcut, action);
	else
		setShortcut(widget, key, Qt::WidgetWithChildrenShortcut, action);
})


SL_DEFINE_METHOD(Window, set_timeout, {
	Py_ssize_t size = PyTuple_Size(args);
	if (PyErr_Occurred())
		return NULL;
	
	if (size < 1) {
		PyErr_SetString(PyExc_ValueError, "missing parameter(s)");
		return NULL;
	}
	
	int timeout = PyInt_AsLong(PyTuple_GetItem(args, 0));
	if (PyErr_Occurred())
		return NULL;
	
	PyObject *func_args = PyTuple_GetSlice(args, 1, size);
	
	setTimeout(impl, timeout, NULL, func_args);
	
	Py_DECREF(func_args);
})


SL_DEFINE_METHOD(Window, fit, {
	if (impl->layout()) {
		impl->layout()->activate();
		impl->resize(impl->layout()->minimumSize());
	}
})


SL_DEFINE_METHOD(Window, render, {
	QPixmap pixmap(impl->size());
	impl->render(&pixmap);
	
	return createBitmapObject(pixmap);
})


SL_DEFINE_METHOD(Window, get_datatype, {
	return PyInt_FromLong(qvariant_cast<int>(impl->property("dataType")));
})


SL_DEFINE_METHOD(Window, set_datatype, {
	int datatype;
	
	if (!PyArg_ParseTuple(args, "i", &datatype))
		return NULL;
	
	impl->setProperty("dataType", QVariant::fromValue(datatype));
})


SL_DEFINE_METHOD(Window, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Window, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
})


SL_DEFINE_METHOD(Window, get_pos, {
	return createVectorObject(impl->pos());
})


SL_DEFINE_METHOD(Window, set_pos, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	impl->move(pos);
})


SL_DEFINE_METHOD(Window, get_size, {
	return createVectorObject(impl->size());
})


SL_DEFINE_METHOD(Window, set_size, {
	QSize size, minSize = impl->minimumSize(), sizeHint = impl->sizeHint();
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	if (minSize.isNull())
		minSize = impl->minimumSizeHint();
	
	if (size.width() <= 0)
		size.rwidth() = minSize.width();
	if (size.height() <= 0)
		size.rheight() = minSize.height();
	if (size.width() <= 0)
		size.rwidth() = sizeHint.width();
	if (size.height() <= 0)
		size.rheight() = sizeHint.height();
	
	impl->setProperty("explicitSize", QVariant::fromValue(size));
	impl->blockSignals(true);
	impl->resize(size);
	impl->blockSignals(false);
	impl->updateGeometry();
})


SL_DEFINE_METHOD(Window, get_minsize, {
	QSize size = impl->minimumSize();
	
	if (size.isNull())
		Py_RETURN_NONE;
	
	return createVectorObject(size);
})


SL_DEFINE_METHOD(Window, set_minsize, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
		
	if (size.width() < 0)
		size.rwidth() = 0;
	if (size.height() < 0)
		size.rheight() = 0;
	
	impl->setMinimumSize(size);
	impl->updateGeometry();
})


SL_DEFINE_METHOD(Window, get_maxsize, {
	QSize size = impl->maximumSize();
	
	if (size.isNull())
		Py_RETURN_NONE;
	
	return createVectorObject(size);
})


SL_DEFINE_METHOD(Window, set_maxsize, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
		
	if (size.width() < 0)
		size.rwidth() = QWIDGETSIZE_MAX;
	if (size.height() < 0)
		size.rheight() = QWIDGETSIZE_MAX;
	
	impl->setMaximumSize(size);
	impl->updateGeometry();
})


SL_DEFINE_METHOD(Window, get_tip, {
	return createStringObject(impl->toolTip());
})


SL_DEFINE_METHOD(Window, set_tip, {
	QString tip;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &tip))
		return NULL;
	
	impl->setToolTip(tip);
})


SL_DEFINE_METHOD(Window, is_enabled, {
	return createBoolObject(impl->isEnabled());
})


SL_DEFINE_METHOD(Window, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	impl->setEnabled(enabled);
})


SL_DEFINE_METHOD(Window, get_cursor, {
	QCursor cursor(impl->cursor());
	int id;
	
	switch (cursor.shape()) {
	case Qt::IBeamCursor:			id = SL_CURSOR_IBEAM; break;
	case Qt::PointingHandCursor:	id = SL_CURSOR_HAND; break;
	case Qt::WaitCursor:			id = SL_CURSOR_WAIT; break;
	case Qt::CrossCursor:			id = SL_CURSOR_CROSS; break;
	case Qt::BlankCursor:			id = SL_CURSOR_NONE; break;
	case Qt::ClosedHandCursor:		id = SL_CURSOR_MOVE; break;
	case Qt::SizeAllCursor:			id = SL_CURSOR_RESIZE; break;
	case Qt::SizeVerCursor:			id = SL_CURSOR_RESIZE_VERTICAL; break;
	case Qt::SizeHorCursor:			id = SL_CURSOR_RESIZE_HORIZONTAL; break;
	case Qt::SizeFDiagCursor:		id = SL_CURSOR_RESIZE_DIAGONAL_F; break;
	case Qt::SizeBDiagCursor:		id = SL_CURSOR_RESIZE_DIAGONAL_B; break;
	case Qt::OpenHandCursor:		id = SL_CURSOR_OPEN_HAND; break;
	default:						id = SL_CURSOR_NORMAL; break;
	}

	return PyInt_FromLong(id);
})


SL_DEFINE_METHOD(Window, set_cursor, {
	Qt::CursorShape cursor;
	int id;
	
	if (!PyArg_ParseTuple(args, "i", &id))
		return NULL;
	
	switch (id) {
	case SL_CURSOR_IBEAM:				cursor = Qt::IBeamCursor; break;
	case SL_CURSOR_HAND:				cursor = Qt::PointingHandCursor; break;
	case SL_CURSOR_WAIT:				cursor = Qt::WaitCursor; break;
	case SL_CURSOR_CROSS:				cursor = Qt::CrossCursor; break;
	case SL_CURSOR_NONE:				cursor = Qt::BlankCursor; break;
	case SL_CURSOR_MOVE:				cursor = Qt::ClosedHandCursor; break;
	case SL_CURSOR_RESIZE:				cursor = Qt::SizeAllCursor; break;
	case SL_CURSOR_RESIZE_VERTICAL:		cursor = Qt::SizeVerCursor; break;
	case SL_CURSOR_RESIZE_HORIZONTAL:	cursor = Qt::SizeHorCursor; break;
	case SL_CURSOR_RESIZE_DIAGONAL_F:	cursor = Qt::SizeFDiagCursor; break;
	case SL_CURSOR_RESIZE_DIAGONAL_B:	cursor = Qt::SizeBDiagCursor; break;
	case SL_CURSOR_OPEN_HAND:			cursor = Qt::OpenHandCursor; break;
	default:
	case SL_CURSOR_NORMAL:				cursor = Qt::ArrowCursor; break;
	}
	
	impl->setCursor(QCursor(cursor));
})


SL_DEFINE_METHOD(Window, get_color, {
	QColor color = impl->palette().windowText().color();
	if (color == QApplication::palette(impl).windowText().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(Window, set_color, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(impl->isEnabled() ? QPalette::Normal : QPalette::Disabled, QPalette::WindowText);
	}
	
	palette.setColor(QPalette::WindowText, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(Window, get_bgcolor, {
	QColor color;
	if (qobject_cast<QAbstractScrollArea *>(impl)) {
		color = impl->palette().base().color();
		if (color == QApplication::palette(impl).base().color())
			Py_RETURN_NONE;
	}
	else {
		color = impl->palette().window().color();
		if (color == QApplication::palette(impl).window().color())
			Py_RETURN_NONE;
	}
	return createColorObject(color);
})


SL_DEFINE_METHOD(Window, set_bgcolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		impl->setAutoFillBackground(false);
		if (qobject_cast<QAbstractScrollArea *>(impl))
			color = QApplication::palette(impl).color(QPalette::Base);
		else
			color = QApplication::palette(impl).color(QPalette::Window);
	}
	else {
		impl->setAutoFillBackground(true);
	}
	
	if (qobject_cast<QAbstractScrollArea *>(impl))
		palette.setColor(QPalette::Base, color);
	else
		palette.setColor(QPalette::Window, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(Window, get_hicolor, {
	QColor color = impl->palette().highlightedText().color();
	if (color == QApplication::palette(impl).highlightedText().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(Window, set_hicolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(QPalette::HighlightedText);
	}
	
	palette.setColor(QPalette::HighlightedText, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(Window, get_hibgcolor, {
	QColor color = impl->palette().highlight().color();
	if (color == QApplication::palette(impl).highlight().color())
		Py_RETURN_NONE;
	return createColorObject(color);
})


SL_DEFINE_METHOD(Window, set_hibgcolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(QPalette::Highlight);
	}
	
	palette.setColor(QPalette::Highlight, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(Window, get_font, {
	return createFontObject(impl->font());
})


SL_DEFINE_METHOD(Window, set_font, {
	QFont font;
	
	if (!PyArg_ParseTuple(args, "O&", convertFont, &font))
		return NULL;
	
	font.resolve(impl->font());
	impl->setFont(font);
})


SL_DEFINE_METHOD(Window, get_margins, {
	QMargins margins = qvariant_cast<QMargins>(impl->property("boxMargins"));
	
	return Py_BuildValue("(iiii)", margins.top(), margins.right(), margins.bottom(), margins.left());
})


SL_DEFINE_METHOD(Window, set_margins, {
	QList<int> margins;
	int top = 0, right = 0, bottom = 0, left = 0;
	
	if (!PyArg_ParseTuple(args, "O&", convertIntList, &margins))
		return NULL;
	
	if (margins.size() == 1) {
		right = bottom = left = top = margins[0];
	}
	else if (margins.size() == 2) {
		top = bottom = margins[0];
		left = right = margins[1];
	}
	else if (margins.size() == 3) {
		top = margins[0];
		left = right = margins[1];
		bottom = margins[2];
	}
	else if (margins.size() >= 4) {
		top = margins[0];
		right = margins[1];
		bottom = margins[2];
		left = margins[3];
	}
	
	QVariant v;
	v.setValue(QMargins(left, top, right, bottom));
	impl->setProperty("boxMargins", v);
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->invalidate();
})


SL_DEFINE_METHOD(Window, get_boxalign, {
	unsigned long value = 0;
	
	Qt::Alignment alignment = qvariant_cast<Qt::Alignment>(impl->property("boxAlign"));
	if (alignment == (Qt::Alignment)0) {
		value = SL_LAYOUTABLE_BOXALIGN_EXPAND;
	}
	else {
		switch (alignment & Qt::AlignHorizontal_Mask) {
		case Qt::AlignRight:	value |= SL_ALIGN_RIGHT; break;
		case Qt::AlignHCenter:	value |= SL_ALIGN_HCENTER; break;
		default:				break;
		}
		switch (alignment & Qt::AlignVertical_Mask) {
		case Qt::AlignBottom:	value |= SL_ALIGN_BOTTOM; break;
		case Qt::AlignVCenter:	value |= SL_ALIGN_VCENTER; break;
		default:				break;
		}
	}
	
	return PyLong_FromUnsignedLong(value);
})


SL_DEFINE_METHOD(Window, set_boxalign, {
	unsigned int value;
	Qt::Alignment alignment = (Qt::Alignment)0;
	
	if (!PyArg_ParseTuple(args, "I", &value))
		return NULL;
	
	if (value != SL_LAYOUTABLE_BOXALIGN_EXPAND) {
		switch (value & SL_ALIGN_HMASK) {
		case SL_ALIGN_HCENTER:	alignment |= Qt::AlignHCenter; break;
		case SL_ALIGN_RIGHT:	alignment |= Qt::AlignRight; break;
		default:				alignment |= Qt::AlignLeft; break;
		}
		switch (value & SL_ALIGN_VMASK) {
		case SL_ALIGN_VCENTER:	alignment |= Qt::AlignVCenter; break;
		case SL_ALIGN_BOTTOM:	alignment |= Qt::AlignBottom; break;
		default:				alignment |= Qt::AlignTop; break;
		}
	}
	
	impl->setProperty("boxAlign", QVariant::fromValue(alignment));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->setAlignment(impl, alignment);
})


SL_DEFINE_METHOD(Window, get_cell, {
	return createVectorObject(qvariant_cast<QPoint>(impl->property("layoutCell")));
})


SL_DEFINE_METHOD(Window, set_cell, {
	QPoint cell;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &cell))
		return NULL;
	
	impl->setProperty("layoutCell", QVariant::fromValue(cell));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(Window, get_span, {
	return createVectorObject((qvariant_cast<QSize>(impl->property("layoutSpan"))).expandedTo(QSize(1,1)));
})


SL_DEFINE_METHOD(Window, set_span, {
	QSize span;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &span))
		return NULL;
	
	impl->setProperty("layoutSpan", QVariant::fromValue(span));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(Window, get_prop, {
	return PyInt_FromLong(qvariant_cast<int>(impl->property("layoutProp")));
})


SL_DEFINE_METHOD(Window, set_prop, {
	int prop;
	
	if (!PyArg_ParseTuple(args, "i", &prop))
		return NULL;
	
	impl->setProperty("layoutProp", QVariant::fromValue(prop));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent) {
		QPoint cell = qvariant_cast<QPoint>(impl->property("layoutCell"));
		if (parent->orientation() == Qt::Horizontal) {
			parent->setColumnMinimumWidth(cell.x(), 0);
			parent->setColumnStretch(cell.x(), prop);
		}
		else if (parent->orientation() == Qt::Vertical) {
			parent->setRowMinimumHeight(cell.y(), 0);
			parent->setRowStretch(cell.y(), prop);
		}
	}
})



SL_START_ABSTRACT_PROXY_DERIVED(Window, Widget)
SL_METHOD(insert)
SL_METHOD(remove)

SL_METHOD(set_updates_enabled)
SL_METHOD(set_visible)
SL_METHOD(is_visible)
SL_METHOD(set_focus)
SL_METHOD(has_focus)
SL_METHOD(repaint)
SL_METHOD(message_box)
SL_METHOD(map_to_local)
SL_METHOD(map_to_global)
SL_METHOD(grab_mouse)
SL_METHOD(set_opacity)
SL_METHOD(popup_message)
SL_METHOD(find_focus)
SL_METHOD(set_shortcut)
SL_METHOD(set_timeout)
SL_METHOD(fit)
SL_METHOD(render)

SL_PROPERTY(datatype)
SL_PROPERTY(style)
SL_PROPERTY(pos)
SL_PROPERTY(size)
SL_PROPERTY(minsize)
SL_PROPERTY(maxsize)
SL_PROPERTY(tip)
SL_BOOL_PROPERTY(enabled)
SL_PROPERTY(cursor)
SL_PROPERTY(color)
SL_PROPERTY(bgcolor)
SL_PROPERTY(hicolor)
SL_PROPERTY(hibgcolor)
SL_PROPERTY(font)

SL_PROPERTY(margins)
SL_PROPERTY(boxalign)
SL_PROPERTY(cell)
SL_PROPERTY(span)
SL_PROPERTY(prop)
SL_END_ABSTRACT_PROXY_DERIVED(Window, Widget)


#include "window.moc"

