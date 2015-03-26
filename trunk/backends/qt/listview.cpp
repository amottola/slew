#include "slew.h"
#include "objects.h"

#include "listview.h"
#include "constants/widget.h"

#include <QMouseEvent>



class ListView_Delegate : public ItemDelegate
{
	Q_OBJECT
	
public:
	ListView_Delegate(ListView_Impl *parent) : ItemDelegate(parent), fListView(parent) {}
	
	virtual void preparePaint(QStyleOptionViewItem *opt, QStyleOptionViewItem *backOpt, const QModelIndex& index) const
	{
		backOpt->showDecorationSelected = fListView->isWideSel();
	}
	
private:
	ListView_Impl		*fListView;
};



ListView_Impl::ListView_Impl()
	: QListView(), WidgetInterface(), fWideSel(false)
{
	setItemDelegate(new ListView_Delegate(this));
	setContextMenuPolicy(Qt::CustomContextMenu);
	setDragDropMode(DropOnly);
	setDropIndicatorShown(false);
	setEditTriggers(NoEditTriggers);
	setTextElideMode(Qt::ElideMiddle);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setResizeMode(QListView::Adjust);
	setWrapping(true);
	
	connect(this, SIGNAL(activated(const QModelIndex&)), this, SLOT(handleActivated(const QModelIndex&)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(handleContextMenu(const QPoint&)));
}


void
ListView_Impl::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
	QAbstractItemView::dataChanged(topLeft, bottomRight, roles);
#else
	QAbstractItemView::dataChanged(topLeft, bottomRight);
#endif
	ListView_Delegate *delegate = qobject_cast<ListView_Delegate *>(itemDelegate());
	if (delegate)
		delegate->invalidate();
}


void
ListView_Impl::reset()
{
	QListView::reset();
	ListView_Delegate *delegate = qobject_cast<ListView_Delegate *>(itemDelegate());
	if (delegate)
		delegate->invalidate();
}


void
ListView_Impl::setModel(QAbstractItemModel *model)
{
	QAbstractItemModel *oldModel = this->model();
	if (model == oldModel)
		return;
	
	QItemSelectionModel *m = selectionModel();
	QListView::setModel(model);
	delete m;
	connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(handleSelectionChanged(const QItemSelection&, const QItemSelection&)));
}


void
ListView_Impl::handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		PyObject *widget = getObject(this);
		if (!widget)
			return;
		PyObject *selection = PyObject_CallMethod(widget, "get_selection", NULL);
		if (!selection) {
			PyErr_Print();
			PyErr_Clear();
			Py_DECREF(widget);
			return;
		}
		runner.set("selection", selection);
		Py_DECREF(widget);
		runner.run();
	}
}


void
ListView_Impl::handleActivated(const QModelIndex& index)
{
	EventRunner runner(this, "onActivate");
	if (runner.isValid()) {
		DataModel_Impl *model = (DataModel_Impl *)this->model();
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
}


void
ListView_Impl::handleContextMenu(const QPoint& pos)
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		DataModel_Impl *model = (DataModel_Impl *)this->model();
		runner.set("index", model->getDataIndex(indexAt(pos)), false);
		runner.run();
	}
}


