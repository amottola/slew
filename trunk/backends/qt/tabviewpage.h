#ifndef __tabviewpage_h__
#define __tabviewpage_h__


#include "slew.h"

#include "tabview.h"
#include "constants/window.h"


class TabViewPage_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(TabViewPage)
	
	SL_DECLARE_SET_VISIBLE(QWidget)
	SL_DECLARE_SIZE_HINT(QWidget)
	
	void setTabView(TabView_Impl *tabView, int index);
	
	void setTitle(const QString& title);
	QString title() { return fTitle; }
	
	void setIcon(const QIcon& icon);
	QIcon icon() { return fIcon; }
	
	void _setEnabled(bool enabled);
	bool _isEnabled() { return fEnabled; }
	
private:
	TabView_Impl	*fTabView;
	QString			fTitle;
	QIcon			fIcon;
	bool			fEnabled;
};


#endif
