#ifndef __combobox_h__
#define __combobox_h__


#include "slew.h"
#include "constants/combobox.h"

#include <QComboBox>


class ComboBox_Impl : public QComboBox, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ComboBox)
	
	SL_DECLARE_SET_VISIBLE(QComboBox)
	SL_DECLARE_SIZE_HINT(QComboBox)
	
	virtual bool eventFilter(QObject *obj, QEvent *event);
	virtual void showPopup();
	virtual void hidePopup();
	
	void setEnterTabs(bool enabled) { fEnterTabs = enabled; }
	bool isEnterTabs() { return fEnterTabs; }
	void setReadOnly(bool readonly) { fReadOnly = readonly; }
	bool isReadOnly() { return fReadOnly; }
	
	virtual bool isModifyEvent(QEvent *event);
	
	virtual QSize minimumSizeHint() const;
	
public slots:
	void handleCurrentIndexChanged(int index);
	void handleEditingFinished();
	
private:
	bool		fEnterTabs;
	bool		fReadOnly;
	bool		fPopupShown;
};


#endif
