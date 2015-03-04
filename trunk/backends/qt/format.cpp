#include "slew.h"
#include "searchfield.h"

#include <QAbstractButton>
#include <QMenu>
#include <QDateTime>
#include <QFocusEvent>
#include <QResizeEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrameV2>
#include <QClipboard>
#include <QLatin1Char>
#include <QChar>
#include <QCache>


#define FORMAT_BLANK_IF_ZERO		0x00000001
#define FORMAT_THOUSANDS_SEP		0x00000002
#define FORMAT_UNSIGNED				0x00000004
#define FORMAT_RED_IF_NEGATIVE		0x00000008
#define FORMAT_ZERO_FILL			0x00000010
#define FORMAT_MONETARY				0x00000020
#define FORMAT_MONETARY_SYMBOL		0x00000040
#define FORMAT_PERCENTAGE			0x00000080

#define FORMAT_HAS_SEPARATOR		0x00000001
#define FORMAT_HAS_DAY				0x00000002
#define FORMAT_HAS_MONTH			0x00000004
#define FORMAT_HAS_YEAR				0x00000008
#define FORMAT_HAS_WEEKDAY			0x00000010
#define FORMAT_REMOVE_ZERO_DAY		0x00000020
#define FORMAT_REMOVE_ZERO_MONTH	0x00000040
#define FORMAT_MONTH_NAME			0x00000080
#define FORMAT_MONTH_NAME_IS_SHORT	0x00000100
#define FORMAT_YEAR_IS_SHORT		0x00000200
#define FORMAT_WEEKDAY_IS_SHORT		0x00000400
#define FORMAT_HAS_HOURS			0x00000800
#define FORMAT_HAS_MINUTES			0x00001000
#define FORMAT_HAS_SECONDS			0x00002000
#define FORMAT_REMOVE_ZERO_HOURS	0x00004000
#define FORMAT_REMOVE_ZERO_MINUTES	0x00008000
#define FORMAT_REMOVE_ZERO_SECONDS	0x00010000
#define FORMAT_HAS_MASK				0x0000380E

#define DATE_DAY					0
#define DATE_MONTH					1
#define DATE_YEAR					2

#define CACHE_SIZE					1024


typedef struct {
	QString		fKey;
	QString		fFormat;
	FormatInfo	fInfo[2];
	QString		fHumanFormat;
	QString		fPattern;
} FORMAT_INFO;


static const QChar kLocalMonSymbol = 0x20AC;
static QMutex sFormatCacheMutex;
static QCache<QString, FORMAT_INFO> sFormatCache(1000);


static bool
isSpace(const QChar& c)
{
	return (((c) == QLatin1Char(' ')) || ((c) == QChar::Nbsp) || ((c) == QChar::LineSeparator) || ((c) == QLatin1Char('\t')));
}


static bool
isSeparator(const QChar& c)
{
	switch (c.toLatin1()) {
	case '.':
	case ',':
	case '?':
	case '!':
	case ':':
	case ';':
	case '-':
	case '<':
	case '>':
	case '[':
	case ']':
	case '(':
	case ')':
	case '{':
	case '}':
	case '=':
	case '/':
	case '+':
	case '%':
	case '&':
	case '^':
	case '*':
	case '\'':
	case '"':
	case '~':
	case '|':
		return true;
	default:
		return false;
	}
}


	
class Validator : public QRegExpValidator
{
	Q_OBJECT

public:
	Validator(QObject *parent) : QRegExpValidator(parent) {}
	
	virtual State validate(QString& input, int& pos) const
	{
		FormattedLineEdit *widget = (FormattedLineEdit *)parent();
		if (widget->isCapsOnly())
			input = input.toUpper();
		if (input.isEmpty())
			return Acceptable;
		return QRegExpValidator::validate(input, pos);
	}
};



class Icon : public QAbstractButton
{
	Q_OBJECT
	
public:
	Icon(FormattedLineEdit *parent, const QIcon& icon)
		: QAbstractButton(parent), fIcon(icon)
	{
		setFocusPolicy(Qt::NoFocus);
		setCursor(Qt::PointingHandCursor);
		connect(this, SIGNAL(clicked()), parent, SIGNAL(iconClicked()));
		setEnabled(parent->isEnabled());
	}
	
	virtual ~Icon() {}
	
	QIcon icon() { return fIcon; }
	
protected:
	virtual void changeEvent(QEvent *event)
	{
		if (event->type() == QEvent::EnabledChange) {
			setCursor(isEnabled() ? Qt::PointingHandCursor : Qt::ArrowCursor);
			update();
		}
	}

	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		
		QIcon::Mode mode = isEnabled() ? (isDown() ? QIcon::Selected : QIcon::Normal) : QIcon::Disabled;
		QRect rect = painter.viewport();
		QSize size = fIcon.actualSize(rect.size(), mode);
		
		if (rect.width() > size.width())
			rect.moveLeft(rect.left() + ((rect.width() - size.width()) / 2));
		if (rect.height() > size.height())
			rect.moveTop(rect.top() + ((rect.height() - size.height()) / 2));
		
		painter.drawPixmap(rect.topLeft(), fIcon.pixmap(size, mode));
	}
	
	virtual void mouseReleaseEvent(QMouseEvent *event)
	{
		parent()->event(event);
		if (event->isAccepted())
			QAbstractButton::mouseReleaseEvent(event);
	}

private:
	QIcon					fIcon;
};



