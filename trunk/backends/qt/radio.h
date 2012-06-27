#ifndef __radio_h__
#define __radio_h__


#include "slew.h"

#include "constants/window.h"

#include <QRadioButton>
#include <QButtonGroup>


class Radio_Impl : public QRadioButton, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Radio, {
		clear();
	})
	
	SL_DECLARE_SET_VISIBLE(QRadioButton)
	SL_DECLARE_SIZE_HINT(QRadioButton)

	void setGroup(const QString& group);
	QString group() { return fGroup; }
	
	void setValue(const QString& value) { fValue = value; }
	QString value() { return fValue; }
	
	virtual bool isModifyEvent(QEvent *event);
	
	virtual bool event(QEvent *event);
	
public slots:
	void handleToggled(bool toggled);

private:
	void clear();

	QString			fValue;
	QString			fGroup;
	QButtonGroup	*fButtonGroup;
};


#endif
