#include "slew.h"
#include "objects.h"

#include "searchfield.h"

#include <QKeyEvent>
#include <QClipboard>
#include <QToolButton>



class SearchIcon : public QToolButton
{
	Q_OBJECT
	
public:
	SearchIcon(FormattedLineEdit *parent, const QIcon& icon, QMenu *menu)
		: QToolButton(parent)
	{
		setFocusPolicy(Qt::NoFocus);
		setIcon(icon);
		setMenu(menu);
		setPopupMode(InstantPopup);
		setCursor(Qt::PointingHandCursor);
		connect(this, SIGNAL(clicked()), parent, SIGNAL(iconClicked()));
	}
	
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		QIcon::Mode mode;
		
		if (!isEnabled())
			mode = QIcon::Disabled;
		else if ((isDown()) || (isChecked()))
			mode = QIcon::Selected;
		else
			mode = QIcon::Normal;
		
		QPixmap pixmap = icon().pixmap(size(), mode, QIcon::Off);
		painter.drawPixmap((size().width() - pixmap.size().width()) / 2, (size().height() - pixmap.size().height()) / 2, pixmap);
	}
};



class CancelButton : public QToolButton
{
	Q_OBJECT
	
public:
	CancelButton(FormattedLineEdit *parent, const QIcon& icon)
		: QToolButton(parent)
	{
		const unsigned char kData[] = {
			0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10,
			0x00, 0x00, 0x00, 0x10, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0xf3, 0xff, 0x61, 0x00, 0x00, 0x00, 0x19, 0x74, 0x45, 0x58,
			0x74, 0x53, 0x6f, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x41, 0x64, 0x6f, 0x62, 0x65, 0x20, 0x49, 0x6d, 0x61, 0x67,
			0x65, 0x52, 0x65, 0x61, 0x64, 0x79, 0x71, 0xc9, 0x65, 0x3c, 0x00, 0x00, 0x01, 0xc0, 0x49, 0x44, 0x41, 0x54, 0x78, 0xda,
			0x8c, 0x53, 0xcf, 0x4b, 0x02, 0x41, 0x14, 0xde, 0x99, 0x5d, 0x7f, 0x65, 0xae, 0xb7, 0x94, 0xed, 0xe0, 0x22, 0x84, 0x10,
			0x98, 0x37, 0xcf, 0xe2, 0x25, 0x8f, 0x7b, 0xe9, 0xd4, 0xb5, 0x3f, 0xac, 0x4b, 0x75, 0x88, 0x60, 0x6f, 0x7a, 0x12, 0x8f,
			0xe2, 0xad, 0x8c, 0x82, 0xa0, 0x48, 0x90, 0x16, 0xbb, 0x2d, 0x52, 0xf9, 0x63, 0x73, 0x7b, 0xdf, 0xb2, 0x23, 0xd3, 0x94,
			0xd4, 0x03, 0x71, 0xe6, 0xed, 0xfb, 0xbe, 0xf7, 0xe6, 0x9b, 0x6f, 0x58, 0x18, 0x86, 0x9a, 0x1a, 0xb3, 0xd9, 0x6c, 0xdb,
			0xf3, 0xbc, 0xbd, 0xe9, 0x74, 0x5a, 0xc5, 0x3e, 0x97, 0xcb, 0x0d, 0x2d, 0xcb, 0x7a, 0x48, 0xa5, 0x52, 0x6f, 0x6a, 0xad,
			0x21, 0x6f, 0xe6, 0xf3, 0xf9, 0x56, 0xbf, 0xdf, 0xbf, 0x1a, 0x8f, 0xc7, 0x87, 0x8c, 0x31, 0x6d, 0xb9, 0x5c, 0x32, 0xe4,
			0x13, 0x89, 0x44, 0xb8, 0x5a, 0xad, 0x58, 0xa9, 0x54, 0x72, 0xeb, 0xf5, 0xfa, 0x31, 0x11, 0xbd, 0x0b, 0x0c, 0x13, 0x13,
			0xf8, 0xbe, 0xbf, 0xd3, 0x6e, 0xb7, 0x3d, 0x02, 0x71, 0x2a, 0xd6, 0x7e, 0x0b, 0xce, 0xb9, 0x96, 0x4c, 0x26, 0x83, 0x56,
			0xab, 0xb5, 0x9b, 0xcf, 0xe7, 0x5f, 0xa3, 0x9c, 0xe8, 0x0c, 0x30, 0xfd, 0x6f, 0x04, 0x23, 0xf0, 0x8d, 0x8e, 0x67, 0x74,
			0x3a, 0x9d, 0x17, 0x60, 0xd6, 0x04, 0x18, 0x1b, 0x9d, 0xb1, 0x2e, 0x14, 0x0a, 0x4f, 0xf4, 0x7b, 0x56, 0xc1, 0x71, 0xfe,
			0x09, 0xeb, 0xc5, 0x62, 0xa1, 0x0f, 0x06, 0x83, 0xd3, 0x88, 0x00, 0x82, 0xe1, 0xcc, 0x72, 0xe7, 0x46, 0xa3, 0x51, 0x95,
			0x49, 0x00, 0xa4, 0x5c, 0x4d, 0x34, 0x44, 0xed, 0x68, 0x34, 0x3a, 0xa2, 0x29, 0xb2, 0x1c, 0x6a, 0x43, 0x30, 0x11, 0x93,
			0xc9, 0xa4, 0xdc, 0xeb, 0xf5, 0xae, 0x63, 0x92, 0x68, 0x1a, 0x5a, 0x1f, 0x20, 0x47, 0xdf, 0x6c, 0x49, 0x8f, 0x10, 0x58,
			0x03, 0x57, 0x25, 0xd4, 0x56, 0x48, 0x86, 0xcd, 0x66, 0x73, 0x1f, 0xfb, 0x6e, 0xb7, 0x7b, 0x8b, 0x9c, 0x5c, 0x13, 0x04,
			0x01, 0x23, 0x6c, 0x85, 0x6b, 0x9b, 0x03, 0x67, 0x0a, 0xb5, 0x3f, 0x82, 0x93, 0x49, 0x6e, 0x70, 0xcf, 0x8a, 0x60, 0x18,
			0xbb, 0x46, 0x9d, 0xef, 0xd1, 0x1d, 0x6b, 0x21, 0xe0, 0xda, 0x40, 0x86, 0x11, 0x9a, 0xa6, 0x79, 0xc7, 0xe1, 0x30, 0x98,
			0x44, 0x11, 0xac, 0x2a, 0xce, 0x2c, 0x8e, 0xa3, 0x92, 0x00, 0x53, 0x2c, 0x16, 0x1f, 0x39, 0x5c, 0x45, 0x0e, 0xbb, 0x84,
			0x49, 0x44, 0xc4, 0xe0, 0xb2, 0xa4, 0x89, 0x8d, 0x9c, 0xb8, 0x05, 0x5d, 0xd7, 0x35, 0xdb, 0xb6, 0xcf, 0x81, 0x8d, 0x9c,
			0x08, 0x53, 0xb8, 0xae, 0xeb, 0xc3, 0x24, 0xda, 0x3f, 0x22, 0x93, 0xc9, 0x04, 0x8e, 0xe3, 0x98, 0xe4, 0xca, 0x8f, 0x88,
			0x11, 0x4c, 0xb0, 0x67, 0x3a, 0x9d, 0xfe, 0x94, 0x27, 0x51, 0x03, 0x9d, 0x01, 0xa6, 0x5a, 0x0b, 0xe0, 0x6f, 0x6f, 0x41,
			0x58, 0x9a, 0x1c, 0x76, 0x46, 0x26, 0x71, 0x70, 0xcf, 0xb8, 0x2a, 0x21, 0x18, 0xcc, 0x43, 0x63, 0x5f, 0xd0, 0x63, 0x3a,
			0x11, 0xe0, 0x1f, 0x04, 0x12, 0x51, 0x36, 0x7e, 0xce, 0x15, 0xec, 0xa1, 0x36, 0x04, 0x93, 0x5f, 0xa1, 0x88, 0x2f, 0x01,
			0x06, 0x00, 0x0b, 0xf8, 0x15, 0x9e, 0xd0, 0x0b, 0x0e, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
			0x60, 0x82
		};
		
		if (icon.isNull()) {
			QPixmap pixmap;
			pixmap.loadFromData(kData, sizeof(kData));
			setIcon(QIcon(pixmap));
		}
		else {
			setIcon(icon);
		}
		setFocusPolicy(Qt::NoFocus);
		connect(this, SIGNAL(clicked()), parent, SLOT(handleCancelClicked()));
	}
	
	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter painter(this);
		QIcon::Mode mode;
		