void
parseFormat(const QString& _format, int dataType, FormatInfo *formatInfo, QString *humanFormat, QRegExp *regExp)
{
	QMutexLocker locker(&sFormatCacheMutex);
	QString key = QString("%1__%2").arg(dataType).arg(_format);
	FORMAT_INFO *cached_info = sFormatCache[key];
	if (!cached_info) {
		cached_info = new FORMAT_INFO;
	
		QLocale locale = getLocale();
		QString format = locale.dateFormat(QLocale::NarrowFormat);
		QChar defaultDSep, defaultTSep;
		int i, datePos[3];
		
		switch (dataType) {
		case SL_DATATYPE_DATE:
		case SL_DATATYPE_TIME:
		case SL_DATATYPE_TIMESTAMP:
			{
				for (i = 0; i < format.size(); i++) {
					if ((!format[i].isLetter()) && (!format[i].isSpace())) {
						defaultDSep = format[i];
						break;
					}
				}
				
				if (format.indexOf('M') < format.indexOf('d')) {
					datePos[0] = DATE_MONTH;
					datePos[1] = DATE_DAY;
				}
				else {
					datePos[0] = DATE_DAY;
					datePos[1] = DATE_MONTH;
				}
				datePos[2] = DATE_YEAR;
				
				format = locale.timeFormat(QLocale::NarrowFormat);
				for (i = 0; i < format.size(); i++) {
					if ((!format[i].isLetter()) && (!format[i].isSpace())) {
						defaultTSep = format[i];
						break;
					}
				}
				format = _format;
				if (format.isEmpty())
					format = "dm2yHMS:PdmyHMS";
			}
			break;
		default:
			format = _format;
			break;
		}
		
		if (format.indexOf(":") == -1)
			format = QString("%1:%2").arg(_format).arg(_format);
		
		QStringList formatsList = format.split(":");
		
		for (int f = 0; f < 2; f++) {
			QString pattern;
			QString buffer = formatsList[f];
			FormatInfo *info = &formatInfo[f];
			
			info->fFlags = 0;
			info->fAlign = (Qt::Alignment)0;
			
			switch (dataType) {
			case SL_DATATYPE_INTEGER:
			case SL_DATATYPE_DECIMAL:
			case SL_DATATYPE_FLOAT:
				{
					info->fLen = info->fDecLen = -1;
					info->fMonSymbol = kLocalMonSymbol;
					for (int i = 0; i < buffer.length(); i++) {
						QChar c = buffer[i];
						switch (c.unicode()) {
						case 'b':	info->fFlags |= FORMAT_BLANK_IF_ZERO; break;
						case 'l':	info->fAlign = Qt::AlignLeft; break;
						case 'c':	info->fAlign = Qt::AlignHCenter; break;
						case 'r':	info->fAlign = Qt::AlignRight; break;
						case 't':	info->fFlags |= FORMAT_THOUSANDS_SEP; break;
						case 'u':	info->fFlags |= FORMAT_UNSIGNED; break;
						case 'k':	info->fFlags |= FORMAT_RED_IF_NEGATIVE; break;
						case 'p':	info->fFlags |= FORMAT_PERCENTAGE; break;
						case 'm':	if (dataType != SL_DATATYPE_INTEGER) info->fFlags |= FORMAT_MONETARY; break;
						case 'e':
							{
								info->fFlags |= FORMAT_MONETARY_SYMBOL;
								if ((i + 1 < buffer.length()) && (buffer[i + 1] == '{')) {
									if (i + 2 < buffer.length()) {
										if (buffer[i + 2] != '}')
											info->fMonSymbol = buffer[i + 2];
										else
											info->fMonSymbol = 0;
									}
									int pos = buffer.indexOf('}', i);
									if (pos < 0)
										i = buffer.length();
									else
										i = pos;
								}
							}
							break;
						default:
							{
								bool hasDec = false;
								
								if (c.isDigit()) {
									if (c == '0')
										info->fFlags |= FORMAT_ZERO_FILL;
									
									int count = buffer.indexOf('.', i);
									if (count != -1) {
										hasDec = true;
										count -= i;
									}
									else {
										count = 1;
										while ((i + count < buffer.length()) && (buffer[i + count].isDigit()))
											count++;
									}
									info->fLen = buffer.mid(i, count).toUInt();
									i += count - 1;
									if (hasDec)
										i++;
								}
								if ((hasDec) || (c == '.')) {
									int count = 1;
									while ((i + count < buffer.length()) && (buffer[i + count].isDigit()))
										count++;
									info->fDecLen = buffer.mid(i + 1, count - 1).toUInt();
									i += count - 1;
								}
							}
							break;
						}
					}
					if (f == 0) {
						pattern = QString("(\\d{0,%1})?");
						if (info->fLen >= 0)
							pattern = pattern.arg(info->fLen);
						else
							pattern = pattern.arg("");
						if (!(info->fFlags & FORMAT_UNSIGNED))
							pattern.prepend("([+\x002D])?");
						else
							pattern.prepend("([+])?");
						if (dataType != SL_DATATYPE_INTEGER) {
							pattern.append(QString("(([.,])(\\d{0,%1})?)?"));
							if (info->fDecLen >= 0)
								pattern = pattern.arg(info->fDecLen);
							else
								pattern = pattern.arg("");
						}
						for (int i = (info->fLen < 0 ? 10 : info->fLen); i >= 0; i--)
							cached_info->fHumanFormat.append('N');
						if (dataType != SL_DATATYPE_INTEGER) {
							cached_info->fHumanFormat.append("[.");
							for (int i = (info->fDecLen < 0 ? 10 : info->fDecLen); i >= 0; i--)
								cached_info->fHumanFormat.append('N');
							cached_info->fHumanFormat.append("]");
						}
					}
				}
				break;
			
			case SL_DATATYPE_DATE:
			case SL_DATATYPE_TIME:
			case SL_DATATYPE_TIMESTAMP:
				{
					QChar dSep = defaultDSep;
					QChar tSep = defaultTSep;
					QString format;
					
					for (int i = 0; i < buffer.length(); i++) {
						switch (buffer[i].unicode()) {
						case 'l':	info->fAlign = Qt::AlignLeft; break;
						case 'c':	info->fAlign = Qt::AlignHCenter; break;
						case 'r':	info->fAlign = Qt::AlignRight; break;
						case 'P':
							{
								info->fFlags |= FORMAT_HAS_SEPARATOR;
								if ((i + 1 < buffer.length()) && (buffer[i+1] == '{')) {
									if (i + 2 < buffer.length()) {
										if ((dataType == SL_DATATYPE_DATE) || (dataType == SL_DATATYPE_TIMESTAMP))
											dSep = buffer[i + 2];
										else
											tSep = buffer[i + 2];
										if (i + 3 < buffer.length())
											tSep = buffer[i + 3];
									}
									int pos = buffer.indexOf('}', i);
									if (pos < 0)
										i = buffer.length();
									else
										i = pos;
								}
							}
							break;
						}
						if ((dataType == SL_DATATYPE_DATE) || (dataType == SL_DATATYPE_TIMESTAMP)) {
							switch (buffer[i].unicode()) {
							case 'w':
								{
									info->fFlags |= FORMAT_HAS_WEEKDAY;
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 'a')) {
										i++;
										info->fFlags |= FORMAT_WEEKDAY_IS_SHORT;
									}
								}
							case 'd':
								{
									info->fFlags |= FORMAT_HAS_DAY;
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 's')) {
										i++;
										info->fFlags |= FORMAT_REMOVE_ZERO_DAY;
									}
								}
								break;
							case 'm':
								{
									info->fFlags |= FORMAT_HAS_MONTH;
									while (i + 1 < buffer.length()) {
										if (buffer[i+1] == 's')
											info->fFlags |= FORMAT_REMOVE_ZERO_MONTH;
										else if (buffer[i+1] == 'n')
											info->fFlags |= FORMAT_MONTH_NAME;
										else if (buffer[i+1] == 'a')
											info->fFlags |= FORMAT_MONTH_NAME_IS_SHORT;
										else
											break;
										i++;
									}
								}
								break;
							case '2':
								{
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 'y')) {
										info->fFlags |= FORMAT_HAS_YEAR | FORMAT_YEAR_IS_SHORT;
										i++;
									}
								}
								break;
							case 'y':	info->fFlags |= FORMAT_HAS_YEAR; break;
							default:	break;
							}
						}
						if ((dataType == SL_DATATYPE_TIME) || (dataType == SL_DATATYPE_TIMESTAMP)) {
							switch (buffer[i].unicode()) {
							case 'H':
								{
									info->fFlags |= FORMAT_HAS_HOURS;
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 's')) {
										info->fFlags |= FORMAT_REMOVE_ZERO_HOURS;
										i++;
									}
								}
								break;
							case 'M':
								{
									info->fFlags |= FORMAT_HAS_MINUTES;
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 's')) {
										info->fFlags |= FORMAT_REMOVE_ZERO_MINUTES;
										i++;
									}
								}
								break;
							case 'S':
								{
									info->fFlags |= FORMAT_HAS_SECONDS;
									if ((i + 1 < buffer.length()) && (buffer[i+1] == 's')) {
										info->fFlags |= FORMAT_REMOVE_ZERO_SECONDS;
										i++;
									}
								}
								break;
							default:	break;
							}
						}
					}
					if (f == 0)
						info->fFlags &= ~(FORMAT_HAS_WEEKDAY | FORMAT_MONTH_NAME);
					if (info->fFlags & FORMAT_HAS_WEEKDAY)
						info->fFlags |= FORMAT_HAS_SEPARATOR;
					if ((info->fFlags & (FORMAT_REMOVE_ZERO_DAY | FORMAT_REMOVE_ZERO_MONTH)) && (!(info->fFlags & (FORMAT_HAS_WEEKDAY | FORMAT_MONTH_NAME))))
						info->fFlags |= FORMAT_HAS_SEPARATOR;
					if (info->fFlags & (FORMAT_REMOVE_ZERO_HOURS | FORMAT_REMOVE_ZERO_MINUTES | FORMAT_REMOVE_ZERO_SECONDS))
						info->fFlags |= FORMAT_HAS_SEPARATOR;
					if (!(info->fFlags & (FORMAT_HAS_WEEKDAY | FORMAT_HAS_DAY | FORMAT_HAS_MONTH | FORMAT_HAS_YEAR | FORMAT_HAS_HOURS | FORMAT_HAS_MINUTES | FORMAT_HAS_SECONDS))) {
	// 					info->fFlags = FORMAT_HAS_DAY | FORMAT_HAS_MONTH | FORMAT_HAS_YEAR | FORMAT_HAS_HOURS | FORMAT_HAS_MINUTES | FORMAT_HAS_SECONDS;
						if (f == 0)
							info->fFlags |= FORMAT_YEAR_IS_SHORT;
						else
							info->fFlags |= FORMAT_HAS_SEPARATOR;
					}
					if (info->fFlags & FORMAT_MONTH_NAME)
						dSep = ' ';
					
					if ((dataType == SL_DATATYPE_DATE) || (dataType == SL_DATATYPE_TIMESTAMP)) {
						for (int i = 0; i < 3; i++) {
							switch (datePos[i]) {
							case DATE_DAY:
								{
									if (!(info->fFlags & FORMAT_HAS_DAY))
										continue;
									if (!format.isEmpty()) {
										if (info->fFlags & FORMAT_HAS_SEPARATOR) {
											format.append(QString("'%1'").arg(dSep));
											if (f == 0) {
												pattern = pattern.append(QString("\\x%1").arg(dSep.unicode(), 4, 16, QChar('0')));
												cached_info->fHumanFormat.append(dSep);
											}
										}
										else if ((i > 0) && (datePos[i - 1] == DATE_MONTH) && (info->fFlags & FORMAT_MONTH_NAME))
											format.append(" ");
									}
									if ((f != 0) && (info->fFlags & FORMAT_HAS_WEEKDAY)) {
										if (info->fFlags & FORMAT_WEEKDAY_IS_SHORT)
											format.append("ddd ");
										else
											format.append("dddd ");
									}
									format.append("d");
									if (!(info->fFlags & FORMAT_REMOVE_ZERO_DAY)) {
										format.append("d");
									}
									if (f == 0) {
										pattern.append("(\\d{1%1})?");
										cached_info->fHumanFormat.append("D");
										if (info->fFlags & FORMAT_REMOVE_ZERO_DAY)
											pattern = pattern.arg("");
										else {
											pattern = pattern.arg(",2");
											cached_info->fHumanFormat.append("D");
										}
									}
								}
								break;
							case DATE_MONTH:
								{
									if (!(info->fFlags & FORMAT_HAS_MONTH))
										continue;
									if (!format.isEmpty()) {
										if (info->fFlags & FORMAT_HAS_SEPARATOR) {
											format.append(QString("'%1'").arg(dSep));
											if (f == 0) {
												pattern = pattern.append(QString("\\x%1").arg(dSep.unicode(), 4, 16, QChar('0')));
												cached_info->fHumanFormat.append(dSep);
											}
										}
										else if ((i > 0) && (datePos[i - 1] == DATE_DAY) && (info->fFlags & FORMAT_MONTH_NAME))
											format.append(" ");
									}
									if ((f != 0) && (info->fFlags & FORMAT_MONTH_NAME)) {
										if (info->fFlags & FORMAT_MONTH_NAME_IS_SHORT)
											format.append("MMM");
										else
											format.append("MMMM");
									}
									else {
										format.append("M");
										if (!(info->fFlags & FORMAT_REMOVE_ZERO_MONTH)) {
											format.append("M");
										}
									}
									if (f == 0) {
										pattern.append("(\\d{1%1})?");
										cached_info->fHumanFormat.append("M");
										if (info->fFlags & FORMAT_REMOVE_ZERO_MONTH)
											pattern = pattern.arg("");
										else {
											pattern = pattern.arg(",2");
											cached_info->fHumanFormat.append("M");
										}
									}
								}
								break;
							case DATE_YEAR:
								{
									if (!(info->fFlags & FORMAT_HAS_YEAR))
										continue;
									if (!format.isEmpty()) {
										if (info->fFlags & FORMAT_HAS_SEPARATOR) {
											format.append(QString("'%1'").arg(dSep));
											if (f == 0) {
												pattern = pattern.append(QString("\\x%1").arg(dSep.unicode(), 4, 16, QChar('0')));
												cached_info->fHumanFormat.append(dSep);
											}
										}
										else if ((i > 0) && (info->fFlags & (FORMAT_MONTH_NAME | FORMAT_HAS_WEEKDAY))) {
											if (datePos[i - 1] == DATE_DAY)
												format.append(", ");
											else
												format.append(" ");
										}
									}
									format.append("yy");
									if (!(info->fFlags & FORMAT_YEAR_IS_SHORT)) {
										format.append("yy");
									}
									if (f == 0) {
										pattern.append("(\\d{1,%1})?");
										cached_info->fHumanFormat.append(QString("YY").repeated(info->fFlags & FORMAT_YEAR_IS_SHORT ? 1 : 2));
										pattern = pattern.arg(info->fFlags & FORMAT_YEAR_IS_SHORT ? 2 : 4);
									}
								}
								break;
							}
						}
						
						if ((info->fFlags & (FORMAT_HAS_HOURS | FORMAT_HAS_MINUTES | FORMAT_HAS_SECONDS)) && (!format.isEmpty()) && (info->fFlags & FORMAT_HAS_SEPARATOR)) {
							if (f == 0)
								format.append(" ");
							else {
								format.append(", ");
								pattern.append("\\s?");
								cached_info->fHumanFormat.append(" ");
							}
						}
					}
					
					if ((dataType == SL_DATATYPE_TIME) || (dataType == SL_DATATYPE_TIMESTAMP)) {
						if (info->fFlags & FORMAT_HAS_HOURS) {
							format.append("h");
							if (!(info->fFlags & FORMAT_REMOVE_ZERO_HOURS)) {
								format.append("h");
							}
							if (f == 0) {
								pattern.append("(\\d{1%1})?");
								cached_info->fHumanFormat.append("h");
								if (info->fFlags & FORMAT_REMOVE_ZERO_HOURS)
									pattern = pattern.arg("");
								else {
									pattern = pattern.arg(",2");
									cached_info->fHumanFormat.append("h");
								}
							}
						}
						if (info->fFlags & FORMAT_HAS_MINUTES) {
							if ((info->fFlags & FORMAT_HAS_SEPARATOR) && (!format.isEmpty())) {
								format.append(QString("'%1'").arg(tSep));
								if (f == 0) {
									pattern = pattern.append(QString("\\x%1").arg(tSep.unicode(), 4, 16, QChar('0')));
									cached_info->fHumanFormat.append(tSep);
								}
							}
							format.append("m");
							if (!(info->fFlags & FORMAT_REMOVE_ZERO_MINUTES)) {
								format.append("m");
							}
							if (f == 0) {
								pattern.append("(\\d{1%1})?");
								cached_info->fHumanFormat.append("m");
								if (info->fFlags & FORMAT_REMOVE_ZERO_MINUTES)
									pattern = pattern.arg("");
								else {
									pattern = pattern.arg(",2");
									cached_info->fHumanFormat.append("m");
								}
							}
						}
						if (info->fFlags & FORMAT_HAS_SECONDS) {
							if ((info->fFlags & FORMAT_HAS_SEPARATOR) && (!format.isEmpty())) {
								format.append(QString("'%1'").arg(tSep));
								if (f == 0) {
									pattern = pattern.append(QString("\\x%1").arg(tSep.unicode(), 4, 16, QChar('0')));
									cached_info->fHumanFormat.append(tSep);
								}
							}
							format.append("s");
							if (!(info->fFlags & FORMAT_REMOVE_ZERO_SECONDS)) {
								format.append("s");
							}
							if (f == 0) {
								pattern.append("(\\d{1%1})?");
								cached_info->fHumanFormat.append("s");
								if (info->fFlags & FORMAT_REMOVE_ZERO_SECONDS)
									pattern = pattern.arg("");
								else {
									pattern = pattern.arg(",2");
									cached_info->fHumanFormat.append("s");
								}
							}
						}
					}
					
	// 				qDebug() << "DT Format is:" << format;
					info->fDTFormat = format;
				}
				break;
			
			case SL_DATATYPE_YEAR:
				{
					info->fFlags |= FORMAT_HAS_YEAR;
					for (int i = 0; i < buffer.length(); i++) {
						switch (buffer[i].unicode()) {
						case 'l':	info->fAlign = Qt::AlignLeft; break;
						case 'c':	info->fAlign = Qt::AlignHCenter; break;
						case 'r':	info->fAlign = Qt::AlignRight; break;
						case 's':	info->fFlags |= FORMAT_YEAR_IS_SHORT; break;
						default:	break;
						}
					}
					pattern = QString("(\\d{1,%1})").arg(info->fFlags & FORMAT_YEAR_IS_SHORT ? 2 : 4);
				}
				break;
			
			case SL_DATATYPE_STRING:
				{
					for (int i = 0; i < buffer.length(); i++) {
						switch (buffer[i].unicode()) {
						case 'l':	info->fAlign = Qt::AlignLeft; break;
						case 'c':	info->fAlign = Qt::AlignHCenter; break;
						case 'r':	info->fAlign = Qt::AlignRight; break;
						default:	break;
						}
					}
				}
				/* fallthrough */
			
			default:
				{
					pattern = ".*";
				}
				break;
			}
			
			if (f == 0) {
				cached_info->fPattern = pattern;
	// 			fprintf(stderr, "pattern: %s\n", (const char *)pattern.toUtf8());
			}
		}
		cached_info->fFormat = _format;
		cached_info->fInfo[0] = formatInfo[0];
		cached_info->fInfo[1] = formatInfo[1];
		
		sFormatCache.insert(key, cached_info);
	}
	
	formatInfo[0] = cached_info->fInfo[0];
	formatInfo[1] = cached_info->fInfo[1];
	if (regExp)
		regExp->setPattern(cached_info->fPattern);
	if (humanFormat)
		*humanFormat = cached_info->fHumanFormat;
}


