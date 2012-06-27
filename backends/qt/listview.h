#ifndef __listview_h__
#define __listview_h__


#include "slew.h"

#include "constants/listview.h"
#include "constants/iconview.h"

#include <QListView>


class ListView_Impl : public QListView, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ListView)
	
	SL_DECLARE_SET_VISIBLE(QListView)
	SL_DECLARE_SIZE_HINT(QListView)
	SL_DECLARE_VIEW(QListView)
	
	virtual void setModel(QAbstractItemModel *model);
	
	void setWideSel(bool enabled) { fWideSel = enabled; update(); }
	bool isWideSel() { return fWideSel; }
	
public slots:
	void handleSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
	void handleActivated(const QModelIndex& index);
	void handleContextMenu(const QPoint& pos);
	
	QStyleOptionViewItem initOptionView() { return viewOptions(); }
	void prepareDrag() { setDirtyRegion(viewport()->rect()); startAutoScroll(); }
	void unprepareDrag() { stopAutoScroll(); setState(NoState); viewport()->update(); }

private:
	bool		fWideSel;
};


#endif
