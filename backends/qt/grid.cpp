#include "slew.h"

#include "grid.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QToolTip>



class Grid_Delegate : public ItemDelegate
{
	Q_OBJECT
	
public:
	Grid_Delegate(Grid_Impl *parent) : ItemDelegate(parent), fGrid(parent) {}
	
	virtual void preparePaint(QStyleOptionViewItem *opt, QStyleOptionViewItem *backOpt, const QModelIndex& index) const
	{
		if ((fGrid->editTriggers() != QAbstractItemView::NoEditTriggers) && (fGrid->selectionBehavior() == QAbstractItemView::SelectRows))
			opt->state &= ~QStyle::State_HasFocus;
	}
	
private:
	Grid_Impl		*fGrid;
};



Grid_Impl::Grid_Impl()
	: QTableView(), WidgetInterface(), fHeaders(Qt::Horizontal)
{
	setHorizontalScrollMode(ScrollPerPixel);
	setVerticalScrollMode(ScrollPerPixel);
	
	verticalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	verticalHeader()->setDefaultSectionSize(QFontMetrics(font()).height() + 5);
	verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	verticalHeader()->setHighlightSections(true);
	verticalHeader()->setVisible(false);
	verticalHeader()->setMinimumWidth(style()->pixelMetric(QStyle::PM_HeaderMargin, NULL, verticalHeader()) + verticalHeader()->fontMetrics().maxWidth());
	verticalHeader()->setResizeMode(QHeaderView::Fixed);
	
	horizontalHeader()->setStretchLastSection(true);
	horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	horizontalHeader()->setMovable(true);
	horizontalHeader()->setClickable(false);
	horizontalHeader()->setHighlightSections(false);
	horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
	horizontalHeader()->setMinimumHeight(style()->pixelMetric(QStyle::PM_HeaderMargin, NULL, horizontalHeader()) + horizontalHeader()->fontMetrics().height());
	horizontalHeader()->setResizeMode(QHeaderView::Interactive);
	new HeaderStyle(horizontalHeader());
	
	setAlternatingRowColors(true);
	setShowGrid(true);
	setTabKeyNavigation(true);
	setWordWrap(false);
	
	setDragDropMode(DropOnly);
	setDropIndicatorShown(false);
	setEditTriggers(AllEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(SingleSelection);
	setItemDelegate(new Grid_Delegate(this));
	setAttribute(Qt::WA_MacShowFocusRect, false);
	
	connect(horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnHeader(const QPoint&)));
	connect(verticalHeader(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnVerticalHeader(const QPoint&)));
	connect(horizontalHeader(), SIGNAL(sectionMoved(int, int, int)), this, SLOT(handleSectionMoved(int, int, int)));
	connect(horizontalHeader(), SIGNAL(sectionResized(int, int, int)), this, SLOT(handleSectionResized(int, int, int)));
	connect(this, SIGNAL(activated(const QModelIndex&)), this, SLOT(handleActivated(const QModelIndex&)), Qt::QueuedConnection);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnViewport(const QPoint&)));
	
	horizontalHeader()->viewport()->installEventFilter(this);
	verticalHeader()->viewport()->installEventFilter(this);
	
	viewport()->setMouseTracking(true);
}


