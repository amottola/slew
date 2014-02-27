#ifndef __treeview_h__
#define __treeview_h__


#include "slew.h"
#include "objects.h"

#include "constants/treeview.h"
#include "constants/window.h"

#include <QTreeView>
#include <QBasicTimer>
#include <QPersistentModelIndex>


class TreeView_Impl : public QTreeView, public WidgetInterface
{
	Q_OBJECT
	
	enum ExpandState {
		Waiting,
		Blinking,
	};
	
public:
	SL_DECLARE_OBJECT(TreeView)
	
	SL_DECLARE_SET_VISIBLE(QTreeView)
	SL_DECLARE_SIZE_HINT(QTreeView)
	SL_DECLARE_VIEW(QTreeView)
	
	void moveCursorByDelta(int dx, int dy);
	
	void setShowExpanders(bool enabled) { fShowExpanders = enabled; }
	bool showExpanders() { return fShowExpanders; }
	
	void setShowRules(bool enabled) { fShowRules = enabled; }
	bool showRules() { return fShowRules; }
	
	void redrawBranches(QPainter *painter, const QModelIndex& index) { drawBranches(painter, fLastBranchesRect, index); }
	
	virtual void setModel(QAbstractItemModel *model);
	virtual QModelIndex indexAt(const QPoint& point) const;
	virtual QRect visualRect(const QModelIndex& index) const;
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool canFocusOut(QWidget *oldFocus, QWidget *newFocus);
	
public slots:
	void handleConfigureHeader(const QPoint& headerPos, Qt::TextElideMode elideMode);
	void handleContextMenu(const QPoint& pos);
	void handleContextMenuOnHeader(const QPoint& pos);
	void handleActivated(const QModelIndex& index);
	void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void handleExpanded(const QModelIndex& index);
	void handleCollapsed(const QModelIndex& index);
	void handleResizeToContents();
	void startEdit();
	void restartEdit(const QModelIndex& index, int position);
	void stopEdit();
	virtual void reset();
	virtual void resetColumns();
	virtual void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
	void resizeColumns();
	
	QStyleOptionViewItem initOptionView() { return viewOptions(); }
	void prepareDrag() { setDirtyRegion(viewport()->rect()); startAutoScroll(); }
	void unprepareDrag() { stopAutoScroll(); setState(NoState); viewport()->update(); }
	
protected:
	virtual bool edit(const QModelIndex& index, EditTrigger trigger, QEvent* event);
	virtual void drawBranches(QPainter *painter, const QRect& rect, const QModelIndex& index) const;
	virtual bool eventFilter(QObject *obj, QEvent *event);
	virtual void focusInEvent(QFocusEvent *event);
	virtual void scrollContentsBy(int dx, int dy);
	
private:
	bool					fShowExpanders;
	bool					fShowRules;
	QBasicTimer				fExpandTimer;
	ExpandState				fExpandState;
	QPersistentModelIndex	fExpandIndex;
	int						fExpandFrame;
	QPersistentModelIndex	fEditIndex;
	QRect					fLastBranchesRect;
};


#endif