void
ListView_Impl::paintDropTarget(QPainter *painter, const QModelIndex& index, int where)
{
	QStyleOptionViewItem option = viewOptions();
	QRect rect = this->visualRect(index);
	QWidget *viewport = this->viewport();
	QColor highlight = palette().color(QPalette::HighlightedText);
	QColor color = option.state & QStyle::State_Selected ? highlight : palette().color(QPalette::Highlight);
	QPen pen(color);
	
	painter->save();
	
	if (!index.isValid())
		where = SL_EVENT_DRAG_ON_VIEWPORT;
	
	switch (where) {
	case SL_EVENT_DRAG_BELOW_ITEM:
	case SL_EVENT_DRAG_ABOVE_ITEM:
		{
			if (viewMode() == IconMode) {
				QSize size = gridSize();
				if (size.isEmpty())
					size = rect.size();
				int x, y, height = size.height();
				int cellWidth = size.width() + spacing();
				int cellHeight = height + spacing();
				
				x = rect.left() + horizontalOffset();
				if (where == SL_EVENT_DRAG_BELOW_ITEM)
					x += cellWidth;
				x = ((x / cellWidth) * cellWidth) - horizontalOffset();
				y = (((rect.top() + verticalOffset()) / cellHeight) * cellHeight) - verticalOffset();
				height = qMax(5, height - 5);
				
				painter->setRenderHint(QPainter::Antialiasing);
				
				pen.setWidth(3);
				pen.setColor(highlight);
				painter->setPen(pen);
				painter->drawEllipse(QPointF(x, y + height), 3, 3);
				painter->drawLine(x, y + 5, x, y + height - 3);
				
				pen.setWidth(2);
				pen.setColor(color);
				painter->setPen(pen);
				painter->drawEllipse(QPointF(x, y + height), 3, 3);
				painter->drawLine(x, y + 5, x, y + height - 3);
			}
			else {
				int x, y, width;
				
				if (where == SL_EVENT_DRAG_BELOW_ITEM)
					y = rect.bottom() + 1;
				else
					y = rect.top();
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
		}
		break;
	
	case SL_EVENT_DRAG_ON_ITEM:
		{
			option.rect = rect;
			rect.adjust(1, 1, -1, -1);
			
			painter->setRenderHint(QPainter::Antialiasing);
			int radius = qMin(8, rect.height() / 2);
			
			pen.setWidth(3);
			pen.setColor(highlight);
			painter->setPen(pen);
			painter->drawRoundedRect(rect, radius, radius);

			pen.setWidth(2);
			
			if (viewMode() == IconMode) {
				color = palette().color(QPalette::Inactive, QPalette::Highlight);
				pen.setColor(color);
				painter->setPen(pen);
				painter->setBrush(QBrush(color));
				painter->drawRoundedRect(rect, radius, radius);
				
				QItemSelectionModel *selection = selectionModel();
				
				if ((selection) && (selection->isSelected(index)))
					option.state |= QStyle::State_Selected;
				if (!(model()->flags(index) & Qt::ItemIsEnabled))
					option.state &= ~QStyle::State_Enabled;
				if (option.state & QStyle::State_Enabled)
					option.palette.setCurrentColorGroup(QPalette::Normal);
				else
					option.palette.setCurrentColorGroup(QPalette::Disabled);
				itemDelegate(index)->paint(painter, option, index);
			}
			else {
				pen.setColor(color);
				painter->setPen(pen);
				painter->drawRoundedRect(rect, radius, radius);
			}
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


SL_DEFINE_METHOD(ListView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->viewMode() == QListView::IconMode)
		style |= SL_LISTVIEW_STYLE_ICON;
	if (impl->flow() == QListView::LeftToRight)
		style |= SL_LISTVIEW_STYLE_FLOW_HORIZONTAL;
	if (impl->resizeMode() == QListView::Fixed)
		style |= SL_LISTVIEW_STYLE_FIXED_RESIZE;
	if (impl->isWrapping())
		style |= SL_LISTVIEW_STYLE_WRAP;
	if (impl->selectionMode() == QAbstractItemView::ExtendedSelection)
		style |= SL_LISTVIEW_STYLE_MULTI;
	if (impl->isWideSel())
		style |= SL_ICONVIEW_STYLE_WIDE_SEL;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(ListView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setViewMode(style & SL_LISTVIEW_STYLE_ICON ? QListView::IconMode : QListView::ListMode);
	impl->setDragDropMode(QAbstractItemView::DropOnly);
	impl->setDropIndicatorShown(false);
	impl->setMovement(QListView::Static);
	impl->setEditTriggers(QAbstractItemView::NoEditTriggers);
	impl->setFlow(style & SL_LISTVIEW_STYLE_FLOW_HORIZONTAL ? QListView::LeftToRight : QListView::TopToBottom);
	impl->setResizeMode(style & SL_LISTVIEW_STYLE_FIXED_RESIZE ? QListView::Fixed : QListView::Adjust);
	impl->setWrapping(style & SL_LISTVIEW_STYLE_WRAP ? true : false);
	impl->setSelectionMode(style & SL_LISTVIEW_STYLE_MULTI ? QAbstractItemView::ExtendedSelection : QAbstractItemView::SingleSelection);
	impl->setWideSel(style & SL_ICONVIEW_STYLE_WIDE_SEL ? true : false);
})


SL_DEFINE_METHOD(ListView, get_model_column, {
	return PyInt_FromLong(impl->modelColumn());
})


SL_DEFINE_METHOD(ListView, set_model_column, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	impl->setModelColumn(column);
})


SL_DEFINE_METHOD(ListView, get_grid_size, {
	return createVectorObject(impl->gridSize());
})


SL_DEFINE_METHOD(ListView, set_grid_size, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	impl->setGridSize(size);
})


SL_DEFINE_METHOD(ListView, get_icon_size, {
	return createVectorObject(impl->iconSize());
})


SL_DEFINE_METHOD(ListView, set_icon_size, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	impl->setIconSize(size);
})


SL_DEFINE_METHOD(ListView, get_spacing, {
	return PyInt_FromLong(impl->spacing());
})


SL_DEFINE_METHOD(ListView, set_spacing, {
	int spacing;
	
	if (!PyArg_ParseTuple(args, "i", &spacing))
		return NULL;
	
	impl->setSpacing(spacing);
})



SL_START_VIEW_PROXY(ListView)
SL_PROPERTY(style)
SL_PROPERTY(model_column)
SL_PROPERTY(selection)
SL_PROPERTY(grid_size)
SL_PROPERTY(icon_size)
SL_PROPERTY(spacing)
SL_END_VIEW_PROXY(ListView)


#include "listview.moc"
#include "listview_h.moc"

