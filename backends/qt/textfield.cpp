#include "slew.h"
#include "objects.h"

#include "textfield.h"

#include <QKeyEvent>
#include <QClipboard>


TextField_Impl::TextField_Impl()
	: FormattedLineEdit(), WidgetInterface()
{
	connect(this, SIGNAL(textModified(const QString&, int)), this, SLOT(handleTextModified(const QString&, int)));
	connect(this, SIGNAL(returnPressed()), this, SLOT(handleReturnPressed()));
	connect(this, SIGNAL(iconClicked()), this, SLOT(handleIconClicked()));
	connect(this, SIGNAL(contextMenu()), this, SLOT(handleContextMenu()));
}


QSize
TextField_Impl::minimumSizeHint() const
{
	QSize size = FormattedLineEdit::minimumSizeHint();
	size.setWidth(qMax(size.width(), 50));
	return size;
}


bool
TextField_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if (((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter)) && (isEnterTabs()))
				return false;
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
TextField_Impl::isFocusOutEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Tab) || (e->key() == Qt::Key_Backtab))
				return true;
			if (((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter)) && (isEnterTabs()))
				return true;
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::TouchBegin:
	case QEvent::Wheel:
		{
			QWidget *w = QApplication::widgetAt(QCursor::pos());
			return (!w) || ((w != this) && (!isAncestorOf(w)));
		}
		break;
	default:
		break;
	}
	return false;
}


bool
TextField_Impl::canFocusOut(QWidget *oldFocus, QWidget *newFocus)
{
	if (!isValid()) {
		QApplication::beep();
		return false;
	}
	if (Completer::isRunningOn(this)) {
		Completer::complete();
		SL_QAPP()->sendTabEvent(this);
		return false;
	}
	return EventRunner(oldFocus, "onFocusOut").run() || (oldFocus->focusPolicy() == Qt::NoFocus);
}


void
TextField_Impl::handleTextModified(const QString& text, int completion)
{
	if ((completion >= 0) && (!canModify(this)))
		return;
	
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", text);
		runner.set("text", FormattedLineEdit::text());
		runner.set("completion", completion);
		runner.run();
	}
}


void
TextField_Impl::handleReturnPressed()
{
	EventRunner(this, "onEnter").run();
	
	if (isEnterTabs())
		QMetaObject::invokeMethod(qApp, "sendTabEvent", Qt::QueuedConnection, Q_ARG(QObject *, this));
}


void
TextField_Impl::handleIconClicked()
{
	EventRunner(this, "onClick").run();
}


void
TextField_Impl::handleContextMenu()
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


SL_DEFINE_METHOD(TextField, cut, {
	impl->cut();
})


SL_DEFINE_METHOD(TextField, copy, {
	impl->copy();
})


SL_DEFINE_METHOD(TextField, paste, {
	impl->paste();
})


SL_DEFINE_METHOD(TextField, delete, {
	impl->del();
})


SL_DEFINE_METHOD(TextField, insert, {
	int pos;
	QString text;
	
	if (!PyArg_ParseTuple(args, "iO&", &pos, convertString, &text))
		return NULL;
	
	impl->setCursorPosition(pos);
	impl->insert(text);
})


SL_DEFINE_METHOD(TextField, is_modified, {
	return createBoolObject(impl->isModified());
})


SL_DEFINE_METHOD(TextField, is_valid, {
	return createBoolObject(impl->state() == FormattedLineEdit::Acceptable);
})


SL_DEFINE_METHOD(TextField, undo, {
	impl->undo();
})


SL_DEFINE_METHOD(TextField, redo, {
	impl->redo();
})


SL_DEFINE_METHOD(TextField, is_undo_available, {
	return createBoolObject(impl->isUndoAvailable());
})


SL_DEFINE_METHOD(TextField, is_redo_available, {
	return createBoolObject(impl->isRedoAvailable());
})


SL_DEFINE_METHOD(TextField, set_completer, {
	DataModel_Impl *model;
	PyObject *object;
	int column;
	QColor color, bgcolor, hicolor, hibgcolor;
	
	if (!PyArg_ParseTuple(args, "OiO&O&O&O&", &object, &column, convertColor, &color, convertColor, &bgcolor, convertColor, &hicolor, convertColor, &hibgcolor))
		return NULL;
	
	if (object == Py_None) {
		model = NULL;
	}
	else {
		if (!PyObject_TypeCheck(object, (PyTypeObject *)PyDataModel_Type)) {
			PyErr_SetString(PyExc_TypeError, "expecting 'Completer' or None object");
			return NULL;
		}
		model = (DataModel_Impl *)getImpl(object);
		if (!model)
			SL_RETURN_NO_IMPL;
	}
	
	impl->setCompleter(model, column, color, bgcolor, hicolor, hibgcolor);
})


SL_DEFINE_METHOD(TextField, complete, {
	impl->complete();
})


SL_DEFINE_METHOD(TextField, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isReadOnly())
		style |= SL_TEXTFIELD_STYLE_READONLY;
	if (impl->echoMode() == QLineEdit::Password)
		style |= SL_TEXTFIELD_STYLE_PASSWORD;
	if (impl->isSelectedOnFocus())
		style |= SL_TEXTFIELD_STYLE_SELECT;
	if (impl->isCapsOnly())
		style |= SL_TEXTFIELD_STYLE_CAPS;
	if (impl->isEnterTabs())
		style |= SL_TEXTFIELD_STYLE_ENTERTABS;
	
	return PyInt_FromLong(style);
})


