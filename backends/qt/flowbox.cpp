#include "slew.h"

#include "flowbox.h"
#include "sizer.h"
#include "constants/window.h"

#include <QWidgetItem>
#include <QSizePolicy>


FlowBox_Impl::FlowBox_Impl()
	: QLayout(), WidgetInterface(), AbstractSizerInterface(), fOrientation(Qt::Horizontal)
{
	setContentsMargins(0, 0, 0, 0);
}


QLayoutItem *
FlowBox_Impl::takeAt(int index)
{
	if ((index >= 0) && (index < fItems.size()))
		return fItems.takeAt(index);
	else
		return NULL;
}


void
FlowBox_Impl::setGeometry(const QRect &rect)
{
	QLayout::setGeometry(rect);
	doLayout(rect, false);
}


QSize
FlowBox_Impl::minimumSize() const
{
	QSize size;
	int left, top, right, bottom;

	getContentsMargins(&left, &top, &right, &bottom);
	foreach (QLayoutItem *item, fItems) {
		size = size.expandedTo(item->minimumSize());
	}
	size += QSize(left + right, top + bottom);
	return size;
}


int
FlowBox_Impl::doLayout(const QRect& rect, bool testOnly) const
{
	int left, top, right, bottom;
	getContentsMargins(&left, &top, &right, &bottom);
	QRect effectiveRect = rect.adjusted(left, top, -right, -bottom);
	int x = effectiveRect.x();
	int y = effectiveRect.y();
	int lineWidth = 0;
	int lineHeight = 0;

	foreach (QLayoutItem *item, fItems) {
		if (fOrientation == Qt::Horizontal) {
			int nextX = x + item->sizeHint().width() + fSpacing.x();
			if ((nextX - fSpacing.x() > effectiveRect.right()) && (lineHeight > 0)) {
				x = effectiveRect.x();
				y = y + lineHeight + fSpacing.y();
				nextX = x + item->sizeHint().width() + fSpacing.x();
				lineHeight = 0;
			}
			if (!testOnly)
				item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
			x = nextX;
		}
		else {
			int nextY = y + item->sizeHint().height() + fSpacing.y();
			if ((nextY - fSpacing.y() > effectiveRect.bottom()) && (lineWidth > 0)) {
				y = effectiveRect.y();
				x = x + lineWidth + fSpacing.x();
				nextY = y + item->sizeHint().height() + fSpacing.y();
				lineWidth = 0;
			}
			if (!testOnly)
				item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));
			y = nextY;
		}
		lineWidth = qMax(lineWidth, item->sizeHint().width());
		lineHeight = qMax(lineHeight, item->sizeHint().height());
	}
	return y + lineHeight - rect.y() + bottom;
}


void
FlowBox_Impl::doReparentChildren(QLayoutItem *item, QLayout *parent)
{
	QLayout *layout = qobject_cast<QLayout *>(item->layout());
	QWidget *widget = qobject_cast<QWidget *>(item->widget());
	
	if (layout) {
		for (int i = 0; i < layout->count(); i++) {
			QLayoutItem *child = layout->itemAt(i);
			if (child->layout())
				child->layout()->setParent(layout);
			doReparentChildren(child, layout);
		}
	}
	else if (widget) {
		QWidget *parentWidget = parent ? parent->parentWidget() : NULL;
		bool needsShow = (parentWidget) && (parentWidget->isVisible()) && (!(widget->isHidden() && widget->testAttribute(Qt::WA_WState_ExplicitShowHide)));
		
		widget->setParent(parentWidget);
		if (needsShow)
			widget->show();
	}
}


QLayout *
FlowBox_Impl::clone()
{
	FlowBox_Impl *flowbox = new FlowBox_Impl;
	
	QPoint cell = qvariant_cast<QPoint>(property("layoutCell"));
	QSize span = qvariant_cast<QSize>(property("layoutSpan")).expandedTo(QSize(1,1));
	
	flowbox->setProperty("layoutCell", QVariant::fromValue(cell));
	flowbox->setProperty("layoutSpan", QVariant::fromValue(span));
	flowbox->setAlignment(alignment());
	
	while (count() > 0) {
		QLayoutItem *item = takeAt(0);
		if (item->layout()) {
			flowbox->addItem(item->layout());
		}
		else {
			flowbox->addWidget(item->widget());
			delete item;
		}
	}
	
	flowbox->setSpacing(spacing());
	flowbox->setOrientation(orientation());
	
	doReparentChildren(flowbox, flowbox);
	return flowbox;
}




SL_DEFINE_METHOD(FlowBox, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	bool ok = false;
	
	if (isWindow(object)) {
		Window_Impl *widget = (Window_Impl *)child;
		impl->addItem(new WidgetItem(widget));
		ok = true;
	}
	else if ((isSizer(object)) || (isFlowBox(object))) {
		QLayout *widget = (QLayout *)child;
		widget->setParent(impl);
		impl->addItem(widget);
		ok = true;
	}
	impl->doReparentChildren(impl, impl);
	
	if (ok) {
		QVariant v;
		v.setValue((QObject *)impl);
		child->setProperty("parentLayout", v);
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(FlowBox, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (isWindow(object)) {
		Window_Impl *widget = (Window_Impl *)child;
		
		impl->removeWidget(widget);
		widget->setProperty("parentLayout", QVariant());
		widget->setParent(NULL);
		
		Py_RETURN_NONE;
	}
	else if ((isSizer(object)) || (isFlowBox(object))) {
		QLayout *widget = (QLayout *)child;
		
		impl->removeItem(widget);
		widget->setProperty("parentLayout", QVariant());
		
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_DETACH;
})


SL_DEFINE_METHOD(FlowBox, map_to_local, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	QWidget *parent = impl->parentWidget();
	if (!parent) {
		PyErr_SetString(PyExc_RuntimeError, "flowbox is not attached to any widget");
		return NULL;
	}
	return createVectorObject(parent->mapFromGlobal(pos) - impl->geometry().topLeft());
})


SL_DEFINE_METHOD(FlowBox, map_to_global, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	QWidget *parent = impl->parentWidget();
	if (!parent) {
		PyErr_SetString(PyExc_RuntimeError, "flowbox is not attached to any widget");
		return NULL;
	}
	return createVectorObject(parent->mapToGlobal(pos) + impl->geometry().topLeft());
})