QString
getFormattedValue(const QString& input, QColor *color, Qt::Alignment *align, int dataType, FormatInfo *formatInfo, bool editMode)
{
	QLocale locale = getLocale();
	QString output;
	const FormatInfo *info = &formatInfo[editMode ? 0 : 1];
	
	switch (dataType) {
	case SL_DATATYPE_INTEGER:
	case SL_DATATYPE_DECIMAL:
	case SL_DATATYPE_FLOAT:
		{
			QString iPart, fPart;
			QChar tSep, dSep;
			bool neg, overflow = false, zero;
			
			tSep = locale.groupSeparator();
			dSep = locale.decimalPoint();
			
			int pos = input.indexOf('.');
			if (pos < 0)
				iPart = input;
			else {
				iPart = input.mid(0, pos);
				fPart = input.mid(pos+1);
			}
			neg = (iPart[0] == '-');
			if (neg)
				iPart = iPart.mid(1);
			while ((!iPart.isEmpty()) && (iPart[0] == '0'))
				iPart = iPart.mid(1);
			if (iPart.isEmpty())
				iPart = "0";
			if ((info->fLen >= 0) && (iPart.length() > info->fLen)) {
				iPart = QString("9").repeated(info->fLen);
				fPart = info->fDecLen < 0 ? "" : QString("9").repeated(info->fDecLen);
				overflow = true;
			}
// 			fprintf(stderr, "fIntValue = '%s', iPart = '%s', fPart = '%s'\n", (const char *)fIntValue.toUtf8(), (const char *)iPart.toUtf8(), (const char *)fPart.toUtf8());
			if (!editMode) {
				int len = iPart.length();
				if (info->fFlags & FORMAT_THOUSANDS_SEP) {
					for (int i = 0; i < len; i++) {
						if ((i > 0) && ((i % 3) == 0))
							output.prepend(tSep);
						output.prepend(iPart[len - i - 1]);
					}
				}
				else {
					if ((info->fLen >= 0) && (info->fLen > len) && (info->fFlags & FORMAT_ZERO_FILL))
						iPart.prepend(QString("0").repeated(info->fLen - len));
					output = iPart;
				}
			}
			else {
				output = iPart;
			}
			
			if (info->fDecLen > 0) {
				if (fPart.length() > info->fDecLen) {
					if (info->fFlags & FORMAT_MONETARY) {
						double remaining = QString("0.%1").arg(fPart.mid(info->fDecLen)).toDouble();
						fPart.truncate(info->fDecLen);
						if (remaining > 0.5) {
							if (fPart == QString("9").repeated(info->fDecLen)) {
								if (!overflow) {
									fPart = "";
									iPart = QString::number(iPart.toULongLong() + 1);
								}
							}
							else {
								int length = fPart.length();
								int leadingZeroes = 0;
								QString temp = fPart;
								while ((leadingZeroes < length) && (temp[leadingZeroes] == '0')) {
									leadingZeroes++;
								}
								temp.truncate(leadingZeroes);
								fPart = temp + QString::number(fPart.toULongLong() + 1);
								if (fPart.length() > length) {
									fPart = "";
									iPart = QString::number(iPart.toULongLong() + 1);
								}
							}
							if ((!overflow) && (info->fLen >= 0) && (iPart.length() > info->fLen)) {
								iPart = QString("9").repeated(info->fLen);
								fPart = info->fDecLen < 0 ? "" : QString("9").repeated(info->fDecLen);
								overflow = true;
							}
						}
					}
					else
						fPart.truncate(info->fDecLen);
				}
				else if ((fPart.length() < info->fDecLen) && ((!editMode) || (info->fFlags & FORMAT_MONETARY)))
					fPart.append(QString("0").repeated(info->fDecLen - fPart.length()));
			}
			if ((info->fDecLen) && (dataType != SL_DATATYPE_INTEGER)) {
				unsigned int temp = fPart.toUInt();
// 				fprintf(stderr, "dec part = '%s' (%d)\n", (const char *)fPart.toUtf8(), temp);
				if ((temp) || (info->fFlags & FORMAT_MONETARY) || (!editMode)) {
					if (fPart.isEmpty())
						fPart = "0";
					output += dSep + fPart;
				}
				zero = (iPart.toULongLong() == 0) && (temp == 0);
			}
			else {
				zero = (iPart.toULongLong() == 0);
			}
			
			if ((zero) && (info->fFlags & FORMAT_BLANK_IF_ZERO)) {
				output = "";
			}
			else {
				if ((neg) && (!(info->fFlags & FORMAT_UNSIGNED)) && (!zero)) {
					if (info->fFlags & FORMAT_RED_IF_NEGATIVE)
						*color = QColor(Qt::red);
					output.prepend("-");
				}
				
				if (!editMode) {
					if (overflow)
						output.prepend(neg ? "< " : "> ");
					if (info->fFlags & FORMAT_PERCENTAGE)
						output.append(" %");
					else if ((info->fFlags & FORMAT_MONETARY_SYMBOL) && (info->fMonSymbol != 0)) {
						output.append(QString(" %1").arg(info->fMonSymbol));
					}
				}
			}
		}
		break;
		
	case SL_DATATYPE_DATE:
	case SL_DATATYPE_TIME:
	case SL_DATATYPE_TIMESTAMP:
		{
			QDateTime dt;
			QString format;
			
			switch (dataType) {
			case SL_DATATYPE_DATE:
				{
					if (input != "0000-00-00") {
						dt = QDateTime::fromString(input, "yyyy-MM-dd");
						if (!dt.isValid()) {
							dt = QDateTime::fromString(input, "yyyy-MM-dd HH:mm:ss");
							if (dt.isValid())
								dt.setTime(QTime(0,0));
						}
					}
				}
				break;
			case SL_DATATYPE_TIME:
				{
					if (input != "00:00:00") {
						dt = QDateTime::fromString(input, "HH:mm:ss");
						if (!dt.isValid()) {
							dt = QDateTime::fromString(input, "yyyy-MM-dd HH:mm:ss");
							if (dt.isValid())
								dt.setDate(QDate(1970,1,1));
						}
					}
				}
				break;
			case SL_DATATYPE_TIMESTAMP:
				{
					if (input != "0000-00-00 00:00:00") {
						dt = QDateTime::fromString(input, "yyyy-MM-dd HH:mm:ss");
						if (!dt.isValid()) {
							dt = QDateTime::fromString(input, "yyyy-MM-dd");
							if (dt.isValid()) {
								dt.setTime(QTime(0,0));
							}
							else {
								dt = QDateTime::fromString(input, "HH:mm:ss");
								if (dt.isValid())
									dt.setDate(QDate(1970,1,1));
							}
						}
					}
				}
				break;
			}
			if (!dt.isValid()) {
				if (editMode)
					output = input;
				break;
			}
			
			output = dt.toString(info->fDTFormat);
		}
		break;
	
	case SL_DATATYPE_YEAR:
		{
			bool ok;
			unsigned int year = input.toUInt(&ok);
			if (ok) {
				if (info->fFlags & FORMAT_YEAR_IS_SHORT)
					output = QString::number(year % 100);
				else
					output = QString::number(year);
			}
			else
				output = input;
		}
		break;
		
	default:
		output = input;
		break;
	}

	*align = info->fAlign;
	
	return output;
}