		if (!isEnabled())
			mode = QIcon::Disabled;
		else if ((isDown()) || (isChecked()))
			mode = QIcon::Selected;
		else
			mode = QIcon::Normal;
		
		QPixmap pixmap = icon().pixmap(size(), mode, QIcon::Off);
		painter.drawPixmap((size().width() - pixmap.size().width()) / 2, (size().height() - pixmap.size().height()) / 2, pixmap);
	}
};


SearchField_Impl::SearchField_Impl()
	: FormattedLineEdit(), WidgetInterface(), fCancelButton(NULL), fMenu(NULL)
{
	setEmptyText("Search\u2026");
	connect(this, SIGNAL(textModified(const QString&, int)), this, SLOT(handleTextModified(const QString&, int)));
	connect(this, SIGNAL(returnPressed()), this, SLOT(handleReturnPressed()));
	connect(this, SIGNAL(iconClicked()), this, SLOT(handleSearchClicked()));
	connect(this, SIGNAL(contextMenu()), this, SLOT(handleContextMenu()));
}


QAbstractButton *
SearchField_Impl::createIconButton(const QIcon& icon)
{
	return new SearchIcon(this, icon, fMenu);
}


void
SearchField_Impl::setCancelIcon(const QIcon& icon)
{
	fCancelIcon = icon;
	if (fCancelButton) {
		delete fCancelButton;
		fCancelButton = new CancelButton(this, fCancelIcon);
		updateGeometries();
	}
}