bool
Grid_Impl::eventFilter(QObject *object, QEvent *event)
{
	switch (int(event->type())) {
	
	case QEvent::ToolTip:
		{
			if ((object == horizontalHeader()->viewport()) || (object == verticalHeader()->viewport())) {
				QHelpEvent *e = (QHelpEvent *)event;
				
				PyAutoLocker locker;
				PyObject *model = getDataModel(this->model());
				PyObject *pos = createVectorObject(QPoint(object == horizontalHeader()->viewport() ? horizontalHeader()->logicalIndexAt(e->x()) : -1,
														  object == verticalHeader()->viewport() ? verticalHeader()->logicalIndexAt(e->y()) : -1));
				PyObject *dataSpecifier = PyObject_CallMethod(model, "header", "O", pos);
				if (!dataSpecifier) {
					Py_DECREF(pos);
					Py_DECREF(model);
					PyErr_Print();
					PyErr_Clear();
					break;
				}
				
				QString tip;
				
				if (getObjectAttr(dataSpecifier, "tip", &tip))
					QToolTip::showText(e->globalPos(), tip);
				
				Py_DECREF(dataSpecifier);
				Py_DECREF(pos);
				Py_DECREF(model);
			}
		}
		break;
	
	case QEvent::MouseButtonPress:
		{
			if ((object == horizontalHeader()->viewport()) || (object == verticalHeader()->viewport())) {
				QMouseEvent *e = (QMouseEvent *)event;
				
				PyAutoLocker locker;
				PyObject *model = getDataModel(this->model());
				PyObject *pos = createVectorObject(QPoint(object == horizontalHeader()->viewport() ? horizontalHeader()->logicalIndexAt(e->x()) : -1,
														  object == verticalHeader()->viewport() ? verticalHeader()->logicalIndexAt(e->y()) : -1));
				PyObject *dataSpecifier = PyObject_CallMethod(model, "header", "O", pos);
				if (!dataSpecifier) {
					Py_DECREF(pos);
					Py_DECREF(model);
					PyErr_Print();
					PyErr_Clear();
					break;
				}
				
				int flags = SL_DATA_SPECIFIER_SELECTABLE;
				getObjectAttr(dataSpecifier, "flags", &flags);
				
				Py_DECREF(dataSpecifier);
				Py_DECREF(pos);
				Py_DECREF(model);
				
				if ((!(flags & SL_DATA_SPECIFIER_SELECTABLE)) || (!(flags & SL_DATA_SPECIFIER_ENABLED)))
					return true;
			}
		}
		break;
	
	case QEvent::MouseButtonRelease:
		{
			QHeaderView *header = horizontalHeader();
			if (object == header->viewport()) {
				QMouseEvent *e = (QMouseEvent *)event;
				
				PyAutoLocker locker;
				PyObject *model = getDataModel(this->model());
				PyObject *pos = createVectorObject(QPoint(header->logicalIndexAt(e->x()), -1));
				PyObject *dataSpecifier = PyObject_CallMethod(model, "header", "O", pos);
				if (!dataSpecifier) {
					Py_DECREF(pos);
					Py_DECREF(model);
					PyErr_Print();
					PyErr_Clear();
					break;
				}
				
				int flags = SL_DATA_SPECIFIER_SELECTABLE;
				getObjectAttr(dataSpecifier, "flags", &flags);
				
				Py_DECREF(dataSpecifier);
				Py_DECREF(pos);
				Py_DECREF(model);
				
				if ((!(flags & SL_DATA_SPECIFIER_SELECTABLE)) || (!(flags & SL_DATA_SPECIFIER_ENABLED))) {
					/*
					 *	Hack to prevent section moving!
					 *	This forces internal QHeaderView state to QHeaderViewPrivate::NoState, and hides the moving section indicator
					 */
					disconnect(horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnHeader(const QPoint&)));
					QContextMenuEvent fakeEvent(QContextMenuEvent::Other, e->pos());
					QApplication::sendEvent(header->viewport(), &fakeEvent);
					connect(horizontalHeader(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnHeader(const QPoint&)));
					return true;
				}
			}
		}
		break;
	}
	return false;
}


void
Grid_Impl::moveCursorByDelta(int dx, int dy)
{
	CursorAction action;
	int count = 0;
	
	if (dx > 0) {
		action = MoveRight;
		count = dx;
	}
	else if (dx < 0) {
		action = MoveLeft;
		count = -dx;
	}
	else if (dy > 0) {
		action = MoveDown;
		count = dy;
	}
	else if (dy < 0) {
		action = MoveUp;
		count = -dy;
	}
	while (count > 0) {
		moveCursor(action, Qt::NoModifier);
		count--;
	}
}


