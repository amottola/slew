#include "slew.h"

#include "treeview.h"

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QToolTip>



class TreeView_Delegate : public ItemDelegate
{
	Q_OBJECT
	
public:
	TreeView_Delegate(TreeView_Impl *parent) : ItemDelegate(parent), fTreeView(parent) {}
	
	virtual void preparePaint(QStyleOptionViewItem *opt, QStyleOptionViewItem *backOpt, const QModelIndex& index) const
	{
		if (fTreeView->selectionBehavior() == QAbstractItemView::SelectRows) {
			if ((fTreeView->isDecorated()) && (!(fTreeView->showExpanders())) && (index.column() == 0)) {
				if (index.parent().isValid()) {
#ifdef Q_WS_WIN
					opt->rect.adjust(4, 0, 0, 0);
#elif defined(Q_WS_MAC)
					opt->rect.adjust(6, 0, 0, 0);
#endif
				}
				backOpt->rect.setLeft(0);
			}
			if ((!fTreeView->header()->stretchLastSection()) && (index.column() == fTreeView->header()->count() - 1))
				backOpt->rect.setRight(1000000);
		}
	}
	
	virtual void finishPaint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		painter->save();
	
		QRect viewRect = fTreeView->viewport()->rect();
		QRect rect = fTreeView->visualRect(index);
		if (fTreeView->isDecorated()) {
			if ((!fTreeView->indexBelow(index).isValid()) || (!fTreeView->indexBelow(index).parent().isValid())) {
				QModelIndex parent = index;
				while (parent.parent().isValid())
					parent = fTreeView->model()->parent(parent);
				QVariant color = fTreeView->model()->data(parent, Qt::ForegroundRole);
				QPen pen(qvariant_cast<QColor>(color));
				painter->setClipping(false);
				painter->setPen(pen);
				painter->drawLine(viewRect.left(), rect.bottom(), viewRect.right(), rect.bottom());
			}
			else if (!index.parent().isValid()) {
				if (!fTreeView->isExpanded(index)) {
					QVariant color = fTreeView->model()->data(index, Qt::ForegroundRole);
					QPen pen(qvariant_cast<QColor>(color));
					painter->setClipping(false);
					painter->setPen(pen);
					painter->drawLine(viewRect.left(), rect.bottom(), viewRect.right(), rect.bottom());
				}
			}
			else {
				QColor color = qvariant_cast<QColor>(fTreeView->model()->data(index, Qt::ForegroundRole));
				QPen pen(color.darker(120));
				QVector<qreal> dashes;
				dashes << 1 << 1;
				pen.setDashPattern(dashes);
				rect.adjust(fTreeView->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, fTreeView) + 1, 0, 0, 0);
				painter->setPen(pen);
				painter->drawLine(rect.left(), rect.bottom(), viewRect.right() + 1, rect.bottom());
			}
		}
		else if ((fTreeView->showRules()) && (!(option.state & QStyle::State_Selected))) {
			QColor color = qvariant_cast<QColor>(fTreeView->model()->data(index, Qt::BackgroundRole));
			if (!color.isValid())
				color = Qt::gray;
			else
				color = color.darker(120);
			QPen pen(color);
			painter->setPen(pen);
			int left = rect.left();
			QModelIndex below = fTreeView->indexBelow(index);
			if ((index.column() == 0) && (below.isValid()))
				left = qMin(left, fTreeView->visualRect(below).left());
			painter->drawLine(left, rect.bottom(), rect.right(), rect.bottom());
			if (index.column() > 0)
				painter->drawLine(left, rect.top(), left, rect.bottom());
		}
		painter->restore();
	}
	
private:
	TreeView_Impl		*fTreeView;
};