void
SearchField_Impl::setMenu(QMenu *menu)
{
	SearchIcon *button = (SearchIcon *)fIcon;
	if (button) {
		button->setMenu(menu);
		button->setPopupMode(QToolButton::InstantPopup);
	}
	fMenu = menu;
}


void
SearchField_Impl::setCancellable(bool enabled)
{
	if (((enabled) && (fCancelButton)) ||
		((!enabled) && (!fCancelButton)))
		return;
	
	delete fCancelButton;
	fCancelButton = NULL;
	
	if (enabled) {
		fCancelButton = new CancelButton(this, fCancelIcon);
		updateGeometries();
	}
}


void
SearchField_Impl::resizeEvent(QResizeEvent *event)
{
	QLineEdit::resizeEvent(event);
	updateGeometries();
}


void
SearchField_Impl::updateGeometries()
{
	int leftMargin = 0, rightMargin = 0;
	
	if (fIcon) {
		QStyleOptionFrameV2 panel;
		initStyleOption(&panel);
		QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
		leftMargin = rect.height() + style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, this) + 1;
		rect.adjust(0, 0, -(rect.width() - leftMargin), 0);
		fIcon->setGeometry(rect);
	}

	if (fCancelButton) {
		if (text().isEmpty()) {
			fCancelButton->hide();
		}
		else {
			QStyleOptionFrameV2 panel;
			initStyleOption(&panel);
			QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
			rightMargin = rect.height() + style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, this) + 1;
			rect.adjust(rect.width() - rect.height(), 0, 0, 0);
			fCancelButton->setGeometry(rect);
			fCancelButton->show();
		}
	}
	setTextMargins(leftMargin, 0, rightMargin, 0);
}


QSize
SearchField_Impl::sizeHint() const
{
	QSize size = qvariant_cast<QSize>(property("explicitSize"));
	
	ensurePolished();
	QFontMetrics fm(font());
	QMargins margins = textMargins();
	int h = qMax(fm.height(), 14) + 2 + margins.top() + margins.bottom();
	int w = fm.width(QLatin1Char('x')) * 17 + 4;
	QStyleOptionFrameV2 opt;
	initStyleOption(&opt);
	QSize tsize = (style()->sizeFromContents(QStyle::CT_LineEdit, &opt, QSize(w, h).expandedTo(QApplication::globalStrut()), this));
	
	if (size.width() <= 0)
		size.rwidth() = tsize.width();
	if (size.height() <= 0)
		size.rheight() = tsize.height();
	return size;
}


