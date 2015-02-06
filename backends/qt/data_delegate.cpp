#include "slew.h"
#include "objects.h"

#include <QResizeEvent>
#include <QToolButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyleOptionComboBox>
#include <QPointer>
#include <QKeyEvent>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextFormat>
#include <QTextObject>
#include <QVector>
#include <QMenu>
#include <QHeaderView>


class Base_Editor
{
public:
	Base_Editor(QWidget *parent, const QModelIndex& index)
		: fView((QAbstractItemView *)parent->parent()), fIndex(index) {}
	
protected:
	QAbstractItemView			*fView;
	QPersistentModelIndex		fIndex;
};



class Custom_Editor : public QWidget, public Base_Editor
{
	Q_OBJECT
	
public:
	Custom_Editor(QWidget *parent, const QModelIndex& index, PyObject *object)
		: QWidget(parent), Base_Editor(parent, index), fObject(object)
	{
		PyAutoLocker locker;
		Py_INCREF(object);
		fWidget = (QWidget *)getImpl(object);
		fWidget->setParent(this);
		setAutoFillBackground(true);
		setFocusProxy(fWidget);
		setFocusPolicy(fWidget->focusPolicy());
	}
	
	virtual ~Custom_Editor()
	{
		PyAutoLocker locker;
		fWidget->setParent(NULL);
		Py_DECREF(fObject);
	}
	
	virtual void resizeEvent(QResizeEvent *event)
	{
		fWidget->setGeometry(QRect(QPoint(0, 0), event->size()));
	}

public slots:
	void handleStartEditingEvent(QEvent *event)
	{
	}
	
private:
	PyObject	*fObject;
	QWidget		*fWidget;
};



class LineEdit_Editor : public FormattedLineEdit, public Base_Editor
{
	Q_OBJECT
	
public:
	LineEdit_Editor(QWidget *parent, const QModelIndex& index)
		: FormattedLineEdit(parent), Base_Editor(parent, index), fCurrentCompletion(-1)
	{
		connect(this, SIGNAL(textModified(const QString&, int)), this, SLOT(handleTextModified(const QString&, int)));
		connect(this, SIGNAL(iconClicked()), this, SLOT(handleStartEditingEvent()));
		connect(this, SIGNAL(contextMenu()), this, SLOT(handleContextMenu()));
		setProperty("editor", true);
	}
	
	int currentCompletion()
	{
		if (!Completer::isRunningOn(this))
			return fCurrentCompletion;
		return Completer::completion();
	}
	
	bool isModifyEvent(QEvent *event)
	{
		if (event->type() == QEvent::KeyPress) {
			QString text, oldText = QLineEdit::text();
			return (isValidInput((QKeyEvent *)event, &text)) && (oldText != text);
		}
		return false;
	}
	
	virtual bool canModify(QWidget *widget)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		EventRunner runner(fView, "onModify");
		if (!runner.isValid())
			return true;
		runner.set("index", model->getDataIndex(fIndex), false);
		return runner.run();
	}
	
	virtual bool modifyClipboardOnPaste(QString& text)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		EventRunner runner(fView, "onPaste");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(fIndex), false);
			runner.set("value", text);
			if (runner.run())
				return runner.get("value", &text);
		}
		return false;
	}

public slots:
	void handleStartEditingEvent(QEvent *event = NULL)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		DataSpecifier *spec = model->getDataSpecifier(fIndex);
		
		fCurrentCompletion = -1;
		
		if (event) {
			if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonRelease)) {
				QMouseEvent *e = (QMouseEvent *)event;
				if (e->button() == Qt::RightButton) {
					handleContextMenu();
					return;
				}
			}
			else if (event->type() == QEvent::ContextMenu) {
				handleContextMenu();
				return;
			}
		}
		
		if (((!event) || (event->type() == QEvent::MouseButtonRelease)) && (spec) && (spec->isClickableIcon())) {
			EventRunner runner(fView, "onClick");
			if (runner.isValid()) {
				runner.set("index", model->getDataIndex(fIndex), false);
				runner.run();
			}
		}
	}
	
	void handleTextModified(const QString& text, int completion)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		
		fCurrentCompletion = completion;
		
		EventRunner runner(fView, "onChange");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(fIndex), false);
			runner.set("value", text);
			runner.set("text", FormattedLineEdit::text());
			runner.set("completion", completion);
			runner.run();
			
			runner.setName("onCellDataChanged");
			runner.run();
		}
	}

	void handleContextMenu()
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		QPoint pos = fView->viewport()->mapFromGlobal(QCursor::pos());
		
		EventRunner runner(this, "onContextMenu");
		if (runner.isValid()) {
			runner.set("header", createVectorObject(QPoint(-1, -1)));
			runner.set("pos", createVectorObject(pos));
			runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
			runner.set("index", model->getDataIndex(fIndex), false);
			if (runner.run()) {
				QMenu *menu = createContextMenu();
				menu->setAttribute(Qt::WA_DeleteOnClose);
				menu->popup(QCursor::pos());
			}
		}
	}
	
