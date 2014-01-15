#include "slew.h"
#include "objects.h"

#include <QDesktopWidget>
#include <QTableView>
#include <QHeaderView>
#include <QItemDelegate>
#include <QKeyEvent>


class ItemDelegate;
class CompleterPopup;


static CompleterPopup *sPopup = NULL;
static bool sEatFocus = false;




class CompleterItemDelegate : public QItemDelegate
{
	Q_OBJECT
	
public:
	CompleterItemDelegate(QObject *parent, DataModel_Impl *model)
		: QItemDelegate(parent), fModel(model) {}
	
	virtual void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QStyle *style = QApplication::style();
		DataSpecifier *spec = fModel->getDataSpecifier(index);
		QStyleOptionViewItem opt(option);
		
		if (spec->isCheckBox()) {
			opt.state &= ~QStyle::State_Active;
		}
		else if (spec->isComboBox()) {
			opt.state &= ~QStyle::State_Active;
		}
		
		if (spec->fFlags & SL_DATA_SPECIFIER_ELIDE_LEFT)
			opt.textElideMode = Qt::ElideLeft;
		else if (spec->fFlags & SL_DATA_SPECIFIER_ELIDE_MIDDLE)
			opt.textElideMode = Qt::ElideMiddle;
		else if (spec->fFlags & SL_DATA_SPECIFIER_ELIDE_RIGHT)
			opt.textElideMode = Qt::ElideRight;
		else
			opt.textElideMode = Qt::ElideNone;
		
		QRect rect = opt.rect;
		int margin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option);
		QTableView *popup = qobject_cast<QTableView *>(parent());
		
		if ((popup) && (popup->lineWidth()) && (index.column() == 0)) {
			opt.rect.setWidth(rect.width() - (margin * 2));
		}
		
		QItemDelegate::paint(painter, opt, index);
		
		if ((popup) && (popup->lineWidth()) && (index.column() > 0)) {
			painter->save();
			
			QPen pen(popup->palette().color(QPalette::WindowText).darker(120));
			QVector<qreal> dashes;
			dashes << 1 << 1;
			pen.setDashPattern(dashes);
			painter->setPen(pen);
			painter->setClipping(false);
			painter->drawLine(rect.x() - margin, rect.y(), rect.x() - margin, rect.y() + rect.height());
			
			painter->restore();
		}
	}
	
private:
	DataModel_Impl		*fModel;
};



class CompleterPopup : public QTableView
{
	Q_OBJECT
	
public:
	CompleterPopup()
		: QTableView(NULL), fLastCompleter(NULL)
	{
		moveToThread(QApplication::instance()->thread());
		int cellHeight = QFontMetrics(font()).height() + 2;
		horizontalHeader()->setStretchLastSection(true);
		horizontalHeader()->hide();
		verticalHeader()->setDefaultSectionSize(cellHeight);
		verticalHeader()->hide();
		setEditTriggers(QAbstractItemView::NoEditTriggers);
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setSelectionBehavior(QAbstractItemView::SelectRows);
		setSelectionMode(QAbstractItemView::SingleSelection);
		setShowGrid(false);
		setParent(NULL, Qt::Popup);
	// 	setFocusPolicy(Qt::NoFocus);
	}
	
	void popup(Completer *completer)
	{
		DataModel_Impl *model = completer->model();
		
		int cellHeight = QFontMetrics(font()).height() + 2;
		int rowCount = model->rowCount();
		int columnCount = model->columnCount();
		int i, margin;
		QSize size(0,0);
		QWidget *widget = completer->lineEdit();
		
		QPalette palette = style()->standardPalette();
		if (completer->bgcolor().isValid()) {
			palette.setColor(QPalette::Base, completer->bgcolor());
			setAutoFillBackground(false);
			setAttribute(Qt::WA_TranslucentBackground);
		}
		if (completer->hicolor().isValid())
			palette.setColor(QPalette::HighlightedText, completer->hicolor());
		if (completer->hibgcolor().isValid())
			palette.setColor(QPalette::Highlight, completer->hibgcolor());
		if (completer->color().isValid()) {
			palette.setColor(QPalette::WindowText, completer->color());
			palette.setColor(QPalette::Text, completer->color());
			setFrameStyle(QFrame::Panel | QFrame::Plain);
			setLineWidth(1);
			setMidLineWidth(0);
			size = QSize(2,2);
		}
		setPalette(palette);
		fBorderColor = completer->color();
		
		setItemDelegate(new CompleterItemDelegate(this, model));
		setModel(model);
		setFocusProxy(widget);
		
		for (i = 0; i < columnCount; i++) {
			int width = model->headerData(i, Qt::Horizontal, Qt::UserRole).toInt();
			size.rwidth() += width;
			horizontalHeader()->resizeSection(i, width);
		}
		
		margin = lineWidth();
		size.rwidth() += style()->pixelMetric(QStyle::PM_ScrollBarExtent);
		size.rheight() += (qMin(rowCount, 15) * cellHeight);
		size.rwidth() = qMax(size.width(), widget->width() - (margin * 2));
		setFixedSize(size);
		
		const QRect screen = QApplication::desktop()->availableGeometry(widget);
		QPoint pos = widget->mapToGlobal(QPoint(margin, widget->height() - 2 + margin));
		if (pos.y() + size.height() >= screen.bottom())
			pos.setY(pos.y() - widget->height() - size.height() + 2);
		if (pos.x() + size.width() >= screen.right())
			pos.setX(pos.x() + widget->width() - size.width());
		move(pos);
		
		fLastCompleter = completer;
		installEventFilter(completer);
		completer->lineEdit()->installEventFilter(completer);
		
		connect(this, SIGNAL(complete(bool)), completer, SLOT(completionActivated(bool)));
		connect(this, SIGNAL(clicked(QModelIndex)), completer, SLOT(completionActivated()));
		connect(this, SIGNAL(activated(QModelIndex)), completer, SLOT(completionActivated()));
		connect(selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), completer, SLOT(completionSelected(QItemSelection)));
		