TreeView_Impl::TreeView_Impl()
	: QTreeView(), WidgetInterface(), fShowExpanders(true), fIsDecorated(false), fShowRules(false), fExpandState(Waiting)
{
	setHorizontalScrollMode(ScrollPerPixel);
	setVerticalScrollMode(ScrollPerPixel);
	
	setTabKeyNavigation(true);
	setAllColumnsShowFocus(true);
	setExpandsOnDoubleClick(false);
	setUniformRowHeights(false);
	setAlternatingRowColors(true);
	setDragDropMode(DropOnly);
	setDropIndicatorShown(false);
	setEditTriggers(NoEditTriggers);
	setContextMenuPolicy(Qt::CustomContextMenu);
	setSelectionMode(SingleSelection);
	setItemDelegate(new TreeView_Delegate(this));
	setAttribute(Qt::WA_MacShowFocusRect, false);
	
	header()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	header()->setContextMenuPolicy(Qt::CustomContextMenu);
	header()->setMinimumHeight(style()->pixelMetric(QStyle::PM_HeaderMargin, NULL, header()) + header()->fontMetrics().height());
	new HeaderStyle(header());
	
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenu(const QPoint&)));
	connect(header(), SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenuOnHeader(const QPoint&)));
	connect(this, SIGNAL(activated(const QModelIndex&)), this, SLOT(handleActivated(const QModelIndex&)));
	connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(handleExpanded(const QModelIndex&)));
	connect(this, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(handleCollapsed(const QModelIndex&)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateEditorGeometries()));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateEditorGeometries()));
	
	header()->viewport()->installEventFilter(this);
	viewport()->installEventFilter(this);
	
	viewport()->setMouseTracking(true);
}


bool
TreeView_Impl::eventFilter(QObject *object, QEvent *event)
{
	if (object == header()->viewport()) {
		switch (int(event->type())) {
	
		case QEvent::ToolTip:
			{
				QHelpEvent *e = (QHelpEvent *)event;
				
				PyAutoLocker locker;
				PyObject *model = getDataModel(this->model());
				PyObject *pos = createVectorObject(QPoint(header()->logicalIndexAt(e->x()), -1));
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
			break;
		}
	}
	else {
		switch (int(event->type())) {
		
		case QEvent::DragMove:
			{
				fExpandTimer.start(800, viewport());
			}
			break;
		
		case QEvent::DragLeave:
		case QEvent::Drop:
			{
				fExpandTimer.stop();
			}
			break;
		
		case QEvent::Timer:
			{
				QTimerEvent *e = (QTimerEvent *)event;
				if (e->timerId() == fExpandTimer.timerId()) {
					fExpandTimer.stop();
					switch (fExpandState) {
					
					case Waiting:
						{
							QPoint pos = viewport()->mapFromGlobal(QCursor::pos());
							if (viewport()->rect().contains(pos)) {
								QModelIndex index = indexAt(pos);
								QAbstractItemModel *model = this->model();
								if ((index.isValid()) && (model->flags(index) & Qt::ItemIsEnabled) && (model->hasChildren(index)) && (!isExpanded(index))) {
									fExpandState = Blinking;
									fExpandFrame = 0;
									fExpandIndex = indexAt(pos);
									setDirtyRegion(QRegion(visualRect(fExpandIndex)));
									update();
									fExpandTimer.start(75, viewport());
								}
							}
						}
						break;
					
					case Blinking:
						{
							fExpandFrame++;
							setDirtyRegion(QRegion(visualRect(fExpandIndex).adjusted(-5, -5, 5, 5)));
							if (fExpandFrame < 4)
								fExpandTimer.start(75, viewport());
							else {
								fExpandState = Waiting;
								setExpanded(fExpandIndex, true);
							}
							update();
						}
						break;
					}
				}
			}
			break;
		}
	}
	return false;
}


void
TreeView_Impl::moveCursorByDelta(int dx, int dy)
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


void
TreeView_Impl::focusInEvent(QFocusEvent *event)
{
	QTreeView::focusInEvent(event);
	if ((event->reason() == Qt::MouseFocusReason) && (state() != EditingState) && (viewport()->underMouse())) {
		if (!indexAt(viewport()->mapFromGlobal(QCursor::pos())).isValid())
			edit(currentIndex(), AllEditTriggers, event);
	}
}


bool
TreeView_Impl::edit(const QModelIndex& index, EditTrigger trigger, QEvent *event)
{
	if ((trigger == SelectedClicked) && (editTriggers() == AllEditTriggers))
		trigger = DoubleClicked;
	if (QTreeView::edit(index, trigger, event)) {
		if (!fEditIndex.isValid()) {
			fEditIndex = index;
			QMetaObject::invokeMethod(this, "startEdit", Qt::QueuedConnection);
		}
		return true;
	}
	return false;
}


void
TreeView_Impl::setModel(QAbstractItemModel *model)
{
	QAbstractItemModel *oldModel = this->model();
	if (model == oldModel)
		return;
	
	if (qobject_cast<DataModel_Impl *>(oldModel)) {
		disconnect(oldModel, SIGNAL(configureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)), this, SLOT(handleConfigureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)));
		disconnect(oldModel, SIGNAL(modelReset()), this, SLOT(resetColumns()));
		disconnect(oldModel, SIGNAL(layoutChanged()), this, SLOT(handleResizeToContents()));
	}
	
	if (qobject_cast<DataModel_Impl *>(model)) {
		connect(model, SIGNAL(configureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)), this, SLOT(handleConfigureHeader(const QPoint&, Qt::TextElideMode, QHeaderView::ResizeMode)));
		connect(model, SIGNAL(modelReset()), this, SLOT(resetColumns()));
		connect(model, SIGNAL(layoutChanged()), this, SLOT(handleResizeToContents()), Qt::QueuedConnection);
	}
	
	stopEdit();
	QTreeView::setModel(model);
	connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(handleSelectionChanged(const QItemSelection&, const QItemSelection&)));
}