private:
	int			fCurrentCompletion;
};



class CheckBox_Editor : public QCheckBox, public Base_Editor
{
	Q_OBJECT
	
public:
	CheckBox_Editor(QWidget *parent, const QModelIndex& index)
		: QCheckBox(parent), Base_Editor(parent, index)
	{
		setFocusProxy(parent);
		connect(this, SIGNAL(toggled(bool)), this, SLOT(handleToggled(bool)));
	}
	
	bool isModifyEvent(QEvent *event)
	{
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Space) || (e->key() == Qt::Key_Select))
				return true;
		}
		else if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick)) {
			QMouseEvent *e = (QMouseEvent *)event;
			return hitButton(mapFromGlobal(e->globalPos()));
		}
		return false;
	}
	
public slots:
	void handleStartEditingEvent(QEvent *event)
	{
		if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick))
			toggle();
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Space) || (e->key() == Qt::Key_Select))
				toggle();
		}
	}
	
	void handleToggled(bool checked)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		
		EventRunner runner(fView, "onChange");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(fIndex), false);
			runner.set("value", checked);
			runner.run();
			
			runner.setName("onCellDataChanged");
			runner.run();
		}
	}
};



class ComboBox_Editor : public QComboBox, public Base_Editor
{
	Q_OBJECT
	
public:
	ComboBox_Editor(QWidget *parent, const QModelIndex& index)
		: QComboBox(parent), Base_Editor(parent, index)
	{
		setEditable(false);
		setDuplicatesEnabled(true);
		connect(this, SIGNAL(activated(int)), this, SLOT(handleActivated(int)));
	}
	
	bool isModifyEvent(QEvent *event)
	{
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Up) ||
				(e->key() == Qt::Key_Down) ||
				(e->key() == Qt::Key_PageUp) ||
				(e->key() == Qt::Key_PageDown) ||
				(e->key() == Qt::Key_Home) ||
				(e->key() == Qt::Key_End))
				return true;
		}
		else if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick)) {
			return true;
		}
		return false;
	}
	
public slots:
	void handleStartEditingEvent(QEvent *event)
	{
		showPopup();
	}
	
	void handleActivated(int index)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		
		EventRunner runner(fView, "onChange");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(fIndex), false);
			runner.set("value", index);
			runner.run();
			
			runner.setName("onCellDataChanged");
			runner.run();
		}
	}
};



class SpinBox_Editor : public QSpinBox, public Base_Editor
{
	Q_OBJECT
	
public:
	SpinBox_Editor(QWidget *parent, const QModelIndex& index)
		: QSpinBox(parent), Base_Editor(parent, index)
	{
		setRange(0, 100);
		connect(this, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
	}
	
	bool isModifyEvent(QEvent *event)
	{
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Up) ||
				(e->key() == Qt::Key_Down) ||
				(e->key() == Qt::Key_PageUp) ||
				(e->key() == Qt::Key_PageDown) ||
				(e->key() == Qt::Key_Home) ||
				(e->key() == Qt::Key_End) ||
				(e->key() == Qt::Key_Backspace) ||
				(e->key() == Qt::Key_Delete) ||
				(!e->text().isEmpty()))
				return true;
		}
		else if ((event->type() == QEvent::MouseButtonPress) || (event->type() == QEvent::MouseButtonRelease) || (event->type() == QEvent::MouseButtonDblClick)) {
			return true;
		}
		return false;
	}
	
public slots:
	void handleStartEditingEvent(QEvent *event)
	{
	}
	
	void handleValueChanged(int value)
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		
		EventRunner runner(fView, "onChange");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(fIndex), false);
			runner.set("value", value);
			runner.run();
			
			runner.setName("onCellDataChanged");
			runner.run();
		}
	}
};



class Browser_Editor : public QWidget, public Base_Editor
{
	Q_OBJECT
	
public:
	Browser_Editor(QWidget *parent, const QModelIndex& index)
		: QWidget(parent), Base_Editor(parent, index)
	{
		fButton = new QToolButton(this);
		fButton->setText("...");
		fButton->setCheckable(true);
		fButton->setFocusPolicy(Qt::NoFocus);
		setFocusPolicy(Qt::StrongFocus);
		connect(fButton, SIGNAL(clicked()), this, SLOT(handleClicked()));
	}
	
	bool isModifyEvent(QEvent *event)
	{
		if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent *e = (QMouseEvent *)event;
			return fButton->geometry().contains(fButton->mapFromGlobal(e->globalPos()));
		}
		return false;
	}
	
	void resizeEvent(QResizeEvent *event)
	{
		QSize size = event->size();
		fButton->setGeometry(QRect(size.width() - size.height(), 0, size.height(), size.height()));
	}
	
