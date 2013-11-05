#include "slew.h"

#include "calendar.h"

#include <QStyle>
#include <QCommonStyle>
#include <QToolButton>
#include <QEvent>
#include <QTextCharFormat>



class Style : public QCommonStyle
{
	Q_OBJECT
	
public:
	Style(Calendar_Impl *calendar) : QCommonStyle(), fCalendar(calendar)
	{
		setParent(calendar);
		calendar->setStyle(this);
	}
	
protected slots:
	QIcon standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option = 0, const QWidget * widget = 0)
	{
		QStyle *style = QApplication::style();
		QIcon icon;
		
		if (standardIcon == SP_ArrowLeft)
			icon = fCalendar->previousIcon();
		else if (standardIcon == SP_ArrowRight)
			icon = fCalendar->nextIcon();
		
		if (icon.isNull()) {
			icon = style->standardIcon(standardIcon, option, widget);
		}
		
		return icon;
	}

private:
	Calendar_Impl	*fCalendar;
};



Calendar_Impl::Calendar_Impl()
	: QCalendarWidget(), WidgetInterface()
{
	QPalette pal = palette();
	pal.setColor(QPalette::Disabled, QPalette::Text, Qt::gray);
	setPalette(pal);
	
	new Style(this);
	
	QTextCharFormat sat_format = weekdayTextFormat(Qt::Saturday);
	QTextCharFormat format = weekdayTextFormat(Qt::Monday);
	format.setFontCapitalization(QFont::Capitalize);
	sat_format.setFontCapitalization(QFont::Capitalize);
	sat_format.setForeground(format.foreground());
	
	setHeaderTextFormat(format);
	setWeekdayTextFormat(Qt::Saturday, sat_format);
	setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
	
	connect(this, SIGNAL(clicked(const QDate&)), this, SLOT(handleClicked(const QDate&)));
	connect(this, SIGNAL(activated(const QDate&)), this, SLOT(handleActivated(const QDate&)));
}


void
Calendar_Impl::handleClicked(const QDate& date)
{
	if (selectionMode() != NoSelection) {
		EventRunner runner(this, "onSelect");
		if (runner.isValid()) {
			runner.set("date", date);
			runner.run();
		}
	}
}


void
Calendar_Impl::handleActivated(const QDate& date)
{
	EventRunner runner(this, "onActivate");
	if (runner.isValid()) {
		runner.set("date", date);
		runner.run();
	}
}


void
Calendar_Impl::setPreviousIcon(const QIcon& icon)
{
	fPreviousIcon = icon;
	QEvent e(QEvent::LayoutDirectionChange);
	QApplication::sendEvent(this, &e);
}


void
Calendar_Impl::setNextIcon(const QIcon& icon)
{
	fNextIcon = icon;
	QEvent e(QEvent::LayoutDirectionChange);
	QApplication::sendEvent(this, &e);
}



SL_DEFINE_METHOD(Calendar, set_today, {
	impl->showToday();
})


SL_DEFINE_METHOD(Calendar, set_previous_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setPreviousIcon(icon);
})


SL_DEFINE_METHOD(Calendar, set_next_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setNextIcon(icon);
})


SL_DEFINE_METHOD(Calendar, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isGridVisible())
		style |= SL_CALENDAR_STYLE_GRID;
	if (impl->selectionMode() == QCalendarWidget::SingleSelection)
		style |= SL_CALENDAR_STYLE_SELECTABLE;
	if (impl->isDateEditEnabled())
		style |= SL_CALENDAR_STYLE_EDITABLE;
	if (impl->isNavigationBarVisible())
		style |= SL_CALENDAR_STYLE_CONTROLS;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Calendar, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setGridVisible(style & SL_CALENDAR_STYLE_GRID ? true : false);
	impl->setSelectionMode(style & SL_CALENDAR_STYLE_SELECTABLE ? QCalendarWidget::SingleSelection : QCalendarWidget::NoSelection);
	impl->setDateEditEnabled(style & SL_CALENDAR_STYLE_EDITABLE ? true : false);
	impl->setNavigationBarVisible(style & SL_CALENDAR_STYLE_CONTROLS ? true : false);
})


SL_DEFINE_METHOD(Calendar, get_names, {
	int names = SL_CALENDAR_NAMES_NONE;
	
	if (impl->horizontalHeaderFormat() == QCalendarWidget::LongDayNames)
		names = SL_CALENDAR_NAMES_LONG;
	else if (impl->horizontalHeaderFormat() == QCalendarWidget::ShortDayNames)
		names = SL_CALENDAR_NAMES_SHORT;
	else if (impl->horizontalHeaderFormat() == QCalendarWidget::SingleLetterDayNames)
		names = SL_CALENDAR_NAMES_LETTER;
	
	return PyInt_FromLong(names);
})


SL_DEFINE_METHOD(Calendar, set_names, {
	int names;
	
	if (!PyArg_ParseTuple(args, "i", &names))
		return NULL;
	
	switch (names) {
	case SL_CALENDAR_NAMES_LONG:	impl->setHorizontalHeaderFormat(QCalendarWidget::LongDayNames); break;
	case SL_CALENDAR_NAMES_SHORT:	impl->setHorizontalHeaderFormat(QCalendarWidget::ShortDayNames); break;
	case SL_CALENDAR_NAMES_LETTER:	impl->setHorizontalHeaderFormat(QCalendarWidget::SingleLetterDayNames); break;
	default:						impl->setHorizontalHeaderFormat(QCalendarWidget::NoHorizontalHeader); break;
	}
})


SL_DEFINE_METHOD(Calendar, get_date, {
	return createDateObject(impl->selectedDate());
})


SL_DEFINE_METHOD(Calendar, set_date, {
	QDate date;
	
	if (!PyArg_ParseTuple(args, "O&", convertDate, &date))
		return NULL;
	
	impl->setSelectedDate(date);
})


SL_DEFINE_METHOD(Calendar, get_min_date, {
	return createDateObject(impl->minimumDate());
})


SL_DEFINE_METHOD(Calendar, set_min_date, {
	QDate date;
	
	if (!PyArg_ParseTuple(args, "O&", convertDate, &date))
		return NULL;
	
	impl->setMinimumDate(date);
})


SL_DEFINE_METHOD(Calendar, get_max_date, {
	return createDateObject(impl->maximumDate());
})


SL_DEFINE_METHOD(Calendar, set_max_date, {
	QDate date;
	
	if (!PyArg_ParseTuple(args, "O&", convertDate, &date))
		return NULL;
	
	impl->setMaximumDate(date);
})


SL_DEFINE_METHOD(Calendar, get_first_day_of_week, {
	int day = (int)impl->firstDayOfWeek();
	return PyInt_FromLong(day % 7);
})


SL_DEFINE_METHOD(Calendar, set_first_day_of_week, {
	int day;
	
	if (!PyArg_ParseTuple(args, "i", &day))
		return NULL;
	
	day = day % 7;
	if (day == 0)
		day = 7;
	impl->setFirstDayOfWeek((Qt::DayOfWeek)day);
})


SL_START_PROXY_DERIVED(Calendar, Window)
SL_METHOD(set_today)
SL_METHOD(set_previous_icon)
SL_METHOD(set_next_icon)

SL_PROPERTY(style)
SL_PROPERTY(names)
SL_PROPERTY(date)
SL_PROPERTY(min_date)
SL_PROPERTY(max_date)
SL_PROPERTY(first_day_of_week)
SL_END_PROXY_DERIVED(Calendar, Window)


#include "calendar.moc"
#include "calendar_h.moc"