static void
_set_style(TextField_Impl *impl, int style)
{
	setWindowStyle(impl, style);
	
	impl->setReadOnly(style & SL_TEXTFIELD_STYLE_READONLY ? true : false);
	impl->setFocusPolicy(style & SL_TEXTFIELD_STYLE_READONLY ? Qt::NoFocus : Qt::StrongFocus);
#ifdef Q_OS_WIN
	impl->setFrame(style & SL_TEXTFIELD_STYLE_READONLY ? false : true);
#endif
	impl->setEchoMode(style & SL_TEXTFIELD_STYLE_PASSWORD ? QLineEdit::Password : QLineEdit::Normal);
	impl->setSelectedOnFocus(style & SL_TEXTFIELD_STYLE_SELECT ? true : false);
	impl->setCapsOnly(style & SL_TEXTFIELD_STYLE_CAPS ? true : false);
	if (style & SL_TEXTFIELD_STYLE_CAPS)
		impl->setInternalValue(impl->value().toUpper());
	impl->setEnterTabs(style & SL_TEXTFIELD_STYLE_ENTERTABS ? true : false);
	impl->setAttribute(Qt::WA_MacShowFocusRect, style & SL_WINDOW_STYLE_NOFOCUS ? false : true);
}


SL_DEFINE_METHOD(TextField, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	_set_style(impl, style);
})


SL_DEFINE_METHOD(TextField, get_datatype, {
	return PyInt_FromLong(impl->dataType());
})


SL_DEFINE_METHOD(TextField, set_datatype, {
	int datatype;
	
	if (!PyArg_ParseTuple(args, "i", &datatype))
		return NULL;
	
	impl->setDataType(datatype);
})


SL_DEFINE_METHOD(TextField, get_selection, {
	return PyTuple_Pack(2, PyInt_FromLong(impl->selectionStart()), PyInt_FromLong(impl->selectionStart() + impl->selectedText().length()));
})


SL_DEFINE_METHOD(TextField, set_selection, {
	int start, end;
	
	if (!PyArg_ParseTuple(args, "ii", &start, &end))
		return NULL;
	
	if ((start < 0) && (end < 0))
		impl->selectAll();
	else {
		if (start < 0)
			start = 0;
		if (end < 0)
			end = 32767;
		impl->setSelection(start, end - start);
	}
})


SL_DEFINE_METHOD(TextField, get_insertion_point, {
	return PyInt_FromLong(impl->cursorPosition());
})


SL_DEFINE_METHOD(TextField, set_insertion_point, {
	int pos;
	
	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	impl->setCursorPosition(pos);
})


SL_DEFINE_METHOD(TextField, get_value, {
	return createStringObject(impl->value());
})


SL_DEFINE_METHOD(TextField, set_value, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setText(text);
})


SL_DEFINE_METHOD(TextField, get_length, {
	return PyInt_FromLong(impl->maxLength());
})


SL_DEFINE_METHOD(TextField, set_length, {
	int length;
	
	if (!PyArg_ParseTuple(args, "i", &length))
		return NULL;
	
	impl->setMaxLength(length > 0 ? length : 32767);
})


SL_DEFINE_METHOD(TextField, get_align, {
	return PyInt_FromLong(toAlign(impl->alignment()));
})


SL_DEFINE_METHOD(TextField, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "i", &align))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align) & Qt::AlignHorizontal_Mask;
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignLeft;
	impl->setAlignment(alignment | Qt::AlignVCenter);
})


SL_DEFINE_METHOD(TextField, get_filter, {
	return createStringObject(impl->internalValidator()->regExp().pattern());
})


SL_DEFINE_METHOD(TextField, set_filter, {
	QString filter;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &filter))
		return NULL;
	
	if (filter.isEmpty())
		filter = ".*";
	impl->internalValidator()->setRegExp(QRegExp(filter));
})


SL_DEFINE_METHOD(TextField, get_format, {
	return createStringObject(impl->format());
})


SL_DEFINE_METHOD(TextField, set_format, {
	QString format;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &format))
		return NULL;
	
	impl->setFormat(format);
})


SL_DEFINE_METHOD(TextField, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_METHOD(TextField, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
})


SL_DEFINE_METHOD(TextField, get_empty_text, {
	return createStringObject(impl->emptyText());
})


SL_DEFINE_METHOD(TextField, set_empty_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setEmptyText(text);
})



SL_START_PROXY_DERIVED(TextField, Window)
SL_METHOD(cut)
SL_METHOD(copy)
SL_METHOD(paste)
SL_METHOD(delete)
SL_METHOD(insert)
SL_METHOD(is_modified)
SL_METHOD(is_valid)
SL_METHOD(undo)
SL_METHOD(redo)
SL_METHOD(is_undo_available)
SL_METHOD(is_redo_available)
SL_METHOD(set_completer)
SL_METHOD(complete)

SL_PROPERTY(style)
SL_PROPERTY(datatype)
SL_PROPERTY(selection)
SL_PROPERTY(insertion_point)
SL_PROPERTY(value)
SL_PROPERTY(length)
SL_PROPERTY(align)
SL_PROPERTY(filter)
SL_PROPERTY(format)
SL_PROPERTY(icon)
SL_PROPERTY(empty_text)
SL_END_PROXY_DERIVED(TextField, Window)


#include "textfield.moc"
#include "textfield_h.moc"