QModelIndex
Grid_Impl::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
	QModelIndex index;
	QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect;
	if (selectionBehavior() == SelectRows) {
		flags |= QItemSelectionModel::Rows;
		if (cursorAction == MoveEnd) {
			index = model()->index(model()->rowCount() - 1, 0, QModelIndex());
			selectionModel()->setCurrentIndex(index, flags);
			return index;
		}
	}
	
	if ((editTriggers() != NoEditTriggers) && ((cursorAction == MoveNext) || (cursorAction == MovePrevious)) && (currentIndex().isValid()) && (!(flags & QItemSelectionModel::Rows))) {
		int count = model()->columnCount() * model()->rowCount();
		
		disconnect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(handleSelectionChanged(const QItemSelection&, const QItemSelection&)));
		index = QTableView::moveCursor(cursorAction, modifiers);
		while ((index.isValid()) && (!(model()->flags(index) & Qt::ItemIsEditable)) && (count-- > 0)) {
			selectionModel()->setCurrentIndex(index, flags);
			index = QTableView::moveCursor(cursorAction, modifiers);
		}
		connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(handleSelectionChanged(const QItemSelection&, const QItemSelection&)));
	}
	else {
		index = QTableView::moveCursor(cursorAction, modifiers);
	}
	if (selectionBehavior() == SelectRows) {
		selectionModel()->setCurrentIndex(index, flags);
	}
// 	else {
// 		if (!edit(index, QAbstractItemView::AllEditTriggers, NULL))
// 			selectionModel()->setCurrentIndex(index, flags);
// 	}
	return index;
}


void
Grid_Impl::focusInEvent(QFocusEvent *event)
{
	QTableView::focusInEvent(event);
	if ((event->reason() == Qt::MouseFocusReason) && (state() != EditingState) && (viewport()->underMouse())) {
		if (!indexAt(viewport()->mapFromGlobal(QCursor::pos())).isValid())
			edit(currentIndex(), AllEditTriggers, event);
	}
}


void
Grid_Impl::keyPressEvent(QKeyEvent *event)
{
	QTableView::keyPressEvent(event);
#ifdef Q_WS_MAC
	if ((event->key() == Qt::Key_Enter) || (event->key() == Qt::Key_Return)) {
		if ((state() != EditingState) || (hasFocus())) {
			QModelIndex index = currentIndex();
			if (index.isValid())
				emit activated(index);
			event->ignore();
		}
	}
#endif
}


bool
Grid_Impl::edit(const QModelIndex& index, EditTrigger trigger, QEvent *event)
{
	if ((trigger == SelectedClicked) && (editTriggers() == AllEditTriggers))
		trigger = DoubleClicked;
	if (QTableView::edit(index, trigger, event)) {
		if (!fEditIndex.isValid()) {
			fEditIndex = index;
			QMetaObject::invokeMethod(this, "startEdit", Qt::QueuedConnection);
		}
		return true;
	}
	return false;
}


void
Grid_Impl::setModel(QAbstractItemModel *model)
{
	QAbstractItemModel *oldModel = this->model();
	if (model == oldModel)
		return;
	
	if (qobject_cast<DataModel_Impl *>(oldModel)) {
		disconnect(oldModel, SIGNAL(configureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)), this, SLOT(handleConfigureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)));
		disconnect(oldModel, SIGNAL(sorted(int, Qt::SortOrder)), this, SLOT(handleSortIndicatorChanged(int, Qt::SortOrder)));
		disconnect(oldModel, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(handleRowsColsRemoved(QModelIndex, int, int)));
		disconnect(oldModel, SIGNAL(columnsRemoved(QModelIndex, int, int)), this, SLOT(handleRowsColsRemoved(QModelIndex, int, int)));
		disconnect(oldModel, SIGNAL(modelReset()), this, SLOT(resetColumns()));
	}
	
	if (qobject_cast<DataModel_Impl *>(model)) {
		connect(model, SIGNAL(configureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)), this, SLOT(handleConfigureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)));
		connect(model, SIGNAL(sorted(int, Qt::SortOrder)), this, SLOT(handleSortIndicatorChanged(int, Qt::SortOrder)));
		connect(model, SIGNAL(rowsRemoved(QModelIndex, int, int)), this, SLOT(handleRowsColsRemoved(QModelIndex, int, int)), Qt::QueuedConnection);
		connect(model, SIGNAL(columnsRemoved(QModelIndex, int, int)), this, SLOT(handleRowsColsRemoved(QModelIndex, int, int)));
		connect(model, SIGNAL(modelReset()), this, SLOT(resetColumns()));
	}
	
	stopEdit();
	QTableView::setModel(model);
	connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(handleSelectionChanged(const QItemSelection&, const QItemSelection&)));
}