public slots:
	void handleStartEditingEvent(QEvent *event)
	{
		fButton->click();
	}
	
	void handleClicked()
	{
		DataModel_Impl *model = (DataModel_Impl *)fView->model();
		PyObject *dataIndex = model->getDataIndex(fIndex);
		QPointer<QToolButton> button = qobject_cast<QToolButton *>(sender());
		
		EventRunner runner(fView, "onClick");
		if (runner.isValid()) {
			runner.set("index", dataIndex, false);
			runner.set("browsed_data", Py_None, false);
			if (runner.run()) {
				PyObject *data;
				if (!runner.get("browsed_data", &data))
					data = Py_None;
				Py_INCREF(data);
				
				EventRunner runner2(fView, "onChange");
				if (runner2.isValid()) {
					runner2.set("index", dataIndex, false);
					runner2.set("value", data, false);
					runner2.run();
					
					runner2.setName("onCellDataChanged");
					runner2.run();
				}
				QVariant value;
				value.setValue(data);
				model->setData(fIndex, value, Qt::UserRole + 1);
				Py_DECREF(data);
			}
			if (button)
				button->setChecked(false);
		}
	}
	
private:
	QToolButton		*fButton;
};



ItemDelegate::ItemDelegate(QObject *parent)
	: QItemDelegate(parent), fCurrentSpec(NULL), fTabEvent(NULL)
{
	fInvalidPattern = QPixmap(32,32);
	fInvalidPattern.fill(Qt::transparent);
	QPainter painter(&fInvalidPattern);
	painter.fillRect(fInvalidPattern.rect(), QBrush(Qt::gray, Qt::Dense5Pattern));
}


void
ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	ItemDelegate *delegate = (ItemDelegate *)this;
	QAbstractItemView *view = (QAbstractItemView *)parent();
	QStyle *style = QApplication::style();
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	DataSpecifier *spec = model->getDataSpecifier(index);
#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
	QStyleOptionViewItem opt, backOpt;
	opt = backOpt = setOptions(option);
#else
	QStyleOptionViewItem opt(option), backOpt(option);
#endif
	
	delegate->fCurrentSpec = spec;
	
	painter->save();
	painter->setClipping(false);
	
	backOpt.showDecorationSelected = true;
	
	preparePaint(&opt, &backOpt, index);
	
	drawBackground(painter, backOpt, index);
	painter->setClipRect(option.rect);
	
	if ((!spec) || (spec->isNone())) {
		painter->fillRect(opt.rect, QBrush(fInvalidPattern));
	}
	else if (spec->isCheckBox()) {
		QStyleOptionViewItem o(opt);
		
		o.state = opt.state | (spec->fSelection < 0 ? QStyle::State_NoChange : (spec->fSelection > 0 ? QStyle::State_On : QStyle::State_Off));
		QString text = spec->fText;
		if (text.isEmpty()) {
			o.rect = QStyle::alignedRect(opt.direction, spec->fAlignment, style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt).size(), opt.rect);
			if (spec->isReadOnly())
				o.state &= ~QStyle::State_Enabled;
			
			style->drawPrimitive(QStyle::PE_IndicatorViewItemCheck, &o, painter, NULL);
		}
		else {
			if (spec->fFont != painter->font())
				painter->setFont(spec->fFont);
			
			QSize checkSize = style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &opt).size();
			QRect checkRect = QStyle::alignedRect(opt.direction, spec->fAlignment, checkSize, QRect(opt.rect.left(), opt.rect.top(), checkSize.width(), opt.rect.height()));
			QRect textRect = opt.rect.adjusted(checkSize.width() + QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin), 0, 0, 0);
			
			Qt::CheckState state;
			if (spec->fSelection == 0)
				state = Qt::Unchecked;
			else if (spec->fSelection > 0)
				state = Qt::Checked;
			else
				state = Qt::PartiallyChecked;
			drawCheck(painter, o, checkRect, state);
			drawDisplay(painter, o, textRect, text);
		}
	}
	else if (spec->isComboBox()) {
		QStyleOptionComboBox o;
		DataModel_Impl *comboModel = (DataModel_Impl *)getImpl(spec->fModel);
		
		o.QStyleOption::operator=(opt);
		if (spec->isReadOnly())
			o.state &= ~QStyle::State_Enabled;
		if (!comboModel) {
			o.currentText = spec->fChoices.value(spec->fSelection);
		}
		else {
			QModelIndex index = comboModel->index(spec->fSelection);
			o.currentText = comboModel->data(index, Qt::DisplayRole).value<QString>();
			o.currentIcon = comboModel->data(index, Qt::DecorationRole).value<QIcon>();
			if (o.currentIcon.availableSizes().count() > 0)
				o.iconSize = o.currentIcon.availableSizes().first();
		}
		
		style->drawComplexControl(QStyle::CC_ComboBox, &o, painter, NULL);
#ifdef Q_OS_WIN32
		// Workaround for Windows as label needs a real QComboBox to exist to properly adjust margins; we do it manually here...
// 		o.rect.adjust(3, 3, -19, -3);
#elif defined(Q_OS_MAC)
		if (!o.currentIcon.isNull())
			o.rect.adjust(1, 0, 0, 0);
#endif
		style->drawControl(QStyle::CE_ComboBoxLabel, &o, painter, NULL);
	}
	else if (spec->fWidget) {
		QWidget *widget = (QWidget *)getImpl(spec->fWidget);
		widget->setEnabled(opt.state & QStyle::State_Enabled ? true : false);
		widget->render(painter, opt.rect.topLeft());
	}
	else {
		if ((spec->isBrowser()) && (view->currentIndex() == index)) {
			opt.rect.setWidth(opt.rect.width() - opt.rect.height());
		}
		if (!spec->fIcon.isNull()) {
			opt.decorationSize = spec->fIcon.availableSizes().value(0);
		}
		painter->setClipRect(opt.rect);
		opt.decorationAlignment = spec->fIconAlignment;
		QItemDelegate::paint(painter, opt, index);
	}
	
	finishPaint(painter, opt, index);
	
	PyAutoLocker locker;
	EventRunner runner(view, "onPaintItem");
	if (runner.isValid()) {
		runner.set("dc", createDCObject(painter));
		runner.set("index", model->getDataIndex(index), false);
		runner.run();
	}
	
	painter->restore();
}


