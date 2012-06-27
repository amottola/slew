#ifndef __calendar_h__
#define __calendar_h__


#include "slew.h"
#include "constants/calendar.h"

#include <QCalendarWidget>
#include <QDate>


class Calendar_Impl : public QCalendarWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Calendar)
	
	SL_DECLARE_SET_VISIBLE(QCalendarWidget)
	SL_DECLARE_SIZE_HINT(QCalendarWidget)
	
	void setPreviousIcon(const QIcon& icon);
	QIcon previousIcon() { return fPreviousIcon; }
	
	void setNextIcon(const QIcon& icon);
	QIcon nextIcon() { return fNextIcon; }
	
public slots:
	void handleClicked(const QDate& date);
	void handleActivated(const QDate& date);
	
private:
	QIcon		fPreviousIcon;
	QIcon		fNextIcon;
};


#endif
