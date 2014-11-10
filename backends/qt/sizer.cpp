#include "slew.h"

#include "sizer.h"
#include "constants/window.h"

#include <QWidgetItem>
#include <QSizePolicy>


class WidgetItem : public QLayoutItem
{
public:
	WidgetItem(Window_Impl *widget) : QLayoutItem(), fWidget(widget)
	{
		invalidate();
	}
	
	virtual QWidget *widget()
	{
		return fWidget;
	}
	
	virtual bool isEmpty() const
	{
		return fWidget->isHidden() || fWidget->isWindow();
	}
	
	virtual QRect geometry() const
	{
		return fWidget->geometry().adjusted(-fMargins.left(), -fMargins.top(), fMargins.right(), fMargins.bottom());
	}
	
	virtual void setGeometry(const QRect& rect)
	{
		if (isEmpty())
			return;
		
		Qt::Alignment align = alignment();
		QRect r = rect.adjusted(fMargins.left(), fMargins.top(), -fMargins.right(), -fMargins.bottom());
		int x = r.left();
		int y = r.top();
		QSize size;
		if (isEmpty())
			size = QSize(0, 0);
		else {
			QSize maxSize = maximumSize();
			if ((maxSize.width() > 0) && (maxSize.width() != QLAYOUTSIZE_MAX))
				maxSize.setWidth(maxSize.width() - fMargins.left() - fMargins.right());
			if ((maxSize.height() > 0) && (maxSize.height() != QLAYOUTSIZE_MAX))
				maxSize.setHeight(maxSize.height() - fMargins.top() - fMargins.bottom());
			size = r.size().boundedTo(maxSize);
		}
		
		if (align & (Qt::AlignHorizontal_Mask | Qt::AlignVertical_Mask)) {
			QSize pref(sizeHint());
			if (!pref.isEmpty())
				pref -= QSize(fMargins.left() + fMargins.right(), fMargins.top() + fMargins.bottom());
			QSizePolicy sp = fWidget->sizePolicy();
			if (sp.horizontalPolicy() == QSizePolicy::Ignored)
				pref.setWidth(fWidget->sizeHint().expandedTo(fWidget->minimumSize()).width());
			if (sp.verticalPolicy() == QSizePolicy::Ignored)
				pref.setHeight(fWidget->sizeHint().expandedTo(fWidget->minimumSize()).height());
			
			if (align & Qt::AlignHorizontal_Mask)
				size.setWidth(qMin(size.width(), pref.width()));
			if (align & Qt::AlignVertical_Mask)
				size.setHeight(qMin(size.height(), pref.height()));
		}
		
		if (align & Qt::AlignRight)
			x += r.width() - size.width();
		else if (!(align & Qt::AlignLeft))
			x += (r.width() - size.width()) / 2;
		
		if (align & Qt::AlignBottom)
			y += r.height() - size.height();
		else if (!(align & Qt::AlignTop))
			y += (r.height() - size.height()) / 2;
		
		fWidget->setGeometry(x, y, size.width(), size.height());
	}
	
	virtual bool hasHeightForWidth() const
	{
		return false;
	}
	
	virtual QSize minimumSize() const
	{
		QSize size(0, 0);
		
		if (isEmpty())
			return size;
		
		QSizePolicy sizePolicy = fWidget->sizePolicy();
		QSize sizeHint = fWidget->sizeHint();
		QSize minSizeHint = fWidget->minimumSizeHint();
		QSize minSize = fWidget->minimumSize();
	
		if (sizePolicy.horizontalPolicy() != QSizePolicy::Ignored) {
			if (sizePolicy.horizontalPolicy() & QSizePolicy::ShrinkFlag)
				size.setWidth(minSizeHint.width());
			else
				size.setWidth(qMax(sizeHint.width(), minSizeHint.width()));
		}
	
		if (sizePolicy.verticalPolicy() != QSizePolicy::Ignored) {
			if (sizePolicy.verticalPolicy() & QSizePolicy::ShrinkFlag) {
				size.setHeight(minSizeHint.height());
			} else {
				size.setHeight(qMax(sizeHint.height(), minSizeHint.height()));
			}
		}
	
		size = size.boundedTo(fWidget->maximumSize());
		if (minSize.width() > 0)
			size.setWidth(minSize.width());
		if (minSize.height() > 0)
			size.setHeight(minSize.height());
		
		return size.expandedTo(QSize(0,0)) + QSize(fMargins.left() + fMargins.right(), fMargins.top() + fMargins.bottom());
	}
	