void
ItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const
{
	QStyle *style = QApplication::style();
	QPen pen = painter->pen();
	QColor color = fCurrentSpec->fColor;
	Qt::Alignment alignment;
	
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
		cg = QPalette::Inactive;
	
	if (option.state & QStyle::State_Selected) {
		color = option.palette.color(cg, QPalette::HighlightedText);
		painter->fillRect(rect, option.palette.brush(cg, QPalette::Highlight));
	}
	else if (!color.isValid())
		color = option.palette.color(cg, QPalette::WindowText);
	
	QString value = getFormattedValue(text, &color, &alignment, fCurrentSpec->fDataType, fCurrentSpec->fFormatInfo);
	
	if ((alignment & Qt::AlignHorizontal_Mask) == 0)
		alignment |= (fCurrentSpec->fAlignment & Qt::AlignHorizontal_Mask);
	if ((alignment & Qt::AlignVertical_Mask) == 0)
		alignment |= (fCurrentSpec->fAlignment & Qt::AlignVertical_Mask);
	
	QRect textRect = rect;
#ifdef Q_OS_MAC
	textRect.adjust(2, 0, -2, 0);
#else
	textRect.adjust(3, 0, -3, 0);
#endif
	
	if ((fCurrentSpec->isClickableIcon()) && (!fCurrentSpec->fIcon.isNull())) {
		QPixmap pixmap;
		QStyleOptionFrameV2 panel;
		panel.rect = rect;
		QRect iconRect = style->subElementRect(QStyle::SE_LineEditContents, &panel, (QWidget *)parent());
		iconRect.adjust(iconRect.width() - iconRect.height(), 0, 0, 0);
		
		textRect.adjust(0, 0, -(iconRect.width() + (style->pixelMetric(QStyle::PM_FocusFrameHMargin, &option) + 1) * 2), 0);
		
		QIcon::Mode mode = option.state & QStyle::State_Enabled ? (option.state & QStyle::State_Selected ? QIcon::Selected : QIcon::Normal) : QIcon::Disabled;
		QSize size = fCurrentSpec->fIcon.actualSize(iconRect.size(), mode);
		pixmap = fCurrentSpec->fIcon.pixmap(size, mode);
		
		if (iconRect.width() > pixmap.width())
			iconRect.moveLeft(iconRect.left() + ((iconRect.width() - pixmap.width()) / 2));
		if (iconRect.height() > pixmap.height())
			iconRect.moveTop(iconRect.top() + ((iconRect.height() - pixmap.height()) / 2));
		painter->drawPixmap(iconRect.topLeft(), pixmap);
	}
	
	pen.setColor(color);
	if (pen != painter->pen())
		painter->setPen(pen);
	if (fCurrentSpec->fFont != painter->font())
		painter->setFont(fCurrentSpec->fFont);
	
	if (fCurrentSpec->isHTML()) {
		QTextDocument doc;
		doc.setHtml(value);
		doc.setDocumentMargin(0);
		if (option.state & QStyle::State_Selected) {
			for (QTextBlock block = doc.firstBlock(); block.isValid(); block = block.next()) {
				QTextCursor cursor(block);
				QTextCharFormat format;
				format.setForeground(QBrush(color));
				cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
				cursor.mergeCharFormat(format);
			}
		}
		doc.setTextWidth(textRect.width());
		painter->save();
		painter->translate(textRect.topLeft());
		int height = doc.size().height();
		painter->translate(0, (textRect.height() - height) / 2);
		textRect.moveTo(0, 0);
		doc.drawContents(painter, textRect);
		painter->restore();
	}
	else {
		Qt::TextElideMode mode = Qt::ElideNone;
		if (fCurrentSpec->fFlags & SL_DATA_SPECIFIER_ELIDE_LEFT)
			mode = Qt::ElideLeft;
		else if (fCurrentSpec->fFlags & SL_DATA_SPECIFIER_ELIDE_MIDDLE)
			mode = Qt::ElideMiddle;
		else if (fCurrentSpec->fFlags & SL_DATA_SPECIFIER_ELIDE_RIGHT)
			mode = Qt::ElideRight;
		painter->drawText(textRect, alignment, painter->fontMetrics().elidedText(value, mode, textRect.width()));
	}
}


