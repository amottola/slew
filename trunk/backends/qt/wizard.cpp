#include "slew.h"

#include "wizard.h"
#include "wizardpage.h"

#include <QWizard>
#include <QAbstractButton>


Wizard_Impl::Wizard_Impl()
	: QWizard(), WidgetInterface()
{
	setAttribute(Qt::WA_QuitOnClose);
#ifdef Q_WS_WIN
	setWizardStyle(ModernStyle);
#endif
}


SL_DEFINE_METHOD(Wizard, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	if (!isWizardPage(object))
		SL_RETURN_CANNOT_ATTACH;
	
	WizardPage_Impl *child = (WizardPage_Impl *)getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	impl->setPage(index, child);
})


SL_DEFINE_METHOD(Wizard, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (!isWizardPage(object))
		SL_RETURN_CANNOT_DETACH;
	
	WizardPage_Impl *child = (WizardPage_Impl *)getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	foreach (int id, impl->pageIds()) {
		if (impl->page(id) == child) {
			impl->removePage(id);
			break;
		}
	}
})


SL_DEFINE_METHOD(Wizard, show_modal, {
	bool blocking;
	int modality;
	Qt::WindowModality qmodality;
	
	if (!PyArg_ParseTuple(args, "O&i", convertBool, &blocking, &modality))
		return NULL;
	
	switch (modality) {
	case SL_DIALOG_MODALITY_WINDOW:
		{
			qmodality = Qt::WindowModal;
		}
		break;
	case SL_DIALOG_MODALITY_APPLICATION:
		{
			qmodality = Qt::ApplicationModal;
		}
		break;
	default:
		{
			if (impl->parentWidget())
				qmodality = Qt::WindowModal;
			else
				qmodality = Qt::ApplicationModal;
		}
		break;
	}
	impl->setWindowModality(qmodality);
	
	if (blocking) {
		Py_BEGIN_ALLOW_THREADS
		
		impl->exec();
		
		Py_END_ALLOW_THREADS
	}
	else {
		impl->open();
	}
	
	if (impl->result() == QDialog::Accepted)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
})


SL_DEFINE_METHOD(Wizard, end_modal, {
	bool result;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &result))
		return NULL;
	
	if (result)
		impl->accept();
	else
		impl->reject();
})


SL_DEFINE_METHOD(Wizard, get_page, {
	return PyInt_FromLong(impl->currentId());
})


SL_DEFINE_METHOD(Wizard, next, {
	impl->next();
})


SL_DEFINE_METHOD(Wizard, back, {
	impl->back();
})


SL_DEFINE_METHOD(Wizard, restart, {
	impl->restart();
})


SL_DEFINE_METHOD(Wizard, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (!(impl->options() & QWizard::NoCancelButton))
		style |= SL_WIZARD_STYLE_CANCEL;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Wizard, set_style, {
	int style;
	QWizard::WizardOptions opts = impl->options();
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	if (style & SL_WIZARD_STYLE_CANCEL)
		opts &= ~QWizard::NoCancelButton;
	else
		opts |= QWizard::NoCancelButton;
	
	impl->setOptions(opts);
})


SL_DEFINE_METHOD(Wizard, get_start_page, {
	return PyInt_FromLong(impl->startId());
})


SL_DEFINE_METHOD(Wizard, set_start_page, {
	int page;
	
	if (!PyArg_ParseTuple(args, "i", &page))
		return NULL;
	
	impl->setStartId(page);
})


SL_DEFINE_METHOD(Wizard, is_back_enabled, {
	QAbstractButton *button = impl->button(QWizard::BackButton);
	return createBoolObject(button->isEnabled());
})


SL_DEFINE_METHOD(Wizard, set_back_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	QAbstractButton *button = impl->button(QWizard::BackButton);
	button->setEnabled(enabled);
})


SL_DEFINE_METHOD(Wizard, is_next_enabled, {
	QAbstractButton *button = impl->button(QWizard::NextButton);
	return createBoolObject(button->isEnabled());
})


SL_DEFINE_METHOD(Wizard, set_next_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	QAbstractButton *button = impl->button(QWizard::NextButton);
	button->setEnabled(enabled);
})


SL_DEFINE_METHOD(Wizard, set_watermark, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::WatermarkPixmap, bitmap);
})


SL_DEFINE_METHOD(Wizard, set_logo, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::LogoPixmap, bitmap);
})


SL_DEFINE_METHOD(Wizard, set_banner, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::BannerPixmap, bitmap);
})


SL_DEFINE_METHOD(Wizard, set_background, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::BackgroundPixmap, bitmap);
})


SL_DEFINE_METHOD(Wizard, set_back_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setButtonText(QWizard::BackButton, text);
})


SL_DEFINE_METHOD(Wizard, set_next_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setButtonText(QWizard::NextButton, text);
})


SL_DEFINE_METHOD(Wizard, set_cancel_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setButtonText(QWizard::CancelButton, text);
})


SL_DEFINE_METHOD(Wizard, set_commit_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setButtonText(QWizard::CommitButton, text);
})


SL_DEFINE_METHOD(Wizard, set_finish_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setButtonText(QWizard::FinishButton, text);
})



SL_START_PROXY_DERIVED(Wizard, Dialog)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(show_modal)
SL_METHOD(end_modal)

SL_METHOD(get_page)
SL_METHOD(next)
SL_METHOD(back)
SL_METHOD(restart)
SL_METHOD(set_watermark)
SL_METHOD(set_logo)
SL_METHOD(set_banner)
SL_METHOD(set_background)
SL_METHOD(set_back_text)
SL_METHOD(set_next_text)
SL_METHOD(set_cancel_text)
SL_METHOD(set_commit_text)
SL_METHOD(set_finish_text)

SL_PROPERTY(style)
SL_PROPERTY(start_page)
SL_BOOL_PROPERTY(back_enabled)
SL_BOOL_PROPERTY(next_enabled)
SL_END_PROXY_DERIVED(Wizard, Dialog)


#include "wizard.moc"
#include "wizard_h.moc"