	virtual QSize maximumSize() const
	{
		QSize size(0, 0);
		
		if (isEmpty())
			return size;
		
		Qt::Alignment align = alignment();
		QSizePolicy sizePolicy = fWidget->sizePolicy();
		
		if (align & Qt::AlignHorizontal_Mask && align & Qt::AlignVertical_Mask)
			return QSize(QLAYOUTSIZE_MAX, QLAYOUTSIZE_MAX);
		size = fWidget->maximumSize();
		QSize hint = fWidget->sizeHint().expandedTo(fWidget->minimumSize());
		if (size.width() == QWIDGETSIZE_MAX && !(align & Qt::AlignHorizontal_Mask))
			if (!(sizePolicy.horizontalPolicy() & QSizePolicy::GrowFlag))
				size.setWidth(hint.width());
	
		if (size.height() == QWIDGETSIZE_MAX && !(align & Qt::AlignVertical_Mask))
			if (!(sizePolicy.verticalPolicy() & QSizePolicy::GrowFlag))
				size.setHeight(hint.height());
	
		if (align & Qt::AlignHorizontal_Mask)
			size.setWidth(QLAYOUTSIZE_MAX);
		else
			size.setWidth(size.width() + fMargins.left() + fMargins.right());
		if (align & Qt::AlignVertical_Mask)
			size.setHeight(QLAYOUTSIZE_MAX);
		else
			size.setHeight(size.height() + fMargins.top() + fMargins.bottom());
		return size;
	}
	
	virtual QSize sizeHint() const
	{
		QSize size(0, 0);
		if (!isEmpty()) {
			size = fWidget->sizeHint().expandedTo(fWidget->minimumSizeHint());
			size = size.boundedTo(fWidget->maximumSize()).expandedTo(fWidget->minimumSize());
			
			if (fWidget->sizePolicy().horizontalPolicy() == QSizePolicy::Ignored)
				size.setWidth(0);
			if (fWidget->sizePolicy().verticalPolicy() == QSizePolicy::Ignored)
				size.setHeight(0);
			
			size += QSize(fMargins.left() + fMargins.right(), fMargins.top() + fMargins.bottom());
		}
		return size;
	}
	
	virtual Qt::Orientations expandingDirections() const
	{
		if (isEmpty())
			return Qt::Orientations(0);
		
		Qt::Orientations e = fWidget->sizePolicy().expandingDirections();
		if (fWidget->layout()) {
			if ((fWidget->sizePolicy().horizontalPolicy() & QSizePolicy::GrowFlag) && (fWidget->layout()->expandingDirections() & Qt::Horizontal))
				e |= Qt::Horizontal;
			if ((fWidget->sizePolicy().verticalPolicy() & QSizePolicy::GrowFlag) && (fWidget->layout()->expandingDirections() & Qt::Vertical))
				e |= Qt::Vertical;
		}
		
		if (alignment() & Qt::AlignHorizontal_Mask)
			e &= ~Qt::Horizontal;
		if (alignment() & Qt::AlignVertical_Mask)
			e &= ~Qt::Vertical;
		return e;
	}
	
	virtual void invalidate()
	{
		fMargins = qvariant_cast<QMargins>(fWidget->property("boxMargins"));
	}
	
private:
	Window_Impl		*fWidget;
	QMargins		fMargins;
};


Sizer_Impl::Sizer_Impl()
	: QGridLayout(), WidgetInterface(), fOrientation((Qt::Orientation)0)
{
	setContentsMargins(0, 0, 0, 0);
	setHorizontalSpacing(0);
	setVerticalSpacing(0);
// 	setSizeConstraint(SetMinAndMaxSize);
}


void
Sizer_Impl::reinsert(QObject *child, bool reparent)
{
	Qt::Alignment alignment;
	
	QPoint cell = qvariant_cast<QPoint>(child->property("layoutCell"));
	QSize span = qvariant_cast<QSize>(child->property("layoutSpan")).expandedTo(QSize(1,1));
	
	if (qobject_cast<Sizer_Impl *>(child)) {
		Sizer_Impl *sizer_child = (Sizer_Impl *)child;
		alignment = sizer_child->alignment();
		
		if (reparent)
			removeItem(sizer_child);
		addItem(sizer_child, cell.y(), cell.x(), span.height(), span.width(), alignment);
	}
	else {
		Window_Impl *widget_child = (Window_Impl *)child;
		alignment = qvariant_cast<Qt::Alignment>(child->property("boxAlign"));
		
		if (reparent)
			removeWidget(widget_child);
		addItem(new WidgetItem(widget_child), cell.y(), cell.x(), span.height(), span.width(), alignment);
	}
	if (reparent)
		reparentChildren(this, this);
}