QWidget *
ItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	PyAutoLocker locker;
	DataModel_Impl *model = (DataModel_Impl *)index.model();
	DataSpecifier *spec = model->getDataSpecifier(index);
	QPointer<QWidget> editor;
	
	if ((!spec) || (spec->isNone()))
		return QItemDelegate::createEditor(parent, option, index);
	
	if (spec->isCustom()) {
		editor = new Custom_Editor(parent, index, spec->fWidget);
	}
	else if (spec->isCheckBox()) {
		editor = new CheckBox_Editor(parent, index);
	}
	else if (spec->isComboBox()) {
		editor = new ComboBox_Editor(parent, index);
	}
	else if (spec->isBrowser()) {
		editor = new Browser_Editor(parent, index);
	}
	else if (spec->isSpinField()) {
		editor = new SpinBox_Editor(parent, index);
	}
	else {
		editor = new LineEdit_Editor(parent, index);
	}
	
	return editor;
}


bool
ItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *abstractModel, const QStyleOptionViewItem& option, const QModelIndex& index)
{
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	DataModel_Impl *model = (DataModel_Impl *)abstractModel;
	bool modified = false, edited = true;
	QEvent *editingEvent = NULL;
	QEvent::Type type = event->type();
	
	if ((type == QEvent::MouseButtonPress) || (type == QEvent::MouseButtonRelease) || (type == QEvent::MouseButtonDblClick)) {
		QMouseEvent *e = (QMouseEvent *)event;
		
		PyAutoLocker locker;
		DataSpecifier *spec = model->getDataSpecifier(index);
		QStyle *style = view->style();
		
		if ((!spec) || (spec->isCustom()))
			return true;
		
		editingEvent = new QMouseEvent(type, e->pos(), e->globalPos(), e->button(), e->buttons(), e->modifiers());
		
		if (spec->isCheckBox()) {
			QStyleOptionButton o;
			o.QStyleOption::operator=(option);
			Qt::Alignment alignment;
			if (spec->fText.isEmpty())
				alignment = spec->fAlignment;
			else
				alignment = Qt::AlignLeft | Qt::AlignVCenter;
			QRect rect = QStyle::alignedRect(o.direction, alignment, style->subElementRect(QStyle::SE_ViewItemCheckIndicator, &o).size(), o.rect);
			if (rect.contains(e->pos())) {
				modified = true;
			}
			else {
				edited = false;
			}
		}
		else if (spec->isComboBox()) {
			modified = true;
		}
		else if ((spec->isClickableIcon()) && (spec->isEnabled()) && (!spec->fIcon.isNull())) {
			QStyleOptionFrameV2 o;
			o.QStyleOption::operator=(option);
			QRect rect = option.rect;
			rect.adjust(rect.width() - rect.height() + 1, 1, -1, -1);
			QSize size = spec->fIcon.actualSize(rect.size());
			if (rect.width() > size.width())
				rect.setLeft(rect.left() + ((rect.width() - size.width()) / 2));
			if (rect.height() > size.height())
				rect.setTop(rect.top() + ((rect.height() - size.height()) / 2));
			if (!rect.contains(e->pos())) {
				edited = false;
			}
		}
		else if (spec->isBrowser()) {
			QRect rect = option.rect;
			rect.adjust(rect.width() - rect.height(), 0, 0, 0);
			if (rect.contains(e->pos())) {
				modified = true;
			}
			else {
				edited = false;
			}
		}
// 		if (spec->isText()) {
// 			if (e->button() == Qt::RightButton)
// 				modified = true;
// 		}
	}
	else if (type == QEvent::KeyPress) {
		QKeyEvent *e = (QKeyEvent *)event;
		editingEvent = new QKeyEvent(type, e->key(), e->modifiers(), e->text(), e->isAutoRepeat(), e->count());
		
		switch (e->key()) {
		case Qt::Key_Up:
		case Qt::Key_Down:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_Escape:
		case Qt::Key_Home:
		case Qt::Key_End:
			{
				PyAutoLocker locker;
				DataSpecifier *spec = model->getDataSpecifier(index);
				if (((spec->isComboBox()) || (spec->isSpinField())) && (spec->isEnabled()) && (spec->isSelectable()) && (!spec->isReadOnly()))
					break;
				modified = true;
			}
			break;
		
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Enter:
		case Qt::Key_Return:
			break;
		
		default:
			modified = true;
			break;
		}
	}
	
	bool result = true;
	if ((modified) && (index.flags() & Qt::ItemIsEditable)) {
		EventRunner runner(view, "onModify");
		if (runner.isValid()) {
			runner.set("index", model->getDataIndex(index), false);
			result = runner.run();
		}
	}
	if ((result) && (edited)) {
		QWidget *editor = view->indexWidget(index);
		if ((editor) && (editingEvent))
			QMetaObject::invokeMethod(editor, "handleStartEditingEvent", Qt::DirectConnection, Q_ARG(QEvent *, editingEvent));
	}
	delete editingEvent;
	return !result;
}