bool
Grid_Impl::isFocusOutEvent(QEvent *event)
{
	ItemDelegate *delegate = qobject_cast<Grid_Delegate *>(itemDelegate());
	if (!delegate)
		return false;
	return delegate->isFocusOutEvent(event);
}


bool
Grid_Impl::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	ItemDelegate *delegate = qobject_cast<Grid_Delegate *>(itemDelegate());
	if (!delegate)
		return true;
	return delegate->canFocusOut(oldFocus, newFocus);
}


void
Grid_Impl::handleConfigureHeader(const QPoint& headerPos, Qt::TextElideMode elideMode, QHeaderView::ResizeMode resizeMode)
{
	QHeaderView *header = horizontalHeader();
	if ((headerPos.x() == -1) || (!header))
		return;
	HeaderStyle *style = (HeaderStyle *)header->style();
	style->setMode(elideMode);
	if (header->resizeMode(headerPos.x()) != resizeMode)
		header->setResizeMode(resizeMode);
}


void
Grid_Impl::handleSortIndicatorChanged(int index, Qt::SortOrder order)
{
	EventRunner runner(this, "onSort");
	if (runner.isValid()) {
		runner.set("sortby", index);
		runner.set("ascending", order == Qt::AscendingOrder);
		runner.run();
	}
}


void
Grid_Impl::handleContextMenuOnHeader(const QPoint& pos)
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("header", createVectorObject(QPoint(horizontalHeader()->logicalIndexAt(pos), -1)));
		runner.set("pos", createVectorObject(pos));
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.set("index", Py_None, false);
		runner.run();
	}
}


void
Grid_Impl::handleContextMenuOnVerticalHeader(const QPoint& pos)
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("header", createVectorObject(QPoint(-1, verticalHeader()->logicalIndexAt(pos))));
		runner.set("pos", createVectorObject(pos));
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.set("index", Py_None, false);
		runner.run();
	}
}


void
Grid_Impl::handleContextMenuOnViewport(const QPoint& pos)
{
	DataModel_Impl *model = (DataModel_Impl *)this->model();
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("header", Py_None, false);
		runner.set("pos", createVectorObject(pos));
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.set("index", model->getDataIndex(indexAt(pos)), false);
		runner.run();
	}
}


void
Grid_Impl::handleActivated(const QModelIndex& index)
{
	DataModel_Impl *model = (DataModel_Impl *)this->model();
	EventRunner runner(this, "onActivate");
	if (runner.isValid()) {
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
}


void
Grid_Impl::handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		runner.set("selection", getViewSelection(this));
		runner.run();
	}
}


void
Grid_Impl::handleSectionMoved(int logicalIndex, int fromVisualIndex, int toVisualIndex)
{
	EventRunner runner(this, "onHeaderColumnMoved");
	if (runner.isValid()) {
		runner.set("logical_index", logicalIndex);
		runner.set("from_index", fromVisualIndex);
		runner.set("to_index", toVisualIndex);
		runner.run();
	}
}


void
Grid_Impl::handleSectionResized(int logicalIndex, int fromWidth, int toWidth)
{
	EventRunner runner(this, "onHeaderColumnResized");
	if (runner.isValid()) {
		runner.set("logical_index", logicalIndex);
		runner.set("width", toWidth);
		runner.run();
	}
}