void
Sizer_Impl::reparentChildren(QLayoutItem *item, Sizer_Impl *parent)
{
	QLayout *layout = qobject_cast<QLayout *>(item->layout());
	QWidget *widget = qobject_cast<QWidget *>(item->widget());
	
	if (layout) {
		for (int i = 0; i < layout->count(); i++) {
			QLayoutItem *child = layout->itemAt(i);
			if (child->layout())
				child->layout()->setParent(layout);
			reparentChildren(child, (Sizer_Impl *)layout);
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


Sizer_Impl *
Sizer_Impl::clone()
{
	Sizer_Impl *sizer = new Sizer_Impl;
	
	QPoint cell = qvariant_cast<QPoint>(property("layoutCell"));
	QSize span = qvariant_cast<QSize>(property("layoutSpan")).expandedTo(QSize(1,1));
	
	sizer->setProperty("layoutCell", QVariant::fromValue(cell));
	sizer->setProperty("layoutSpan", QVariant::fromValue(span));
	sizer->setAlignment(alignment());
	
	while (count() > 0) {
		QLayoutItem *item = takeAt(0);
		if (item->layout()) {
			sizer->reinsert(item->layout(), false);
		}
		else {
			sizer->reinsert(item->widget(), false);
			delete item;
		}
	}
	
	for (int column = 0; column < columnCount(); column++) {
		sizer->setColumnMinimumWidth(column, columnMinimumWidth(column));
		sizer->setColumnStretch(column, columnStretch(column));
	}
	
	for (int row = 0; row < rowCount(); row++) {
		sizer->setRowMinimumHeight(row, rowMinimumHeight(row));
		sizer->setRowStretch(row, rowStretch(row));
	}
	
	sizer->setSpacing(spacing());
	
	reparentChildren(sizer, sizer);
	return sizer;
}


void
Sizer_Impl::ensurePosition(QPoint& cell, const QSize& span)
{
	for (;;) {
		if (!itemAtPosition(cell.y(), cell.x()))
			break;
		if (fOrientation == Qt::Horizontal) {
			cell.rx()++;
		}
		else if (fOrientation == Qt::Vertical) {
			cell.ry()++;
		}
		else {
			cell.rx()++;
			if (cell.x() >= columnCount())
				cell = QPoint(0, cell.y() + 1);
		}
	}
}


Qt::Orientations
Sizer_Impl::expandingDirections() const
{
	Qt::Orientations e = QGridLayout::expandingDirections();
	if (alignment() & Qt::AlignHorizontal_Mask)
		e &= ~Qt::Horizontal;
	if (alignment() & Qt::AlignVertical_Mask)
		e &= ~Qt::Vertical;
	return e;
}


QLayoutItem *
Sizer_Impl::takeAt(int index)
{
	QLayoutItem *item = QGridLayout::takeAt(index);
	if (item->layout()) {
		reparentChildren(item, NULL);
		item->layout()->setParent(NULL);
	}
	else if (item->widget()) {
		item->widget()->setProperty("parentLayout", QVariant());
	}
	
	return item;
}


SL_DEFINE_METHOD(Sizer, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	bool ok = false;
	QPoint cell = qvariant_cast<QPoint>(child->property("layoutCell"));
	QSize span = qvariant_cast<QSize>(child->property("layoutSpan")).expandedTo(QSize(1,1));
	int prop = qvariant_cast<int>(child->property("layoutProp"));
	
	if (isWindow(object)) {
		Window_Impl *widget = (Window_Impl *)child;
		Qt::Alignment alignment = qvariant_cast<Qt::Alignment>(widget->property("boxAlign"));
		
		impl->ensurePosition(cell, span);
		
		if (impl->orientation() == Qt::Horizontal) {
			impl->setColumnMinimumWidth(cell.x(), 0);
			impl->setColumnStretch(cell.x(), prop);
		}
		else if (impl->orientation() == Qt::Vertical) {
			impl->setRowMinimumHeight(cell.y(), 0);
			impl->setRowStretch(cell.y(), prop);
		}
		
		impl->addItem(new WidgetItem(widget), cell.y(), cell.x(), span.height(), span.width(), alignment);
		ok = true;
	}
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		
		impl->ensurePosition(cell, span);
		
		if (impl->orientation() == Qt::Horizontal) {
			impl->setColumnMinimumWidth(cell.x(), 0);
			impl->setColumnStretch(cell.x(), prop);
		}
		else if (impl->orientation() == Qt::Vertical) {
			impl->setRowMinimumHeight(cell.y(), 0);
			impl->setRowStretch(cell.y(), prop);
		}
		
		widget->setParent(impl);
		impl->addItem(widget, cell.y(), cell.x(), span.height(), span.width(), widget->alignment());
		ok = true;
	}
	Sizer_Impl::reparentChildren(impl, impl);
	
	if (ok) {
		QVariant v;
		v.setValue((QObject *)impl);
		child->setProperty("parentLayout", v);
		child->setProperty("layoutCell", QVariant::fromValue(cell));
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_ATTACH;
})


SL_DEFINE_METHOD(Sizer, remove, {
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
	else if (isSizer(object)) {
		Sizer_Impl *widget = (Sizer_Impl *)child;
		
		impl->removeItem(widget);
		widget->setProperty("parentLayout", QVariant());
		
		Py_RETURN_NONE;
	}
	
	SL_RETURN_CANNOT_DETACH;
})


SL_DEFINE_METHOD(Sizer, map_to_local, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	QWidget *parent = impl->parentWidget();
	if (!parent) {
		PyErr_SetString(PyExc_RuntimeError, "sizer is not attached to any widget");
		return NULL;
	}
	return createVectorObject(parent->mapFromGlobal(pos) - impl->geometry().topLeft());
})


SL_DEFINE_METHOD(Sizer, map_to_global, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	QWidget *parent = impl->parentWidget();
	if (!parent) {
		PyErr_SetString(PyExc_RuntimeError, "sizer is not attached to any widget");
		return NULL;
	}
	return createVectorObject(parent->mapToGlobal(pos) + impl->geometry().topLeft());
})