QSize
SearchField_Impl::minimumSizeHint() const
{
	QSize size = FormattedLineEdit::minimumSizeHint();
	size.setWidth(qMax(size.width(), 50 + (fIcon ? fIcon->icon().availableSizes().value(0).width() + 8 : 0)));
	return size;
}


bool
SearchField_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if (((e == QKeySequence::Undo) && (isUndoAvailable())) ||
				((e == QKeySequence::Redo) && (isRedoAvailable())))
				return true;
			QString text, oldText = QLineEdit::text();
			return (isValidInput(e, &text)) && (oldText != text);
		}
		break;
	default:
		break;
	}
	return false;
}


bool
SearchField_Impl::isFocusOutEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Tab) || (e->key() == Qt::Key_Backtab))
				return true;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::TouchBegin:
	case QEvent::Wheel:
		return true;
	default:
		break;
	}
	return false;
}


bool
SearchField_Impl::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	if (!isValid()) {
		QApplication::beep();
		return false;
	}
	if (newFocus == oldFocus)
		return true;
	return EventRunner(oldFocus, "onFocusOut").run() || (oldFocus->focusPolicy() == Qt::NoFocus);
}


void
SearchField_Impl::handleTextModified(const QString& text, int completion)
{
	if ((completion >= 0) && (!canModify(this)))
		return;
	
	updateGeometries();
	
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", text);
		runner.set("text", FormattedLineEdit::text());
		runner.set("completion", completion);
		runner.run();
	}
}


void
SearchField_Impl::handleReturnPressed()
{
	EventRunner(this, "onEnter").run();
}


void
SearchField_Impl::handleSearchClicked()
{
	SearchIcon *button = (SearchIcon *)fIcon;
	if (!button->menu())
		EventRunner(this, "onClick").run();
}


void
SearchField_Impl::handleCancelClicked()
{
	EventRunner(this, "onCancel").run();
	setText("");
	updateGeometries();
}


void
SearchField_Impl::handleContextMenu()
{
	bool showDefault = true;
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("pos", QCursor::pos());
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		showDefault = !runner.run();
	}
	if (showDefault) {
		QMenu *menu = createContextMenu();
		menu->setAttribute(Qt::WA_DeleteOnClose);
		menu->popup(QCursor::pos());
	}
}


SL_DEFINE_METHOD(SearchField, get_menu, {
	QMenu *menu = impl->menu();
	if (!menu)
		Py_RETURN_NONE;
	
	PyObject *object = getObject(menu);
	if (!object) {
		PyErr_Clear();
		Py_RETURN_NONE;
	}
	return object;
})


SL_DEFINE_METHOD(SearchField, set_menu, {
	PyObject *object;
	QMenu *menu;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		impl->setMenu(NULL);
	}
	else {
		menu = (QMenu *)getImpl(object);
		if (!menu)
			return NULL;
		impl->setMenu(menu);
	}
})


SL_DEFINE_METHOD(SearchField, get_cancel_icon, {
	return createIconObject(impl->cancelIcon());
})


SL_DEFINE_METHOD(SearchField, set_cancel_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setCancelIcon(icon);
})


SL_DEFINE_METHOD(SearchField, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isCancellable())
		style |= SL_SEARCHFIELD_STYLE_CANCEL;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(SearchField, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setCancellable(style & SL_SEARCHFIELD_STYLE_CANCEL ? true : false);
})



SL_START_PROXY_DERIVED(SearchField, TextField)
SL_METHOD(get_menu)
SL_METHOD(set_menu)
SL_METHOD(get_cancel_icon)
SL_METHOD(set_cancel_icon)

SL_PROPERTY(style)
SL_END_PROXY_DERIVED(SearchField, TextField)


#include "searchfield.moc"
#include "searchfield_h.moc"