bool
ItemDelegate::eventFilter(QObject *editor, QEvent *event)
{
	if (editor) {
		QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
		
		switch (int(event->type())) {
		case QEvent::KeyPress:
			{
				QKeyEvent *e = (QKeyEvent *)event;
				if ((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter)) {
					if (view->editTriggers() == QAbstractItemView::AllEditTriggers) {
						event->ignore();
						QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress, e->modifiers() & Qt::ShiftModifier ? Qt::Key_Backtab : Qt::Key_Tab, e->modifiers() & ~Qt::ShiftModifier);
						QApplication::postEvent(editor, keyEvent);
						return true;
					}
				}
			}
			break;
		
		case QEvent::FocusOut:
			{
				QFocusEvent *e = (QFocusEvent *)event;
				if ((e->reason() != Qt::PopupFocusReason) && (e->reason() != Qt::ActiveWindowFocusReason))
					return QItemDelegate::eventFilter(editor, event);
			}
			break;

		case QEvent::ShortcutOverride:
			{
				return QItemDelegate::eventFilter(editor, event);
			}
			break;
		}
		
		bool modified = false;
		
		LineEdit_Editor *lineEdit = qobject_cast<LineEdit_Editor *>(editor);
		if (lineEdit)
			modified = lineEdit->isModifyEvent(event);
		
		CheckBox_Editor *checkBox = qobject_cast<CheckBox_Editor *>(editor);
		if (checkBox)
			modified = checkBox->isModifyEvent(event);
		
		ComboBox_Editor *comboBox = qobject_cast<ComboBox_Editor *>(editor);
		if (comboBox)
			modified = comboBox->isModifyEvent(event);
		
		SpinBox_Editor *spinBox = qobject_cast<SpinBox_Editor *>(editor);
		if (spinBox)
			modified = spinBox->isModifyEvent(event);
		
		Browser_Editor *browser = qobject_cast<Browser_Editor *>(editor);
		if (browser)
			modified = browser->isModifyEvent(event);
		
		if (modified) {
			DataModel_Impl *model = (DataModel_Impl *)view->model();
			EventRunner runner(view, "onModify");
			if (runner.isValid()) {
				runner.set("index", model->getDataIndex(view->currentIndex()), false);
				return !runner.run();
			}
		}
	}
	
	return false;
}


void
ItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QStyleOptionViewItem o(option);
	if (qobject_cast<CheckBox_Editor *>(editor)) {
		PyAutoLocker locker;
		DataModel_Impl *model = (DataModel_Impl *)index.model();
		DataSpecifier *spec = model->getDataSpecifier(index);
		o.rect = QStyle::alignedRect(o.direction, (Qt::Alignment)spec->fAlignment, QApplication::style()->subElementRect(QStyle::SE_ViewItemCheckIndicator, &o).size(), o.rect);
	}
	QItemDelegate::updateEditorGeometry(editor, o, index);
}


QSize
ItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	QSize size = QItemDelegate::sizeHint(option, index);
	PyAutoLocker locker;
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	DataSpecifier *spec = model->getDataSpecifier(index);

	if ((spec) && (!spec->isNone())) {
		int margin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin, &option);
		if ((spec->isClickableIcon()) && (!spec->fIcon.isNull())) {
			size.rwidth() += size.height() + margin + 1;
		}
		if (spec->isHTML()) {
			QTextDocument doc;
			doc.setHtml(spec->fText);
			doc.setDocumentMargin(0);
			doc.setTextWidth(view->visualRect(index).width() - margin);
			size = size.expandedTo(doc.size().toSize());
		}
	}
	return size;
}