SL_DEFINE_METHOD(Sizer, fit, {
	QWidget *window = impl->parentWidget();
	if (window)
		window->resize(impl->minimumSize());
})


SL_DEFINE_METHOD(Sizer, get_minsize, {
	return createVectorObject(impl->minimumSize());
})


SL_DEFINE_METHOD(Sizer, get_column_width, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	if ((column < 0) || (column >= impl->columnCount())) {
		PyErr_SetString(PyExc_ValueError, "invalid column");
		return NULL;
	}
	
	return PyInt_FromLong(impl->columnMinimumWidth(column));
})


SL_DEFINE_METHOD(Sizer, set_column_width, {
	int column, value;
	
	if (!PyArg_ParseTuple(args, "ii", &column, &value))
		return NULL;
	
	if (column < 0) {
		PyErr_SetString(PyExc_ValueError, "invalid column");
		return NULL;
	}
	
	impl->setColumnStretch(column, 0);
	impl->setColumnMinimumWidth(column, value);
})


SL_DEFINE_METHOD(Sizer, get_column_prop, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	if ((column < 0) || (column >= impl->columnCount())) {
		PyErr_SetString(PyExc_ValueError, "invalid column");
		return NULL;
	}
	
	return PyInt_FromLong(impl->columnStretch(column));
})


SL_DEFINE_METHOD(Sizer, set_column_prop, {
	int column, value;
	
	if (!PyArg_ParseTuple(args, "ii", &column, &value))
		return NULL;
	
	if (column < 0) {
		PyErr_SetString(PyExc_ValueError, "invalid column");
		return NULL;
	}
	
	impl->setColumnMinimumWidth(column, 0);
	impl->setColumnStretch(column, value);
})


SL_DEFINE_METHOD(Sizer, get_row_height, {
	int row;
	
	if (!PyArg_ParseTuple(args, "i", &row))
		return NULL;
	
	if ((row < 0) || (row >= impl->rowCount())) {
		PyErr_SetString(PyExc_ValueError, "invalid row");
		return NULL;
	}
	
	return PyInt_FromLong(impl->rowMinimumHeight(row));
})


SL_DEFINE_METHOD(Sizer, set_row_height, {
	int row, value;
	
	if (!PyArg_ParseTuple(args, "ii", &row, &value))
		return NULL;
	
	if (row < 0) {
		PyErr_SetString(PyExc_ValueError, "invalid row");
		return NULL;
	}
	
	impl->setRowStretch(row, 0);
	impl->setRowMinimumHeight(row, value);
})


SL_DEFINE_METHOD(Sizer, get_row_prop, {
	int row;
	
	if (!PyArg_ParseTuple(args, "i", &row))
		return NULL;
	
	if ((row < 0) || (row >= impl->rowCount())) {
		PyErr_SetString(PyExc_ValueError, "invalid row");
		return NULL;
	}
	
	return PyInt_FromLong(impl->rowStretch(row));
})