FormattedLineEdit::FormattedLineEdit(QWidget *parent)
	: QLineEdit(parent), fDataType(0), fAlign(Qt::AlignLeft), fSelectedOnFocus(true), fCapsOnly(false), fEnterTabs(true),
		fIcon(NULL), fCompleter(NULL), fCursorPosition(0), fCursorEndPosition(0)
{
	fRegExp = QRegExp(".*");
	fValidator = new Validator(this);
	fValidator->setRegExp(fRegExp);
	setValidator(fValidator);
	
	for (int i = 0; i < 2; i++) {
		FormatInfo *info = &fFormatInfo[i];
		info->fFlags = 0;
		info->fAlign = fAlign;
		info->fLen = info->fDecLen = -1;
	}
	setState(Acceptable);
	setDragEnabled(true);
}


FormattedLineEdit::~FormattedLineEdit()
{
// 	delete fModel;
	delete fValidator;
	delete fIcon;
}


void
FormattedLineEdit::setDataType(int dataType)
{
	fDataType = dataType;
	updateDisplay(hasFocus());
}


void
FormattedLineEdit::setAlignment(Qt::Alignment align)
{
	fAlign = align;
	updateDisplay(hasFocus());
}


void
FormattedLineEdit::setColor(const QColor& color)
{
	fColor = color;
	updateDisplay(hasFocus());
}


