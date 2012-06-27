#ifndef __statusbar_h__
#define __statusbar_h__


#include "slew.h"
#include "constants/window.h"

#include <QStatusBar>


class Field
{
public:
	Field(QWidget *widget = NULL, int prop = 0) : fWidget(widget), fProp(prop) {}
	
	QWidget		*fWidget;
	int			fProp;
};


class StatusBar_Impl : public QStatusBar, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(StatusBar)
	
	QList<int> getProps();
	void setProps(const QList<int>& props);
	
	int getProp(int index);
	void setProp(int index, int prop);
	
	QList<Field> *fields() { return &fFields; }
	
	StatusBar_Impl *clone();
	
	virtual bool event(QEvent *event);
	
private:
	QList<Field>		fFields;
};


#endif