SL_DEFINE_METHOD(Sizer, set_row_prop, {
	int row, value;
	
	if (!PyArg_ParseTuple(args, "ii", &row, &value))
		return NULL;
	
	if (row < 0) {
		PyErr_SetString(PyExc_ValueError, "invalid row");
		return NULL;
	}
	
	impl->setRowMinimumHeight(row, 0);
	impl->setRowStretch(row, value);
})


SL_DEFINE_METHOD(Sizer, get_margins, {
	int top, right, bottom, left;
	
	impl->getContentsMargins(&left, &top, &right, &bottom);
	
	return Py_BuildValue("(iiii)", top, right, bottom, left);
})


SL_DEFINE_METHOD(Sizer, set_margins, {
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


SL_DEFINE_METHOD(Sizer, get_boxalign, {
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


SL_DEFINE_METHOD(Sizer, set_boxalign, {
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


SL_DEFINE_METHOD(Sizer, get_cell, {
	return createVectorObject(qvariant_cast<QPoint>(impl->property("layoutCell")));
})


SL_DEFINE_METHOD(Sizer, set_cell, {
	QPoint cell;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &cell))
		return NULL;
	
	impl->setProperty("layoutCell", QVariant::fromValue(cell));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(Sizer, get_span, {
	return createVectorObject(qvariant_cast<QSize>(impl->property("layoutSpan")));
})


SL_DEFINE_METHOD(Sizer, set_span, {
	QSize span;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &span))
		return NULL;
	
	impl->setProperty("layoutSpan", QVariant::fromValue(span));
	Sizer_Impl *parent = (Sizer_Impl *)(qvariant_cast<QObject *>(impl->property("parentLayout")));
	if (parent)
		parent->reinsert(impl);
})


SL_DEFINE_METHOD(Sizer, get_prop, {
	return PyInt_FromLong(qMax(0, qvariant_cast<int>(impl->property("layoutProp"))));
})


SL_DEFINE_METHOD(Sizer, set_prop, {
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


SL_DEFINE_METHOD(Sizer, get_size, {
	return createVectorObject(QSize(impl->columnCount(), impl->rowCount()));
})


SL_DEFINE_METHOD(Sizer, set_size, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	if (size.width() > impl->columnCount())
		impl->setColumnStretch(size.width() - 1, 0);
	if (size.height() > impl->rowCount())
		impl->setRowStretch(size.height() - 1, 0);
})


SL_DEFINE_METHOD(Sizer, get_horizontal_spacing, {
	return PyInt_FromLong(impl->horizontalSpacing());
})


SL_DEFINE_METHOD(Sizer, set_horizontal_spacing, {
	int spacing;
	
	if (!PyArg_ParseTuple(args, "i", &spacing))
		return NULL;
	
	impl->setHorizontalSpacing(spacing);
})


SL_DEFINE_METHOD(Sizer, get_vertical_spacing, {
	return PyInt_FromLong(impl->verticalSpacing());
})


SL_DEFINE_METHOD(Sizer, set_vertical_spacing, {
	int spacing;
	
	if (!PyArg_ParseTuple(args, "i", &spacing))
		return NULL;
	
	impl->setVerticalSpacing(spacing);
})


SL_DEFINE_METHOD(Sizer, get_orientation, {
	if (impl->orientation() == (Qt::Orientation)0)
		Py_RETURN_NONE;
	return PyInt_FromLong(impl->orientation());
})


SL_DEFINE_METHOD(Sizer, set_orientation, {
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


SL_START_PROXY_DERIVED(Sizer, Widget)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(map_to_local)
SL_METHOD(map_to_global)
SL_METHOD(fit)
SL_METHOD(get_minsize)
SL_METHOD(get_column_width)
SL_METHOD(set_column_width)
SL_METHOD(get_column_prop)
SL_METHOD(set_column_prop)
SL_METHOD(get_row_height)
SL_METHOD(set_row_height)
SL_METHOD(get_row_prop)
SL_METHOD(set_row_prop)

SL_PROPERTY(margins)
SL_PROPERTY(boxalign)
SL_PROPERTY(cell)
SL_PROPERTY(span)
SL_PROPERTY(prop)

SL_PROPERTY(size)
SL_PROPERTY(horizontal_spacing)
SL_PROPERTY(vertical_spacing)
SL_PROPERTY(orientation)
SL_END_PROXY_DERIVED(Sizer, Widget)


#include "sizer.moc"
#include "sizer_h.moc"