void
FormattedLineEdit::setText(const QString& value)
{
	setInternalValue(value);
	updateDisplay(hasFocus());
}


void
FormattedLineEdit::setState(State state)
{
	fState = state;
}


void
FormattedLineEdit::setFormat(const QString& format)
{
	parseFormat(format, fDataType, fFormatInfo, &fHumanFormat, &fRegExp);
	if (qobject_cast<SearchField_Impl *>(this))
		fFormatInfo[1].fFlags |= FORMAT_BLANK_IF_ZERO;
	updateDisplay(hasFocus());
	fFormat = format;
}


void
FormattedLineEdit::setInternalValue(const QString& value)
{
	QString result = value;
	
	switch (fDataType) {
	case SL_DATATYPE_INTEGER:
		{
			if (value.isEmpty())
				result = "0";
			else {
				bool ok;
				qlonglong v = value.toLongLong(&ok);
				result = ok ? QString::number(v) : "0";
			}
		}
		break;
	case SL_DATATYPE_DECIMAL:
	case SL_DATATYPE_FLOAT:
		{
			if (value.isEmpty())
				result = "0.0";
			else {
				result = result.replace(',','.');
				bool ok;
				double v = result.toDouble(&ok);
				if (!ok) {
					result = "0.0";
				}
				else {
					bool sign = ((result[0] == '+') || (result[0] == '-'));
					bool neg = (result[0] == '-');
					if (sign)
						result = result.mid(1);
					if (result.startsWith('.'))
						result.prepend('0');
					if (result.endsWith('.'))
						result.append('0');
					if ((neg) && (v != 0.0))
						result.prepend('-');
				}
			}
		}
		break;
	case SL_DATATYPE_DATE:
		{
			QDateTime dt(QDateTime::fromString(value, "yyyy-MM-dd"));
			if (dt.isValid())
				result = dt.toString("yyyy-MM-dd");
			else
				result = "";
		}
		break;
	case SL_DATATYPE_TIME:
		{
			QDateTime dt(QDateTime::fromString(value, "HH:mm:ss"));
			if (dt.isValid())
				result = dt.toString("HH:mm:ss");
			else
				result = "";
		}
		break;
	case SL_DATATYPE_TIMESTAMP:
		{
			QDateTime dt(QDateTime::fromString(value, "yyyy-MM-dd HH:mm:ss"));
			if (dt.isValid())
				result = dt.toString("yyyy-MM-dd HH:mm:ss");
			else
				result = "";
		}
		break;
	case SL_DATATYPE_YEAR:
		{
		}
		break;
	}
	
// 	fprintf(stderr, "Internal value = %s\n", (const char *)result.toUtf8());
	setState(Acceptable);
	fIntValue = result;
}