SL_DEFINE_METHOD(FlowBox, fit, {
	QWidget *window = impl->parentWidget();
	if (window)
		window->resize(impl->minimumSize());
})


SL_DEFINE_METHOD(FlowBox, get_minsize, {
	return createVectorObject(impl->minimumSize());
})


SL_DEFINE_METHOD(FlowBox, get_margins, {
	int top, right, bottom, left;
	
	impl->getContentsMargins(&left, &top, &right, &bottom);
	
	return Py_BuildValue("(iiii)", top, right, bottom, left);
})


SL_DEFINE_METHOD(FlowBox, set_margins, {
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
	
	impl->setContentsMargins(left, top, right, bottom);
})


SL_DEFINE_METHOD(FlowBox, get_boxalign, {
	unsigned long value = 0;
	
	Qt::Alignment alignment = impl->alignment();
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


SL_DEFINE_METHOD(FlowBox, set_boxalign, {
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
	impl->setAlignment(alignment);
	QWidget *parent = impl->parentWidget();
	if (parent)
		parent->layout()->invalidate();
})


SL_DEFINE_METHOD(FlowBox, get_cell, {
	return createVectorObject(qvariant_cast<QPoint>(impl->property("layoutCell")));
})


SL_DEFINE_METHOD(FlowBox, set_cell, {
	QPoint cell;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &cell))
		return NULL;
	
	impl->setProperty("layoutCell", QVariant::fromValue(cell));
	Sizer_Impl *parent = qobject_cast<Sizer_Impl *>(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(FlowBox, get_span, {
	return createVectorObject(qvariant_cast<QSize>(impl->property("layoutSpan")));
})


SL_DEFINE_METHOD(FlowBox, set_span, {
	QSize span;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &span))
		return NULL;
	
	impl->setProperty("layoutSpan", QVariant::fromValue(span));
	Sizer_Impl *parent = qobject_cast<Sizer_Impl *>(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(FlowBox, get_prop, {
	return PyInt_FromLong(qMax(0, qvariant_cast<int>(impl->property("layoutProp"))));
})


SL_DEFINE_METHOD(FlowBox, set_prop, {
	int prop;
	
	if (!PyArg_ParseTuple(args, "i", &prop))
		return NULL;
	
	impl->setProperty("layoutProp", QVariant::fromValue(prop));
	Sizer_Impl *parent = qobject_cast<Sizer_Impl *>(qvariant_cast<QObject *>(impl->property("parentLayout")));
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


SL_DEFINE_METHOD(FlowBox, get_horizontal_spacing, {
	return PyInt_FromLong(impl->spacing().x());
})


SL_DEFINE_METHOD(FlowBox, set_horizontal_spacing, {
	int spacing;
	
	if (!PyArg_ParseTuple(args, "i", &spacing))
		return NULL;
	
	impl->setSpacing(QPoint(spacing, impl->spacing().y()));
})


SL_DEFINE_METHOD(FlowBox, get_vertical_spacing, {
	return PyInt_FromLong(impl->spacing().y());
})


SL_DEFINE_METHOD(FlowBox, set_vertical_spacing, {
	int spacing;
	
	if (!PyArg_ParseTuple(args, "i", &spacing))
		return NULL;
	
	impl->setSpacing(QPoint(impl->spacing().x(), spacing));
})


SL_DEFINE_METHOD(FlowBox, get_orientation, {
	if (impl->orientation() == (Qt::Orientation)0)
		Py_RETURN_NONE;
	return PyInt_FromLong(impl->orientation());
})


SL_DEFINE_METHOD(FlowBox, set_orientation, {
	PyObject *object;
	int orient;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		impl->setOrientation((Qt::Orientation)0);
	}
	else {
		if (!convertInt(object, &orient))
			return NULL;
		impl->setOrientation(orient == SL_HORIZONTAL ? Qt::Horizontal : Qt::Vertical);
	}
})


SL_START_PROXY_DERIVED(FlowBox, Widget)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(map_to_local)
SL_METHOD(map_to_global)
SL_METHOD(fit)
SL_METHOD(get_minsize)

SL_PROPERTY(margins)
SL_PROPERTY(boxalign)
SL_PROPERTY(cell)
SL_PROPERTY(span)
SL_PROPERTY(prop)

SL_PROPERTY(horizontal_spacing)
SL_PROPERTY(vertical_spacing)
SL_PROPERTY(orientation)
SL_END_PROXY_DERIVED(FlowBox, Widget)


#include "flowbox.moc"
#include "flowbox_h.moc"