void
Grid_Impl::handleRowsColsRemoved(const QModelIndex& index, int start, int end)
{
	if (indexWidget(currentIndex()))
		setState(EditingState);
}


void
Grid_Impl::restartEdit(const QModelIndex& index, int position)
{
	setCurrentIndex(index);
	QTableView::edit(index, AllEditTriggers, NULL);
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(indexWidget(index));
	if ((editor) && (position >= 0)) {
		editor->setCursorPosition(position);
	}
}


void
Grid_Impl::startEdit()
{
	ItemDelegate *delegate = qobject_cast<Grid_Delegate *>(itemDelegate());
	if (delegate) {
		delegate->startEditing(fEditIndex);
	}
	fEditIndex = QModelIndex();
}


void
Grid_Impl::stopEdit()
{
	QWidget *editor = indexWidget(currentIndex());
	if (editor) {
		FormattedLineEdit *lineEdit = qobject_cast<FormattedLineEdit *>(editor);
		if ((!lineEdit) || (canFocusOut(this, this))) {
			commitData(editor);
			closeEditor(editor, QAbstractItemDelegate::NoHint);
		}
	}
}


void
Grid_Impl::selectAll()
{
	EventRunner runner(this, "onSort");
	if (runner.isValid()) {
		runner.set("sortby", -1);
		runner.set("ascending", true);
		runner.run();
	}
}


void
Grid_Impl::reset()
{
	QTableView::reset();
	resetColumns();
}


void
Grid_Impl::resetColumns()
{
	QHeaderView *header = horizontalHeader();
	QAbstractItemModel *model = header->model();
	if (model) {
		int count = model->columnCount();
		
		for (int i = 0; i < count; i++)
			header->resizeSection(i, model->headerData(i, Qt::Horizontal, Qt::UserRole).toInt());
	}
}


void
Grid_Impl::paintDropTarget(QPainter *painter, const QModelIndex& index, int where)
{
	QStyleOptionViewItem option = viewOptions();
	QRect rect = this->visualRect(index);
	QWidget *viewport = this->viewport();
	QColor highlight = palette().color(QPalette::HighlightedText);
	QColor color = option.state & QStyle::State_Selected ? highlight : palette().color(QPalette::Highlight);
	QPen pen(color);
	
	painter->save();
	
	if (selectionBehavior() == QAbstractItemView::SelectRows) {
		rect.setLeft(viewport->rect().left());
		rect.setRight(viewport->rect().right());
	}
	
	if (!index.isValid())
		where = SL_EVENT_DRAG_ON_VIEWPORT;
	
	switch (where) {
	case SL_EVENT_DRAG_BELOW_ITEM:
	case SL_EVENT_DRAG_ABOVE_ITEM:
		{
			int x, y, width;
			
			if (where == SL_EVENT_DRAG_BELOW_ITEM)
				y = rect.bottom() + (showGrid() ? 2 : 1);
			else
				y = rect.top();
			if (y == 0)
				y++;
			x = rect.left();
			width = viewport->width() - rect.left() - 10;
			
			painter->setRenderHint(QPainter::Antialiasing);
			
			pen.setWidth(3);
			pen.setColor(highlight);
			painter->setPen(pen);
			painter->drawEllipse(QPointF(x + width, y), 3, 3);
			painter->drawLine(x, y, x + width - 3, y);
			
			pen.setWidth(2);
			pen.setColor(color);
			painter->setPen(pen);
			painter->drawEllipse(QPointF(x + width, y), 3, 3);
			painter->drawLine(x, y, x + width - 3, y);
		}
		break;
	
	case SL_EVENT_DRAG_ON_ITEM:
		{
			rect.adjust(1, 1, -1, -1);
			
			painter->setRenderHint(QPainter::Antialiasing);
			int radius = qMin(8, rect.height() / 2);
			
			pen.setWidth(3);
			pen.setColor(highlight);
			painter->setPen(pen);
			painter->drawRoundedRect(rect, radius, radius);

			pen.setWidth(2);
			
			pen.setColor(color);
			painter->setPen(pen);
			painter->drawRoundedRect(rect, radius, radius);
		}
		break;
		
	case SL_EVENT_DRAG_ON_VIEWPORT:
		{
			rect = viewport->rect();
			rect.adjust(0, 0, -1, -1);
			
			painter->setRenderHint(QPainter::Antialiasing, false);
			
			pen.setWidth(5);
			pen.setColor(highlight);
			painter->setPen(pen);
			painter->drawRect(rect);
			
			pen.setWidth(3);
			pen.setColor(color);
			painter->setPen(pen);
			painter->drawRect(rect);
		}
		break;
	}
	
	painter->restore();
}