bool
TreeView_Impl::isFocusOutEvent(QEvent *event)
{
	ItemDelegate *delegate = qobject_cast<TreeView_Delegate *>(itemDelegate());
	if (!delegate)
		return false;
	return delegate->isFocusOutEvent(event);
}


bool
TreeView_Impl::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	ItemDelegate *delegate = qobject_cast<TreeView_Delegate *>(itemDelegate());
	if (!delegate)
		return true;
	return delegate->canFocusOut(oldFocus, newFocus);
}


void
TreeView_Impl::handleConfigureHeader(const QPoint& headerPos, Qt::TextElideMode elideMode, QHeaderView::ResizeMode resizeMode)
{
	if (headerPos.x() == -1)
		return;
	HeaderStyle *style = (HeaderStyle *)header()->style();
	style->setMode(elideMode);
	if (header()->resizeMode(headerPos.x()) != resizeMode)
		header()->setResizeMode(resizeMode);
}


void
TreeView_Impl::handleResizeToContents()
{
	resizeColumnToContents(0);
}


void
TreeView_Impl::handleContextMenuOnHeader(const QPoint& pos)
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("header", createVectorObject(QPoint(header()->logicalIndexAt(pos), -1)));
		runner.set("pos", createVectorObject(pos));
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.set("index", Py_None, false);
		runner.run();
	}
}


void
TreeView_Impl::handleContextMenu(const QPoint& pos)
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
TreeView_Impl::handleActivated(const QModelIndex& index)
{
	DataModel_Impl *model = (DataModel_Impl *)this->model();
	EventRunner runner(this, "onActivate");
	if (runner.isValid()) {
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
}


void
TreeView_Impl::handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		runner.set("selection", getViewSelection(this));
		runner.run();
	}
}