void
ItemDelegate::startEditing(const QModelIndex& index)
{
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	EventRunner runner(view, "onCellEditStart");
	if (runner.isValid()) {
		DataSpecifier *spec = model->getDataSpecifier(index);
		PyObject *value = NULL;
		
		if (!spec)
			return;
		
		if (spec->isText()) {
			value = createStringObject(spec->fText);
		}
		else if (spec->isCheckBox()) {
			if (spec->fSelection >= 0)
				value = createBoolObject(spec->fSelection != 0);
			else {
				value = Py_None;
				Py_INCREF(value);
			}
		}
		else if (spec->isComboBox()) {
			value = PyInt_FromLong(spec->fSelection);
		}
		else if (spec->isSpinField()) {
			value = PyInt_FromLong(spec->fText.toLong());
		}
		else {
			value = Py_None;
			Py_INCREF(value);
		}
		runner.set("value", value);
		runner.set("text", spec->fText);
		runner.set("index", model->getDataIndex(index), false);
		if (runner.run()) {
			QWidget *editor = view->indexWidget(index);
			if (editor) {
				setEditorData(editor, index);
			}
		}
	}
}


void
ItemDelegate::setEditorData(QWidget *editor, const QModelIndex& index) const
{
	PyAutoLocker locker;
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	DataSpecifier *spec = model->getDataSpecifier(index);
	
	if ((!spec) || (spec->isNone()) || (spec->isCustom()))
		return;
	
	if (spec->isCheckBox()) {
		QCheckBox *checkBox = qobject_cast<QCheckBox *>(editor);
		if (checkBox) {
			checkBox->blockSignals(true);
			if (spec->fSelection < 0)
				checkBox->setCheckState(Qt::PartiallyChecked);
			else
				checkBox->setChecked(spec->fSelection != 0);
			checkBox->setEnabled(!spec->isReadOnly());
			checkBox->blockSignals(false);
		}
	}
	else if (spec->isComboBox()) {
		QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
		if (comboBox) {
			DataModel_Impl *comboModel = (DataModel_Impl *)getImpl(spec->fModel);
			
			if (comboModel) {
				comboBox->setModel(comboModel);
			}
			else {
				comboBox->clear();
				foreach (QString choice, spec->fChoices) {
					if (choice.isEmpty())
						comboBox->insertSeparator(comboBox->count());
					else
						comboBox->addItem(choice);
				}
			}
			comboBox->setCurrentIndex(spec->fSelection);
			comboBox->setEnabled(!spec->isReadOnly());
		}
	}
	else if (spec->isSpinField()) {
		QSpinBox *spinBox = qobject_cast<QSpinBox *>(editor);
		if (spinBox) {
			spinBox->setRange(0, spec->fLength);
			spinBox->setValue(spec->fText.toLong());
			spinBox->setEnabled(!spec->isReadOnly());
		}
	}
	else if (spec->isText()) {
		FormattedLineEdit *lineEdit = qobject_cast<FormattedLineEdit *>(editor);
		if (lineEdit) {
			int pos = lineEdit->cursorPosition();
			lineEdit->setDataType(spec->fDataType);
			lineEdit->setAlignment(spec->fAlignment);
			lineEdit->setMaxLength(spec->fLength ? spec->fLength : 32767);
			lineEdit->setFormat(spec->fFormat);
			lineEdit->setCapsOnly(spec->isCapsOnly());
			lineEdit->setSelectedOnFocus(spec->isSelectedOnFocus());
			lineEdit->setText(spec->fText);
			lineEdit->setCursorPosition(pos);
			lineEdit->internalValidator()->setRegExp(QRegExp(spec->fFilter));
			if ((!spec->fIcon.isNull()) && (spec->isClickableIcon()))
				lineEdit->setIcon(spec->fIcon);
			lineEdit->setCompleter((DataModel_Impl *)getImpl(spec->fCompleter.fModel), spec->fCompleter.fColumn, spec->fCompleter.fColor, spec->fCompleter.fBGColor, spec->fCompleter.fHIColor, spec->fCompleter.fHIBGColor);
			if ((spec->isSelectedOnFocus()) && (editor->hasFocus()))
				lineEdit->selectAll();
		}
	}
}


void
ItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *abstractModel, const QModelIndex& index) const
{
	PyAutoLocker locker;
	DataModel_Impl *model = (DataModel_Impl *)abstractModel;
	DataSpecifier *spec = model->getDataSpecifier(index);
	
	if ((!spec) || (spec->isNone()) || (spec->isCustom()))
		return;
	
	if (spec->isCheckBox()) {
		QCheckBox *checkBox = qobject_cast<QCheckBox *>(editor);
		if (checkBox) {
			bool checked = checkBox->isChecked();
			bool undefined = (checkBox->checkState() == Qt::PartiallyChecked);
			model->setData(index, undefined ? -1 : (checked ? 1 : 0), Qt::UserRole);
		}
	}
	else if (spec->isComboBox()) {
		QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
		if (comboBox) {
			int selection = comboBox->currentIndex();
			model->setData(index, selection, Qt::UserRole);
		}
	}
	else if (spec->isSpinField()) {
		QSpinBox *spinBox = qobject_cast<QSpinBox *>(editor);
		if (spinBox) {
			QString value = QString::number(spinBox->value());
			model->setData(index, value, Qt::EditRole);
		}
	}
	else if (spec->isText()) {
		LineEdit_Editor *lineEdit = qobject_cast<LineEdit_Editor *>(editor);
		if (lineEdit) {
			QString value = lineEdit->value();
			model->setData(index, value, Qt::EditRole);
		}
	}
}


