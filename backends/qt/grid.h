#ifndef __grid_h__
#define __grid_h__


#include "slew.h"
#include "objects.h"

#include "constants/grid.h"
#include "constants/window.h"

#include <QTableView>
#include <QPersistentModelIndex>


class Grid_Impl : public QTableView, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Grid)
	
	SL_DECLARE_SET_VISIBLE(QTableView)
	SL_DECLARE_SIZE_HINT(QTableView)
	SL_DECLARE_VIEW(QTableView)
	
	void moveCursorByDelta(int dx, int dy);
	
	void setHeaderEnabled(Qt::Orientation type, bool enabled) { if (enabled) { fHeaders |= type; } else { fHeaders &= ~type; } }
	bool isHeaderEnabled(Qt::Orientation type) { return fHeaders & type; }
	
	virtual void setModel(QAbstractItemModel *model);
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool canFocusOut(QWidget *oldFocus, QWidget *newFocus);
	
public slots:
	void handleConfigureHeader(const QPoint& headerPos, Qt::TextElideMode elideMode);
	void handleSortIndicatorChanged(int index, Qt::SortOrder order);
	void handleContextMenuOnHeader(const QPoint& pos);
	void handleContextMenuOnVerticalHeader(const QPoint& pos);
	void handleContextMenuOnViewport(const QPoint& pos);
	void handleActivated(const QModelIndex& index);
	void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void handleSectionMoved(int logicalIndex, int fromVisualIndex, int toVisualIndex);
	void handleSectionResized(int logicalIndex, int fromWidth, int toWidth);
	void handleRowsColsRemoved(const QModelIndex& index, int start, int end);
	void startEdit();
	void restartEdit(const QModelIndex& index, int position);
	void stopEdit();
	virtual void selectAll();
	virtual void reset();
	virtual void resetColumns();
	
	QStyleOptionViewItem initOptionView() { return viewOptions(); }
	void prepareDrag() { setDirtyRegion(viewport()->rect()); startAutoScroll(); }
	void unprepareDrag() { stopAutoScroll(); setState(NoState); viewport()->update(); }
	
protected:
	virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);
	virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
	virtual bool eventFilter(QObject *obj, QEvent *event);
	virtual void focusInEvent(QFocusEvent *event);
	virtual void keyPressEvent(QKeyEvent *event);

private:
	Qt::Orientations		fHeaders;
	QPersistentModelIndex	fEditIndex;
};


#endif