void
FormattedLineEdit::setInternalValueFromEditValue(const QString& value)
{
	QRegExp regExp(fRegExp);
	FormatInfo *info = &fFormatInfo[0];
	
	fIntValue = "";
	setState(regExp.exactMatch(value) ? Acceptable : Invalid);
	
	if (fState == Acceptable) {
		int pos = 0;
		QString temp(value);
		if (fValidator->validate(temp, pos) == QValidator::Invalid)
			setState(Invalid);
	}
	
// 	QString caps;
// 	for (int i = 0; i <= regExp.numCaptures(); i++)
// 		caps.append(QString("[%1] ").arg(regExp.cap(i)));
// 	fprintf(stderr, "Pattern = %s\n"
// 					"matched length = %d\n"
// 					"Caps = %s\n", (const char *)regExp.pattern().toUtf8(), regExp.matchedLength(), (const char *)caps.toUtf8());
	
	switch (fDataType) {
	case SL_DATATYPE_INTEGER:
		{
			bool ok;
			fIntValue = QString("%1%2").arg(regExp.cap(1)).arg(regExp.cap(2));
			fIntValue.toLongLong(&ok);
			if (!ok)
				fIntValue = "0";
		}
		break;
	case SL_DATATYPE_DECIMAL:
	case SL_DATATYPE_FLOAT:
		{
			bool ok;
			fIntValue = QString("%1%2.%3").arg(regExp.cap(1)).arg(regExp.cap(2)).arg(regExp.cap(5));
			double value = fIntValue.toDouble(&ok);
			if (!ok) {
				fIntValue = "0.0";
			}
			else {
				bool neg = (fIntValue[0] == '-');
				bool sign = neg || (fIntValue[0] == '+');
				if (sign)
					fIntValue = fIntValue.mid(1);
				if (fIntValue.startsWith('.'))
					fIntValue.prepend('0');
				if (fIntValue.endsWith('.'))
					fIntValue.append('0');
				if ((neg) && (value != 0.0))
					fIntValue.prepend('-');
			}
		}
		break;
	case SL_DATATYPE_DATE:
	case SL_DATATYPE_TIME:
	case SL_DATATYPE_TIMESTAMP:
		{
			if (value.isEmpty())
				return;
			QDate date = QDate::currentDate();
			QTime time = QTime::currentTime();
			int value, flags = 0, index = 1;
			int year = date.year(), month = date.month(), day = date.day();
			bool ok;
			QString dateFormat = getLocale().dateFormat(QLocale::NarrowFormat);
			
			if (dateFormat.indexOf('M') < dateFormat.indexOf('d')) {
				if (info->fFlags & FORMAT_HAS_MONTH) {
					value = regExp.cap(index++).toInt(&ok);
					if ((ok) && (value)) {
						flags |= FORMAT_HAS_MONTH;
						month = value;
					}
				}
				if (info->fFlags & FORMAT_HAS_DAY) {
					value = regExp.cap(index++).toInt(&ok);
					if ((ok) && (value)) {
						flags |= FORMAT_HAS_DAY;
						day = value;
					}
				}
			}
			else {
				if (info->fFlags & FORMAT_HAS_DAY) {
					value = regExp.cap(index++).toInt(&ok);
					if ((ok) && (value)) {
						flags |= FORMAT_HAS_DAY;
						day = value;
					}
				}
				if (info->fFlags & FORMAT_HAS_MONTH) {
					value = regExp.cap(index++).toInt(&ok);
					if ((ok) && (value)) {
						flags |= FORMAT_HAS_MONTH;
						month = value;
					}
				}
			}
			if (info->fFlags & FORMAT_HAS_YEAR) {
				value = regExp.cap(index++).toInt(&ok);
				if (ok) {
					flags |= FORMAT_HAS_YEAR;
					if (info->fFlags & FORMAT_YEAR_IS_SHORT) {
						if (value <= 30)
							value += 2000;
						else
							value += 1900;
					}
					year = value;
				}
			}

			date.setDate(year, month, day);
			if (info->fFlags & FORMAT_HAS_HOURS) {
				value = regExp.cap(index++).toInt(&ok);
				if (ok) {
					flags |= FORMAT_HAS_HOURS;
					time.setHMS(value, time.minute(), time.second());
				}
			}
			if ((info->fFlags & FORMAT_HAS_MINUTES) && (time.isValid())) {
				value = regExp.cap(index++).toInt(&ok);
				if (ok) {
					flags |= FORMAT_HAS_MINUTES;
					time.setHMS(time.hour(), value, time.second());
				}
			}
			if ((info->fFlags & FORMAT_HAS_SECONDS) && (time.isValid())) {
				value = regExp.cap(index++).toInt(&ok);
				if (ok) {
					flags |= FORMAT_HAS_SECONDS;
					time.setHMS(time.hour(), time.minute(), value);
				}
			}
			if ((fState == Acceptable) && ((!date.isValid()) || (!time.isValid()))) {
// 				fprintf(stderr, "Datetime valid ? %s/%s - flags: %04X/%04X\n", date.isValid()?"Y":"N", time.isValid()?"Y":"N", flags & FORMAT_HAS_MASK, info->fFlags & FORMAT_HAS_MASK);
				if ((flags ^ info->fFlags) & FORMAT_HAS_MASK)
					setState(Intermediate);
				else
					setState(Invalid);
				break;
			}
			
			switch (fDataType) {
			case SL_DATATYPE_DATE:		fIntValue = date.toString(Qt::ISODate); break;
			case SL_DATATYPE_TIME:		fIntValue = time.toString(Qt::ISODate); break;
			case SL_DATATYPE_TIMESTAMP:	fIntValue = QString("%1 %2").arg(date.toString(Qt::ISODate)).arg(time.toString(Qt::ISODate)); break;
			}
		}
		break;
	case SL_DATATYPE_YEAR:
		{
			if (value.isEmpty())
				return;
			if (regExp.cap(1).length() == 2) {
				unsigned int year = regExp.cap(1).toUInt();
				if (year <= 30)
					year += 2000;
				else
					year += 1900;
				fIntValue = QString::number(year);
			}
			else
				fIntValue = regExp.cap(1);
		}
		break;
	default:
		{
			fIntValue = value;
		}
		break;
	}
	
// 	fprintf(stderr, "Internal value from edit value = '%s', state = %s, pattern = %s\n", (const char *)fIntValue.toUtf8(), fState == Acceptable ? "Acceptable" : (fState == Invalid ? "Invalid" : "Intermediate"), (const char *)regExp.pattern().toUtf8());
}