bool
ItemDelegate::isFocusOutEvent(QEvent *event)
{
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	QWidget *editor = view->indexWidget(view->currentIndex());
	fTabEvent = NULL;
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
				if (Completer::isRunningOn(editor))
					return false;
				if ((qobject_cast<ComboBox_Editor *>(editor)) || (qobject_cast<SpinBox_Editor *>(editor)))
					return false;
				return true;
			case Qt::Key_Home:
			case Qt::Key_End:
#ifdef Q_OS_MAC
				if (qobject_cast<ComboBox_Editor *>(editor))
					return false;
				return true;
#else
				return false;
#endif
#ifdef Q_OS_WIN
			case Qt::Key_Up:
			case Qt::Key_Down:
				if ((qobject_cast<ComboBox_Editor *>(editor)) || (qobject_cast<SpinBox_Editor *>(editor)) || (Completer::isRunningOn(editor)))
					return false;
				return true;
#endif
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				if (e->count() == 0)
					return false;
				if ((Completer::isRunningOn(editor)) && (Completer::completion() >= 0)) {
					fTabEvent = new QKeyEvent(QEvent::KeyPress, e->key(), e->modifiers(), e->text(), false, 0);
				}
				return true;
			default:
				break;
			}
			return false;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::TouchBegin:
		{
			if ((Completer::isRunningOn(editor)) && (Completer::underMouse()))
				return false;
			QModelIndex index = view->indexAt(view->viewport()->mapFromGlobal(QCursor::pos()));
			if (index == view->currentIndex())
				return false;
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}


bool
ItemDelegate::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	QPointer<QAbstractItemView> view(qobject_cast<QAbstractItemView *>(oldFocus));
	QPointer<QWidget> _newFocus(newFocus);
	DataModel_Impl *model = (DataModel_Impl *)view->model();
	QModelIndex index = view->currentIndex();
	QWidget *editor = view->indexWidget(index);
	QCheckBox *checkBox = qobject_cast<QCheckBox *>(editor);
	QComboBox *comboBox = qobject_cast<QComboBox *>(editor);
	QSpinBox *spinBox = qobject_cast<QSpinBox *>(editor);
	LineEdit_Editor *lineEdit = qobject_cast<LineEdit_Editor *>(editor);
	int completion = -1;
	
	if ((!editor) || (!index.isValid()) || (!(model->flags(index) & Qt::ItemIsEditable))) {
		delete fTabEvent;
		fTabEvent = NULL;
		return true;
	}
	
	if (lineEdit) {
		completion = lineEdit->currentCompletion();
		if (Completer::isRunningOn(lineEdit)) {
			lineEdit->handleTextModified(lineEdit->value(), completion);
		}
		if (!lineEdit->isValid()) {
			QApplication::beep();
			delete fTabEvent;
			fTabEvent = NULL;
			return false;
		}
	}
	
	EventRunner runner(view, "onCellEditEnd");
	if (!runner.isValid()) {
		delete fTabEvent;
		fTabEvent = NULL;
		return true;
	}
	if (checkBox) {
		runner.set("value", checkBox->isChecked());
	}
	else if (comboBox) {
		runner.set("value", comboBox->currentIndex());
	}
	else if (spinBox) {
		runner.set("value", spinBox->value());
	}
	else if (lineEdit) {
		runner.set("value", lineEdit->value());
	}
	runner.set("completion", completion);
	runner.set("index", model->getDataIndex(index), false);
	bool success = runner.run();
	
	if ((success) && (view) && (_newFocus) && (!view->isAncestorOf(_newFocus)))
		success = EventRunner(view, "onFocusOut").run();
	
	if ((success) && (fTabEvent) && (view))
		QApplication::postEvent(view, fTabEvent);
	
	return success;
}


bool
ItemDelegate::isEditValid()
{
	QAbstractItemView *view = qobject_cast<QAbstractItemView *>(QObject::parent());
	QWidget *editor = view->indexWidget(view->currentIndex());
	if (editor) {
		LineEdit_Editor *lineEdit = qobject_cast<LineEdit_Editor *>(editor);
		if (lineEdit) {
			return lineEdit->isValid();
		}
	}
	return true;
}


#include "data_delegate.moc"