SL_DEFINE_METHOD(Grid, edit, {
	PyObject *object;
	int pos;
	
	if (!PyArg_ParseTuple(args, "Oi", &object, &pos))
		return NULL;
	
	DataModel_Impl *model = (DataModel_Impl *)impl->model();
	QModelIndex index = model->index(object);
	if (PyErr_Occurred())
		return NULL;
	
	if (index.isValid())
		QMetaObject::invokeMethod(impl, "restartEdit", Qt::QueuedConnection, Q_ARG(QModelIndex, index), Q_ARG(int, pos));
	else
		impl->stopEdit();
})


SL_DEFINE_METHOD(Grid, set_cell_span, {
	int row, column, rowSpan, columnSpan;
	
	if (!PyArg_ParseTuple(args, "iiii", &row, &column, &rowSpan, &columnSpan))
		return NULL;
	
	impl->setSpan(row, column, rowSpan, columnSpan);
})


SL_DEFINE_METHOD(Grid, get_row_span, {
	int row, column;
	
	if (!PyArg_ParseTuple(args, "ii", &row, &column))
		return NULL;
	
	return PyInt_FromLong(impl->rowSpan(row, column));
})


SL_DEFINE_METHOD(Grid, get_column_span, {
	int row, column;
	
	if (!PyArg_ParseTuple(args, "ii", &row, &column))
		return NULL;
	
	return PyInt_FromLong(impl->columnSpan(row, column));
})


SL_DEFINE_METHOD(Grid, get_column_width, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	return PyInt_FromLong(impl->horizontalHeader()->sectionSize(column));
})


SL_DEFINE_METHOD(Grid, set_column_width, {
	int column, width;
	
	if (!PyArg_ParseTuple(args, "ii", &column, &width))
		return NULL;
	
	impl->horizontalHeader()->resizeSection(column, width);
})


SL_DEFINE_METHOD(Grid, get_column_pos, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	return PyInt_FromLong(impl->horizontalHeader()->visualIndex(column));
})


SL_DEFINE_METHOD(Grid, set_column_pos, {
	int column, pos;
	
	if (!PyArg_ParseTuple(args, "ii", &column, &pos))
		return NULL;
	
	int old_pos = impl->horizontalHeader()->visualIndex(column);
	impl->horizontalHeader()->moveSection(old_pos, pos);
})