void
FormattedLineEdit::updateDisplay(bool editMode)
{
	QString result;
	QColor color;
	Qt::Alignment alignment;
	
	if (fState == Acceptable) {
		result = getFormattedValue(fIntValue, &color, &alignment, fDataType, fFormatInfo, editMode && !isReadOnly());
		
		if (result != QLineEdit::text())
			QLineEdit::setText(result);
		if (alignment == (Qt::Alignment)0)
			alignment = fAlign;
		if (alignment != (QLineEdit::alignment() & Qt::AlignHorizontal_Mask))
			QLineEdit::setAlignment(alignment | Qt::AlignVCenter);
		if (!color.isValid())
			color = fColor;
 	}
	QPalette pal = palette();
	pal.setColor(QPalette::Text, color);
	setPalette(pal);
}


bool
FormattedLineEdit::isValidText(const QString& text)
{
	QString oldIntValue = fIntValue;
	State oldState = fState, newState;
	setInternalValueFromEditValue(text);
	newState = fState;
	fIntValue = oldIntValue;
	setState(oldState);
	return newState != Invalid;
}


bool
FormattedLineEdit::isValidInput(QKeyEvent *event, QString *output)
{
	int key = event->key();
	QString text = QLineEdit::text();
	QString insert = event->text();
	int start = selectionStart();
	int len = selectedText().length();
	int pos = cursorPosition();
	
	if (event == QKeySequence::Cut) {
		key = Qt::Key_Delete;
	}
	else if (event == QKeySequence::Paste) {
		handlePasteClipboard();
		insert = QApplication::clipboard()->text();
		key = -1;
	}
	else if (event == QKeySequence::DeleteEndOfWord) {
		start = pos;
		len = text.length();
		if (pos >= len) {
			if (output)
				*output = text;
			return true;
		}
		if ((pos < len) && (isSeparator(text.at(pos)))) {
			pos++;
			while ((pos < len) && (isSeparator(text.at(pos))))
				pos++;
		}
		else {
			while ((pos < len) && (!isSpace(text.at(pos))) && (!isSeparator(text.at(pos))))
				pos++;
		}
		while ((pos < len) && (isSpace(text.at(pos))))
			pos++;
		if (start == pos) {
			insert = "";
			key = -1;
		}
		else {
			len = pos - start + 1;
			key = Qt::Key_Delete;
		}
	}
	else if (event == QKeySequence::DeleteStartOfWord) {
		int end = pos;
		while ((pos) && (isSpace(text.at(pos - 1))))
			pos--;
		
		if ((pos) && (isSeparator(text.at(pos - 1)))) {
			pos--;
			while ((pos) && (isSeparator(text.at(pos - 1))))
				pos--;
		}
		else {
			while ((pos) && (!isSpace(text.at(pos - 1))) && (!isSeparator(text.at(pos - 1))))
				pos--;
		}
		start = pos;
		if (start == end) {
			insert = "";
			key = -1;
		}
		else {
			len = end - start + 1;
			key = Qt::Key_Delete;
		}
	}
	
	switch (key) {
	case Qt::Key_Backspace:
		{
			if (pos > 0) {
				if (start >= 0)
					text = text.remove(start, len);
				else
					text = text.remove(pos - 1, 1);
			}
		}
		break;
	case Qt::Key_Delete:
		{
			if (start >= 0)
				text = text.remove(start, len);
			else
				text = text.remove(pos, 1);
		}
		break;
	default:
		{
			if ((!insert.isEmpty()) && ((key == -1) || (insert.at(0).isPrint()))) {
				if (start >= 0) {
					text = text.remove(start, len);
					pos = start;
				}
				text.insert(pos, insert);
			}
		}
		break;
	}
	
	if (output)
		*output = text;
	return isValidText(text);
}


bool
FormattedLineEdit::canCut()
{
	int start = selectionStart();
	int end = start = selectedText().length();
	QString text = QLineEdit::text();
	
	if (start >= 0) {
		text = text.mid(0, start) + text.mid(end);
		if (!isValidText(text)) {
			return false;
		}
	}
	return true;
}


bool
FormattedLineEdit::canPaste()
{
	int start = selectionStart();
	int end = start = selectedText().length();
	QString text = QLineEdit::text();
	QString clipboard = QApplication::clipboard()->text();
	
	if (start >= 0) {
		text = text.mid(0, start) + clipboard + text.mid(end);
	}
	else {
		text += clipboard;
	}
	return isValidText(text);
}


void
FormattedLineEdit::keyPressEvent(QKeyEvent *event)
{
	QString text, oldText = QLineEdit::text();
	
	bool valid = isValidInput(event, &text);
	
	if ((text != oldText) && (isReadOnly()))
		valid = false;
	
	if (!valid) {
		QApplication::beep();
		return;
	}
	
	QLineEdit::keyPressEvent(event);
	text = QLineEdit::text();
	
	hidePopupMessage();
	
	if (text != oldText) {
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue);
	}
}


void
FormattedLineEdit::focusInEvent(QFocusEvent *event)
{
	QLineEdit::focusInEvent(event);
	
	if (event->reason() != Qt::PopupFocusReason) {
		QString text = QLineEdit::text();
		fCursorPosition = qMin(fCursorPosition, text.size());
		fCursorEndPosition = qMin(fCursorEndPosition, text.size());
		setCursorPosition(fCursorPosition);
		if (fCursorPosition != fCursorEndPosition)
			setSelection(fCursorPosition, fCursorEndPosition - fCursorPosition);
	}
	
	if ((fState == Acceptable) && (event->reason() != Qt::PopupFocusReason)) {
		updateDisplay(true);
		if ((fSelectedOnFocus) && (!Completer::eatFocus()))
			QMetaObject::invokeMethod(this, "selectAll", Qt::QueuedConnection);
	}
}


void
FormattedLineEdit::focusOutEvent(QFocusEvent *event)
{
	QLineEdit::focusOutEvent(event);
	
	if (event->reason() != Qt::PopupFocusReason) {
		fCursorPosition = cursorPosition();
		fCursorEndPosition = fCursorPosition + selectedText().length();
		setCursorPosition(0);
	}
	
	if ((fState == Acceptable) && (event->reason() != Qt::PopupFocusReason)) {
		setInternalValueFromEditValue(QLineEdit::text());
		updateDisplay(false);
	}
}


