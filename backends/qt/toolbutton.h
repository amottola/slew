#ifndef __toolbutton_h__
#define __toolbutton_h__


#include "slew.h"
#include "constants/window.h"
#include "constants/button.h"

#include <QToolButton>
#include <QPaintEvent>


class ToolButton_Impl : public QToolButton, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(ToolButton)
	
	SL_DECLARE_SET_VISIBLE(QToolButton)
	SL_DECLARE_SIZE_HINT(QToolButton)
	
	void setFlat(bool flat) { fFlat = flat; update(); }
	bool isFlat() { return fFlat; }
	
	virtual bool isModifyEvent(QEvent *event);

protected:
	virtual void paintEvent(QPaintEvent *event);
	
public slots:
	void handleClicked();
	void handleToggled(bool toggled);
	void handleEndDrag(QObject *object);
	
private:
	bool		fFlat;
};


#endif