SL_DEFINE_METHOD(Grid, set_sorting, {
	QHeaderView *header = impl->horizontalHeader();
	int column;
	PyObject *object;
	bool ascending;
	
	if (!PyArg_ParseTuple(args, "iO", &column, &object))
		return NULL;
	
	if (object == Py_None)
		ascending = header->sortIndicatorOrder() == Qt::AscendingOrder;
	else if (!convertBool(object, &ascending))
		return NULL;
	
	header->blockSignals(true);
	header->setSortIndicator(column < 0 ? -1 : column, ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
	header->blockSignals(false);
})


SL_DEFINE_METHOD(Grid, complete, {
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(impl->indexWidget(impl->currentIndex()));
	if (editor)
		editor->complete();
})


SL_DEFINE_METHOD(Grid, get_insertion_point, {
	int pos = -1;
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(impl->indexWidget(impl->currentIndex()));
	if (editor)
		pos = editor->cursorPosition();
	
	return PyInt_FromLong(pos);
})


SL_DEFINE_METHOD(Grid, select_all, {
	impl->QTableView::selectAll();
})


SL_DEFINE_METHOD(Grid, move_cursor, {
	int dx, dy;
	
	if (!PyArg_ParseTuple(args, "ii", &dx, &dy))
		return NULL;
	
	impl->moveCursorByDelta(dx, dy);
})


SL_DEFINE_METHOD(Grid, set_default_row_height, {
	int height;
	
	if (!PyArg_ParseTuple(args, "i", &height))
		return NULL;
	
	impl->verticalHeader()->setDefaultSectionSize(height > 0 ? height : QFontMetrics(impl->font()).height() + 5);
})


SL_DEFINE_METHOD(Grid, set_column_hidden, {
	int column;
	bool hidden;
	
	if (!PyArg_ParseTuple(args, "iO&", &column, convertBool, &hidden))
		return NULL;
	
	impl->horizontalHeader()->setSectionHidden(column, hidden);
})


SL_DEFINE_METHOD(Grid, is_column_hidden, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	return createBoolObject(impl->horizontalHeader()->isSectionHidden(column));
})


SL_DEFINE_METHOD(Grid, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isHeaderEnabled(Qt::Horizontal))
		style |= SL_GRID_STYLE_HEADER;
	if (impl->isHeaderEnabled(Qt::Vertical))
		style |= SL_GRID_STYLE_VERTICAL_HEADER;
	if (impl->verticalHeader()->resizeMode(0) == QHeaderView::ResizeToContents)
		style |= SL_GRID_STYLE_AUTO_ROWS;
	if (impl->alternatingRowColors())
		style |= SL_GRID_STYLE_ALT_ROWS;
	if (impl->isSortingEnabled())
		style |= SL_GRID_STYLE_SORTABLE;
	if (impl->showGrid())
		style |= SL_GRID_STYLE_RULES;
	if (impl->selectionBehavior() == QAbstractItemView::SelectRows)
		style |= SL_GRID_STYLE_SELECT_ROWS;
	if (impl->selectionMode() == QAbstractItemView::NoSelection)
		style |= SL_GRID_STYLE_NO_SELECTION;
	else if (impl->selectionMode() == QAbstractItemView::ExtendedSelection)
		style |= SL_GRID_STYLE_MULTI;
	if (impl->editTriggers() == QAbstractItemView::NoEditTriggers)
		style |= SL_GRID_STYLE_READONLY;
	else if (impl->editTriggers() == (QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed))
		style |= SL_GRID_STYLE_DELAYED_EDIT;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Grid, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->horizontalHeader()->setVisible(style & SL_GRID_STYLE_HEADER ? true : false);
	impl->setHeaderEnabled(Qt::Horizontal, style & SL_GRID_STYLE_HEADER ? true : false);
	impl->verticalHeader()->setVisible(style & SL_GRID_STYLE_VERTICAL_HEADER ? true : false);
	impl->setHeaderEnabled(Qt::Vertical, style & SL_GRID_STYLE_VERTICAL_HEADER ? true : false);
	impl->verticalHeader()->setResizeMode(style & SL_GRID_STYLE_AUTO_ROWS ? QHeaderView::ResizeToContents : QHeaderView::Fixed);
	impl->setAlternatingRowColors(style & SL_GRID_STYLE_ALT_ROWS ? true : false);
	impl->setSortingEnabled(style & SL_GRID_STYLE_SORTABLE ? true : false);
	impl->horizontalHeader()->setClickable(style & SL_GRID_STYLE_SORTABLE ? true : false);
	impl->setShowGrid(style & SL_GRID_STYLE_RULES ? true : false);
	impl->setSelectionBehavior(style & SL_GRID_STYLE_SELECT_ROWS ? QAbstractItemView::SelectRows : QAbstractItemView::SelectItems);
	impl->setSelectionMode(style & SL_GRID_STYLE_NO_SELECTION ? QAbstractItemView::NoSelection : (style & SL_GRID_STYLE_MULTI ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection));
	impl->setEditTriggers(style & SL_GRID_STYLE_READONLY ? QAbstractItemView::NoEditTriggers : ((style & SL_GRID_STYLE_DELAYED_EDIT) ? (QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed) : QAbstractItemView::AllEditTriggers));
})