void
FormattedLineEdit::paintEvent(QPaintEvent *event)
{
	QLineEdit::paintEvent(event);
	
	if ((QLineEdit::text().isEmpty()) && (!hasFocus())) {
		QPainter painter(this);
		QStyleOptionFrameV2 panel;
		initStyleOption(&panel);
		QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
		QMargins margins = textMargins();
		
		painter.setPen(QColor(Qt::lightGray));
		painter.drawText(rect.adjusted(margins.left() + (fIcon ? 1 : 5), 0, -margins.right() - 5, 0), Qt::AlignLeft | Qt::AlignVCenter, fEmptyText);
	}
}


void
FormattedLineEdit::resizeEvent(QResizeEvent *event)
{
	int margin = 0;
	
	if (fIcon) {
		QStyleOptionFrameV2 panel;
		initStyleOption(&panel);
		QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
		margin = rect.height() + style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, this) + 1;
		rect.adjust(rect.width() - rect.height(), 0, 0, 0);
		fIcon->setGeometry(rect);
	}
	setTextMargins(0, 0, margin, 0);
	
	QLineEdit::resizeEvent(event);
}


void
FormattedLineEdit::changeEvent(QEvent *event)
{
	if ((event->type() == QEvent::EnabledChange) && (fIcon)) {
		fIcon->setEnabled(isEnabled());
	}
	QLineEdit::changeEvent(event);
}


void
FormattedLineEdit::dropEvent(QDropEvent *event)
{
	QString str = event->mimeData()->text();
	
	if (!str.isNull() && !isReadOnly()) {
		int oldCursorPos = cursorPosition();
		int oldSelStart = selectionStart();
		int oldSelLen = selectedText().length();
		
		deselect();
		setCursorPosition(cursorPositionAt(event->pos()));
		QKeyEvent ke(QEvent::KeyPress, 0, Qt::NoModifier, str);
		bool result = isValidInput(&ke);
		
		setCursorPosition(oldCursorPos);
		if (oldSelStart >= 0)
			setSelection(oldSelStart, oldSelLen);
		
		if (result) {
			QLineEdit::dropEvent(event);
			setInternalValueFromEditValue(QLineEdit::text());
			return;
		}
	}
	
	event->ignore();
	dragLeaveEvent(NULL);
}


void
FormattedLineEdit::contextMenuEvent(QContextMenuEvent *event)
{
	emit contextMenu();
}


QMenu *
FormattedLineEdit::createContextMenu()
{
	QMenu *popup = new QMenu();
	QAction *action;
	
	if (!isReadOnly()) {
		action = popup->addAction(QLineEdit::tr("&Undo") + SL_ACCEL_KEY(QKeySequence::Undo));
		action->setEnabled(isUndoAvailable());
		connect(action, SIGNAL(triggered()), SLOT(handleUndo()));
		
		action = popup->addAction(QLineEdit::tr("&Redo") + SL_ACCEL_KEY(QKeySequence::Redo));
		action->setEnabled(isRedoAvailable());
		connect(action, SIGNAL(triggered()), SLOT(handleRedo()));
		
		popup->addSeparator();
		
		action = popup->addAction(QLineEdit::tr("Cu&t") + SL_ACCEL_KEY(QKeySequence::Cut));
		action->setEnabled(!isReadOnly() && hasSelectedText() && echoMode() == QLineEdit::Normal);
		connect(action, SIGNAL(triggered()), SLOT(handleCut()));
	}
	
	action = popup->addAction(QLineEdit::tr("&Copy") + SL_ACCEL_KEY(QKeySequence::Copy));
	action->setEnabled(hasSelectedText() && echoMode() == QLineEdit::Normal);
	connect(action, SIGNAL(triggered()), SLOT(copy()));
	
	if (!isReadOnly()) {
		action = popup->addAction(QLineEdit::tr("&Paste") + SL_ACCEL_KEY(QKeySequence::Paste));
		action->setEnabled(!isReadOnly() && !QApplication::clipboard()->text().isEmpty());
		connect(action, SIGNAL(triggered()), SLOT(handlePaste()));
		
		action = popup->addAction(QLineEdit::tr("Delete"));
		action->setEnabled(!isReadOnly() && !text().isEmpty() && hasSelectedText());
		connect(action, SIGNAL(triggered()), SLOT(handleDelete()));
	}
	
	popup->addSeparator();
	
	action = popup->addAction(QLineEdit::tr("Select All") + SL_ACCEL_KEY(QKeySequence::SelectAll));
	action->setEnabled(!text().isEmpty() && (selectedText() != text()));
	connect(action, SIGNAL(triggered()), SLOT(selectAll()));
	
	return popup;
}


void
FormattedLineEdit::handleUndo()
{
	if (canModify(this)) {
		undo();
		QString text = QLineEdit::text();
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue, -1);
	}
}


void
FormattedLineEdit::handleRedo()
{
	if (canModify(this)) {
		redo();
		QString text = QLineEdit::text();
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue, -1);
	}
}


void
FormattedLineEdit::handleCut()
{
	if (!canCut()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		cut();
		QString text = QLineEdit::text();
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue, -1);
	}
}


void
FormattedLineEdit::handlePasteClipboard()
{
	QString text = QApplication::clipboard()->text();
	if (modifyClipboardOnPaste(text))
		QApplication::clipboard()->setText(text);
}


void
FormattedLineEdit::handlePaste()
{
	if (!canPaste()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		handlePasteClipboard();
		paste();
		QString text = QLineEdit::text();
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue, -1);
	}
}


void
FormattedLineEdit::handleDelete()
{
	if (!canCut()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		del();
		QString text = QLineEdit::text();
		setInternalValueFromEditValue(text);
		emit textModified(fIntValue, -1);
	}
}


QAbstractButton *
FormattedLineEdit::createIconButton(const QIcon& icon)
{
	return new Icon(this, icon);
}


void
FormattedLineEdit::setIcon(const QIcon& icon)
{
	delete fIcon;
	fIcon = NULL;
	
	if (!icon.isNull()) {
		fIcon = createIconButton(icon);
// 		fIcon->setEnabled(isEnabled());
		fIcon->show();
	}
	QResizeEvent re(size(), size());
	resizeEvent(&re);
	
	update();
}


QIcon
FormattedLineEdit::icon()
{
	if (fIcon)
		return fIcon->icon();
	return QIcon();
}


void
FormattedLineEdit::setCompleter(DataModel_Impl *model, int column, const QColor& color, const QColor& bgcolor, const QColor& hicolor, const QColor& hibgcolor)
{
	delete fCompleter;
	if (model == NULL) {
		fCompleter = NULL;
	}
	else {
		fCompleter = new Completer(this, model, column, color, bgcolor, hicolor, hibgcolor);
		connect(this, SIGNAL(doComplete()), fCompleter, SLOT(handleComplete()));
	}
}


void
FormattedLineEdit::handleComplete(int index)
{
	emit textModified(QLineEdit::text(), index);
}




#include "format.moc"

