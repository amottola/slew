#include "slew.h"

#include "combobox.h"
#include "objects.h"

#include "constants/window.h"

#include <QKeyEvent>


ComboBox_Impl::ComboBox_Impl()
	: QComboBox(), WidgetInterface(), fEnterTabs(true), fReadOnly(false), fPopupShown(false)
{
	setFocusPolicy(Qt::StrongFocus);
	setInsertPolicy(QComboBox::NoInsert);
	setEditable(false);
	setDuplicatesEnabled(true);
	
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(handleCurrentIndexChanged(int)));
	
	installEventFilter(this);
}


bool
ComboBox_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Up:
			case Qt::Key_Down:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
			case Qt::Key_Home:
			case Qt::Key_End:
			case Qt::Key_Return:
			case Qt::Key_Enter:
			case Qt::Key_Space:
			case Qt::Key_Select:
			case Qt::Key_Back:
				return isEnabled() && (!fReadOnly);
			default:
				break;
			}
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
		return isEnabled() && (!fReadOnly);
	case QEvent::Wheel:
		return isEnabled() && hasFocus() && (!fReadOnly);
	default:
		break;
	}
	return false;
}


void
ComboBox_Impl::handleCurrentIndexChanged(int index)
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		runner.set("selection", index);
		runner.run();
	}
}


void
ComboBox_Impl::handleEditingFinished()
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", currentText());
		runner.run();
	}
}


bool
ComboBox_Impl::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *e = (QKeyEvent *)event;
		
		if ((isEnterTabs()) && ((e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter))) {
			SL_QAPP()->sendTabEvent(this);
			return true;
		}
	}
	else if ((event->type() == QEvent::Wheel) && (!hasFocus()))
		return true;
	
	if (fReadOnly) {
		fReadOnly = false;
		bool skip = isModifyEvent(event);
		fReadOnly = true;
		return skip;
	}
	
	return false;
}




void
ComboBox_Impl::showPopup()
{
	if ((count() > 0) && (!fPopupShown)) {
		if (!EventRunner(this, "onExpand").run())
			return;
	}
	fPopupShown = true;
	QComboBox::showPopup();
}


void
ComboBox_Impl::hidePopup()
{
	QComboBox::hidePopup();
	if (fPopupShown) {
		EventRunner(this, "onCollapse").run();
	}
	fPopupShown = false;
}


QSize
ComboBox_Impl::minimumSizeHint() const
{
	QSize minSize = QComboBox::minimumSizeHint();
	QSize size = qvariant_cast<QSize>(property("explicitSize"));
	if ((!size.isNull()) && (size.width() < minSize.width()))
		minSize.setWidth(size.width());
	return minSize;
}


SL_DEFINE_METHOD(ComboBox, set_model, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O!", PyDataModel_Type, &object))
		return NULL;
	
	DataModel_Impl *model = (DataModel_Impl *)getImpl(object);
	if (!model)
		SL_RETURN_NO_IMPL;
	
	impl->setModel(model);
})


SL_DEFINE_METHOD(ComboBox, open_popup, {
	impl->showPopup();
})


SL_DEFINE_METHOD(ComboBox, close_popup, {
	impl->hidePopup();
})


SL_DEFINE_METHOD(ComboBox, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isEnterTabs())
		style |= SL_COMBOBOX_STYLE_ENTERTABS;
	if (impl->isEditable())
		style |= SL_COMBOBOX_STYLE_EDITABLE;
	if (impl->isReadOnly())
		style |= SL_COMBOBOX_STYLE_READONLY;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(ComboBox, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style | SL_WINDOW_STYLE_NOFOCUS);
	
	impl->setEnterTabs(style & SL_COMBOBOX_STYLE_ENTERTABS ? true : false);
	impl->setEditable(style & SL_COMBOBOX_STYLE_EDITABLE ? true : false);
	impl->setReadOnly(style & SL_COMBOBOX_STYLE_READONLY ? true : false);
	if (impl->lineEdit())
		QObject::connect(impl->lineEdit(), SIGNAL(editingFinished()), impl, SLOT(handleEditingFinished()), Qt::UniqueConnection);
})


SL_DEFINE_METHOD(ComboBox, get_model_column, {
	return PyInt_FromLong(impl->modelColumn());
})


SL_DEFINE_METHOD(ComboBox, set_model_column, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	impl->setModelColumn(column);
})


SL_DEFINE_METHOD(ComboBox, get_selection, {
	return PyInt_FromLong(impl->currentIndex());
})


SL_DEFINE_METHOD(ComboBox, set_selection, {
	int index;
	
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	impl->blockSignals(true);
	impl->setCurrentIndex(index);
	impl->blockSignals(false);
})


SL_DEFINE_METHOD(ComboBox, get_value, {
	return createStringObject(impl->currentText());
})


SL_DEFINE_METHOD(ComboBox, set_value, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->blockSignals(true);
	int index = -1;
	for (int i = 0; i < impl->count(); i++) {
		if (text == impl->itemText(i)) {
			index = i;
			break;
		}
	}
	impl->setCurrentIndex(index);
	if (index < 0) {
		impl->setEditText(text);
	}
	impl->blockSignals(false);
})



SL_START_PROXY_DERIVED(ComboBox, Window)
SL_METHOD(set_model)
SL_METHOD(open_popup)
SL_METHOD(close_popup)

SL_PROPERTY(style)
SL_PROPERTY(model_column)
SL_PROPERTY(selection)
SL_PROPERTY(value)
SL_END_PROXY_DERIVED(ComboBox, Window)


#include "combobox.moc"
#include "combobox_h.moc"