		if ((!isVisible()) && (rowCount > 0) && (!isPopupMessageShown())) {
			connect(completer, SIGNAL(complete(int)), completer->lineEdit(), SLOT(handleComplete(int)));
			show();
		}
	}
	
	void popdown(bool cancel)
	{
		if (cancel)
			setCurrentIndex(QModelIndex());
		
		setFocusProxy(NULL);
		if (fLastCompleter) {
			fLastCompleter->lineEdit()->removeEventFilter(fLastCompleter);
			removeEventFilter(fLastCompleter);
			
			disconnect(this, NULL, fLastCompleter, NULL);
			disconnect(selectionModel(), NULL, fLastCompleter, NULL);
			disconnect(fLastCompleter, SIGNAL(complete(int)), NULL, NULL);
			
			fLastCompleter = NULL;
		}
		if (isVisible())
			hide();
	}
	
	bool handleEvent(QEvent *event, DataModel_Impl *model, int column, FormattedLineEdit *lineEdit)
	{
		switch (event->type()) {
		case QEvent::KeyPress:
			{
				QKeyEvent *e = (QKeyEvent *)event;
				QModelIndex currentIndex = this->currentIndex();
				QModelIndex index;
				int i = 0, count = model->rowCount();
				
				switch (e->key()) {
				case Qt::Key_End:
					{
						index = sPopup->moveCursor(QAbstractItemView::MoveEnd, e->modifiers());
						while ((index.isValid()) && (index.row() > 0) && (++i < count) && !(model->flags(index) & Qt::ItemIsSelectable)) {
							index = model->index(index.row() - 1, column);
						}
						sPopup->setCurrentIndex(index);
						return true;
					}
					break;
				
				case Qt::Key_Home:
					{
						index = sPopup->moveCursor(QAbstractItemView::MoveHome, e->modifiers());
						while ((index.isValid()) && (index.row() < count - 1) && (++i < count) && !(model->flags(index) & Qt::ItemIsSelectable)) {
							index = model->index(index.row() + 1, column);
						}
						sPopup->setCurrentIndex(index);
						return true;
					}
					break;
				
				case Qt::Key_Up:
					{
						if (!currentIndex.isValid()) {
							index = model->index(count - 1, column);
						}
						else if (currentIndex.row() > 0) {
							index = sPopup->moveCursor(QAbstractItemView::MoveUp, e->modifiers());
						}
						while ((index.isValid()) && (index.row() > 0) && (++i < count) && !(model->flags(index) & Qt::ItemIsSelectable)) {
							index = model->index(index.row() - 1, column);
						}
						sPopup->setCurrentIndex(index);
						return true;
					}
					break;
				
				case Qt::Key_Down:
					{
						if (!currentIndex.isValid()) {
							index = model->index(0, column);
						}
						else if (currentIndex.row() < count - 1) {
							index = sPopup->moveCursor(QAbstractItemView::MoveDown, e->modifiers());
						}
						while ((index.isValid()) && (index.row() < count - 1) && (++i < count) && !(model->flags(index) & Qt::ItemIsSelectable)) {
							index = model->index(index.row() + 1, column);
						}
						sPopup->setCurrentIndex(index);
						return true;
					}
					break;
				
				case Qt::Key_PageUp:
					{
						sPopup->setCurrentIndex(sPopup->moveCursor(QAbstractItemView::MovePageUp, e->modifiers()));
						return true;
					}
					break;
					
				case Qt::Key_PageDown:
					{
						sPopup->setCurrentIndex(sPopup->moveCursor(QAbstractItemView::MovePageDown, e->modifiers()));
						return true;
					}
					break;
				
				case Qt::Key_Return:
				case Qt::Key_Enter:
				case Qt::Key_Tab:
				case Qt::Key_Backtab:
					{
						bool editor = qvariant_cast<bool>(lineEdit->property("editor"));
						bool enter = (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter);
						if (currentIndex.isValid()) {
							emit complete((!editor) || (enter));
							if ((editor) || (enter))
								return true;
						}
						else if ((editor) && (enter)) {
							QObject *view = lineEdit->parent()->parent();
							SL_QAPP()->sendTabEvent(view);
							return true;
						}
					}
					break;
				
				case Qt::Key_Escape:
				case Qt::Key_Backspace:
				case Qt::Key_Delete:
					{
						sPopup->popdown(true);
					}
					break;
				}
				
				if ((!e->text().isEmpty()) || (!lineEdit->hasFocus()))
					sPopup->popdown(true);
				
				sEatFocus = true;
				QApplication::sendEvent(lineEdit, event);
				sEatFocus = false;
				
				return event->isAccepted();
			}
			break;
		
		case QEvent::MouseButtonPress:
			{
				if (!sPopup->underMouse()) {
					sPopup->popdown(true);
					return true;
				}
			}
			break;
		
		case QEvent::ShortcutOverride:
			{
				QKeyEvent *e = (QKeyEvent *)event;
				if (e->key() == Qt::Key_Escape)
					sPopup->popdown(true);
				QApplication::sendEvent(lineEdit, event);
			}
			break;
		
		default:
			break;
		}
		return false;
	}