SL_DEFINE_METHOD(Grid, get_row, {
	int row = -1;
	QModelIndex index = impl->currentIndex();
	if (index.isValid())
		row = index.row();
	return PyInt_FromLong(row);
})


SL_DEFINE_METHOD(Grid, set_row, {
	int row, column;
	
	if (!PyArg_ParseTuple(args, "i", &row))
		return NULL;
	
	QModelIndex index = impl->currentIndex();
	QAbstractItemModel *model = impl->model();
	QItemSelectionModel *selection = impl->selectionModel();
	QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current;
	if (impl->selectionBehavior() == QAbstractItemView::SelectRows)
		flags |= QItemSelectionModel::Rows;
	
	selection->blockSignals(true);
	if ((row >= 0) && (row < model->rowCount()) && (model->columnCount() > 0)) {
		if (index.isValid())
			column = index.column();
		else
			column = 0;
		selection->select(model->index(row, column), flags);
	}
	else
		selection->clearSelection();
	selection->blockSignals(false);
})


SL_DEFINE_METHOD(Grid, get_column, {
	int column = -1;
	QModelIndex index = impl->currentIndex();
	if (index.isValid())
		column = index.column();
	return PyInt_FromLong(column);
})


SL_DEFINE_METHOD(Grid, set_column, {
	int row, column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	QModelIndex index = impl->currentIndex();
	QAbstractItemModel *model = impl->model();
	QItemSelectionModel *selection = impl->selectionModel();
	QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current;
	if (impl->selectionBehavior() == QAbstractItemView::SelectRows)
		flags |= QItemSelectionModel::Rows;
	
	selection->blockSignals(true);
	if ((column >= 0) && (column < model->columnCount()) && (model->rowCount() > 0)) {
		if (index.isValid())
			row = index.row();
		else
			row = 0;
		selection->select(model->index(row, column), flags);
	}
	else
		selection->clearSelection();
	selection->blockSignals(false);
})


SL_DEFINE_METHOD(Grid, is_ascending, {
	return createBoolObject(impl->horizontalHeader()->sortIndicatorOrder() == Qt::AscendingOrder);
})


SL_DEFINE_METHOD(Grid, set_ascending, {
	QHeaderView *header = impl->horizontalHeader();
	bool ascending;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &ascending))
		return NULL;
	
	header->setSortIndicator(header->sortIndicatorSection(), ascending ? Qt::AscendingOrder : Qt::DescendingOrder);
})


SL_DEFINE_METHOD(Grid, get_sortby, {
	return PyInt_FromLong(impl->horizontalHeader()->sortIndicatorSection());
})


SL_DEFINE_METHOD(Grid, set_sortby, {
	QHeaderView *header = impl->horizontalHeader();
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	header->setSortIndicator(column < 0 ? -1 : column, header->sortIndicatorOrder());
})



SL_START_VIEW_PROXY(Grid)
SL_METHOD(edit)
SL_METHOD(set_cell_span)
SL_METHOD(get_row_span)
SL_METHOD(get_column_span)
SL_METHOD(get_column_width)
SL_METHOD(set_column_width)
SL_METHOD(get_column_pos)
SL_METHOD(set_column_pos)
SL_METHOD(set_sorting)
SL_METHOD(complete)
SL_METHOD(get_insertion_point)
SL_METHOD(select_all)
SL_METHOD(move_cursor)
SL_METHOD(set_default_row_height)
SL_METHOD(set_column_hidden)
SL_METHOD(is_column_hidden)

SL_PROPERTY(style)
SL_PROPERTY(row)
SL_PROPERTY(column)
SL_BOOL_PROPERTY(ascending)
SL_PROPERTY(sortby)
SL_END_VIEW_PROXY(Grid)


#include "grid.moc"
#include "grid_h.moc"