void
TreeView_Impl::handleExpanded(const QModelIndex& index)
{
	QMetaObject::invokeMethod(this, "handleResizeToContents", Qt::QueuedConnection);
	DataModel_Impl *model = (DataModel_Impl *)this->model();
	EventRunner runner(this, "onExpand");
	if (runner.isValid()) {
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
}


void
TreeView_Impl::handleCollapsed(const QModelIndex& index)
{
	QMetaObject::invokeMethod(this, "handleResizeToContents", Qt::QueuedConnection);
	DataModel_Impl *model = (DataModel_Impl *)this->model();
	EventRunner runner(this, "onCollapse");
	if (runner.isValid()) {
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
}



void
TreeView_Impl::restartEdit(const QModelIndex& index, int position)
{
	setCurrentIndex(index);
	QTreeView::edit(index, AllEditTriggers, NULL);
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(indexWidget(index));
	if ((editor) && (position >= 0)) {
		editor->setCursorPosition(position);
	}
}


void
TreeView_Impl::startEdit()
{
	ItemDelegate *delegate = qobject_cast<TreeView_Delegate *>(itemDelegate());
	if (delegate) {
		delegate->startEditing(fEditIndex);
	}
	fEditIndex = QModelIndex();
}


void
TreeView_Impl::stopEdit()
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
TreeView_Impl::reset()
{
	QTreeView::reset();
	resetColumns();
}


void
TreeView_Impl::resetColumns()
{
	QAbstractItemModel *model = header()->model();
	if (model) {
		int count = model->columnCount();
		
		for (int i = 0; i < count; i++)
			header()->resizeSection(i, model->headerData(i, Qt::Horizontal, Qt::UserRole).toInt());
	}
}


void
TreeView_Impl::paintDropTarget(QPainter *painter, const QModelIndex& index, int where)
{
	QStyleOptionViewItem option = viewOptions();
	QRect rect = visualRect(index);
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
			
			if (where == SL_EVENT_DRAG_BELOW_ITEM) {
				PyAutoLocker locker;
				QAbstractItemModel *model = this->model();
				QModelIndex below = index;
				while (isExpanded(below)) {
					int children = model->rowCount(below);
					if (children == 0)
						break;
					below = model->index(children - 1, 0, below);
				}
				y = visualRect(below).bottom() + 1;
			}
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


void
TreeView_Impl::drawBranches(QPainter *painter, const QRect& rect, const QModelIndex& index) const
{
	if (fShowExpanders)
		QTreeView::drawBranches(painter, rect, index);
}


void
TreeView_Impl::drawRow(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem opt(option);
	
	if (selectionBehavior() == QAbstractItemView::SelectRows) {
		if ((fIsDecorated) && (!fShowExpanders)) {
			if (index.column() == 0)
				opt.rect.setLeft(0);
		}
	}
	
	QTreeView::drawRow(painter, opt, index);
}


SL_DEFINE_METHOD(TreeView, edit, {
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


SL_DEFINE_METHOD(TreeView, set_expanded, {
	PyObject *object;
	bool expanded;
	
	if (!PyArg_ParseTuple(args, "OO&", &object, convertBool, &expanded))
		return NULL;
	
	DataModel_Impl *model = (DataModel_Impl *)impl->model();
	QModelIndex index = model->index(object);
	if (PyErr_Occurred())
		return NULL;
	
	if (index.isValid()) {
		impl->setExpanded(index, expanded);
	}
	else {
		if (expanded)
			impl->expandAll();
		else
			impl->collapseAll();
	}
})


SL_DEFINE_METHOD(TreeView, is_expanded, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	DataModel_Impl *model = (DataModel_Impl *)impl->model();
	QModelIndex index = model->index(object);
	if (PyErr_Occurred())
		return NULL;
	
	return createBoolObject(impl->isExpanded(index));
})


SL_DEFINE_METHOD(TreeView, set_span_first_column, {
	int row;
	bool enabled;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO&O", &row, convertBool, &enabled, &object))
		return NULL;
	
	DataModel_Impl *model = (DataModel_Impl *)impl->model();
	QModelIndex index = model->index(object);
	if (PyErr_Occurred())
		return NULL;
	
	impl->setFirstColumnSpanned(row, index, enabled);
})


SL_DEFINE_METHOD(TreeView, complete, {
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(impl->indexWidget(impl->currentIndex()));
	if (editor)
		editor->complete();
})


SL_DEFINE_METHOD(TreeView, get_insertion_point, {
	int pos = -1;
	FormattedLineEdit *editor = qobject_cast<FormattedLineEdit *>(impl->indexWidget(impl->currentIndex()));
	if (editor)
		pos = editor->cursorPosition();
	
	return PyInt_FromLong(pos);
})


SL_DEFINE_METHOD(TreeView, select_all, {
	impl->QTreeView::selectAll();
})


SL_DEFINE_METHOD(TreeView, move_cursor, {
	int dx, dy;
	
	if (!PyArg_ParseTuple(args, "ii", &dx, &dy))
		return NULL;
	
	impl->moveCursorByDelta(dx, dy);
})


SL_DEFINE_METHOD(TreeView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (!impl->isHeaderHidden())
		style |= SL_TREEVIEW_STYLE_HEADER;
	if (impl->alternatingRowColors())
		style |= SL_TREEVIEW_STYLE_ALT_ROWS;
	if (impl->selectionBehavior() == QAbstractItemView::SelectRows)
		style |= SL_TREEVIEW_STYLE_SELECT_ROWS;
	if (impl->selectionMode() == QAbstractItemView::NoSelection)
		style |= SL_TREEVIEW_STYLE_NO_SELECTION;
	else if (impl->selectionMode() == QAbstractItemView::ExtendedSelection)
		style |= SL_TREEVIEW_STYLE_MULTI;
	if (impl->editTriggers() == QAbstractItemView::NoEditTriggers)
		style |= SL_TREEVIEW_STYLE_READONLY;
	else if (impl->editTriggers() == (QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed))
		style |= SL_TREEVIEW_STYLE_DELAYED_EDIT;
	if (impl->isAnimated())
		style |= SL_TREEVIEW_STYLE_ANIMATED;
	if (impl->isDecorated())
		style |= SL_TREEVIEW_STYLE_DECORATED_ROWS;
	if (impl->showExpanders())
		style |= SL_TREEVIEW_STYLE_EXPANDERS;
	if (impl->showRules())
		style |= SL_TREEVIEW_STYLE_RULES;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(TreeView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setHeaderHidden(style & SL_TREEVIEW_STYLE_HEADER ? false : true);
	impl->setAlternatingRowColors(style & SL_TREEVIEW_STYLE_ALT_ROWS ? true : false);
	impl->setAnimated(style & SL_TREEVIEW_STYLE_ANIMATED ? true : false);
	impl->setSelectionBehavior(style & SL_TREEVIEW_STYLE_SELECT_ROWS ? QAbstractItemView::SelectRows : QAbstractItemView::SelectItems);
	impl->setSelectionMode(style & SL_TREEVIEW_STYLE_NO_SELECTION ? QAbstractItemView::NoSelection : (style & SL_TREEVIEW_STYLE_MULTI ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection));
	impl->setEditTriggers(style & SL_TREEVIEW_STYLE_READONLY ? QAbstractItemView::NoEditTriggers : (style & SL_TREEVIEW_STYLE_DELAYED_EDIT ? QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed : QAbstractItemView::AllEditTriggers));
	impl->setDecorated(style & SL_TREEVIEW_STYLE_DECORATED_ROWS ? true : false);
	impl->setShowExpanders(style & SL_TREEVIEW_STYLE_EXPANDERS ? true : false);
	impl->setShowRules(style & SL_TREEVIEW_STYLE_RULES ? true : false);
	impl->header()->setStretchLastSection(!(style & SL_TREEVIEW_STYLE_FIT_COLS));
	impl->update();
})



SL_START_VIEW_PROXY(TreeView)
SL_METHOD(edit)
SL_METHOD(set_expanded)
SL_METHOD(is_expanded)
SL_METHOD(set_span_first_column)
SL_METHOD(complete)
SL_METHOD(get_insertion_point)
SL_METHOD(select_all)
SL_METHOD(move_cursor)

SL_PROPERTY(style)
SL_END_VIEW_PROXY(TreeView)


#include "treeview.moc"
#include "treeview_h.moc"