signals:
	void complete(bool modified);
	
private:
	QColor			fBorderColor;
	Completer		*fLastCompleter;
	
	friend class Completer;
};



bool
Completer::isRunningOn(QWidget *widget)
{
// 	if (!sPopup)
// 		qDebug() << "Completer::isRunningOn NO POPUP";
// 	else if (!sPopup->isVisible())
// 		qDebug() << "Completer::isRunningOn NOT VISIBLE";
// 	else if (!sPopup->fLastCompleter)
// 		qDebug() << "Completer::isRunningOn NO LAST COMPLETER";
// 	else if (sPopup->fLastCompleter->lineEdit() != widget)
// 		qDebug() << "Completer::isRunningOn WIDGET DIFFERS" << sPopup->fLastCompleter->lineEdit() << widget;
// 	else
// 		qDebug() << "Completer::isRunningOn OK";
	return (sPopup) && (sPopup->isVisible()) && (sPopup->fLastCompleter) && (sPopup->fLastCompleter->lineEdit() == widget);
}


bool
Completer::underMouse()
{
	return (sPopup) && (sPopup->isVisible()) && (sPopup->underMouse());
}


void
Completer::hide()
{
	if (sPopup)
		sPopup->hide();
}


void
Completer::complete()
{
	if ((sPopup) && (sPopup->isVisible()) && (sPopup->fLastCompleter))
		sPopup->fLastCompleter->completionActivated();
}


int
Completer::completion()
{
	QModelIndex index;
	if ((sPopup) && (sPopup->isVisible()))
		index = sPopup->currentIndex();
	if (!index.isValid())
		return -1;
	return index.row();
}


bool
Completer::eatFocus()
{
	return sEatFocus;
}


Completer::Completer(FormattedLineEdit *parent, DataModel_Impl *model, int column, const QColor& color, const QColor& bgcolor, const QColor& hicolor, const QColor& hibgcolor)
	: QObject(parent), fLineEdit(parent), fModel(model), fColumn(column), fColor(color), fBGColor(bgcolor), fHIColor(hicolor), fHIBGColor(hibgcolor)
{
	moveToThread(QApplication::instance()->thread());
}


Completer::~Completer()
{
	if ((sPopup) && (sPopup->fLastCompleter == this))
		sPopup->popdown(false);
}


bool
Completer::eventFilter(QObject *obj, QEvent *event)
{
	if ((sEatFocus) && (obj == fLineEdit) && (event->type() == QEvent::FocusOut)) {
		if ((sPopup) && (sPopup->isVisible()))
			return true;
	}
	
	if (obj != sPopup)
		return QObject::eventFilter(obj, event);
	
	return sPopup->handleEvent(event, fModel, fColumn, fLineEdit);
}


void
Completer::completionSelected(const QItemSelection& selection)
{
	QModelIndex index;
	if (!selection.indexes().isEmpty()) {
		index = selection.indexes().first();
		index = index.sibling(index.row(), fColumn);
	}
	if (index.isValid()) {
		fLineEdit->setText(fModel->data(index, Qt::DisplayRole).toString());
		fLineEdit->selectAll();
	}
	else {
		fLineEdit->setText(fBaseString);
	}
}


void
Completer::completionActivated(bool modified)
{
	QModelIndex index = sPopup->currentIndex();
	
	if (fModel->flags(index) & Qt::ItemIsSelectable) {
		if (modified)
			emit complete(index.row());
		sPopup->popdown(false);
	}
	else {
		sPopup->popdown(true);
	}
}


void
Completer::handleComplete()
{
	if (!sPopup)
		sPopup = new CompleterPopup();
	
	fBaseString = fLineEdit->text();
	sPopup->popup(this);
}


#include "completer.moc"

