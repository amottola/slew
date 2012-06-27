#include "slew.h"

#include "wizard.h"
#include "wizardpage.h"

#include <QWizardPage>


WizardPage_Impl::WizardPage_Impl()
	: QWizardPage(), WidgetInterface(), fIsComplete(true), fHasNextID(false), fNextID(-1)
{
}


void
WizardPage_Impl::initializePage()
{
	EventRunner(this, "onActivate").run();
}


bool
WizardPage_Impl::validatePage()
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", wizard()->currentId());
		return runner.run();
	}
	return false;
}


int
WizardPage_Impl::nextId() const
{
	if (fHasNextID)
		return fNextID;
	return QWizardPage::nextId();
}


SL_DEFINE_METHOD(WizardPage, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isCommitPage())
		style |= SL_WIZARDPAGE_STYLE_COMMIT;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(WizardPage, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setCommitPage(style & SL_WIZARDPAGE_STYLE_COMMIT ? true : false);
})


SL_DEFINE_METHOD(WizardPage, get_title, {
	return createStringObject(impl->title());
})


SL_DEFINE_METHOD(WizardPage, set_title, {
	QString title;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &title))
		return NULL;
	
	impl->setTitle(title);
})


SL_DEFINE_METHOD(WizardPage, get_subtitle, {
	return createStringObject(impl->subTitle());
})


SL_DEFINE_METHOD(WizardPage, set_subtitle, {
	QString title;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &title))
		return NULL;
	
	impl->setSubTitle(title);
})


SL_DEFINE_METHOD(WizardPage, get_next_page, {
	return PyInt_FromLong(impl->nextId());
})


SL_DEFINE_METHOD(WizardPage, set_next_page, {
	int page;
	
	if (!PyArg_ParseTuple(args, "i", &page))
		return NULL;
	
	impl->setNextId(page);
})


SL_DEFINE_METHOD(WizardPage, set_complete, {
	bool complete;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &complete))
		return NULL;
	
	impl->setComplete(complete);
})


SL_DEFINE_METHOD(WizardPage, set_watermark, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::WatermarkPixmap, bitmap);
})


SL_DEFINE_METHOD(WizardPage, set_logo, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::LogoPixmap, bitmap);
})


SL_DEFINE_METHOD(WizardPage, set_banner, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::BannerPixmap, bitmap);
})


SL_DEFINE_METHOD(WizardPage, set_background, {
	QPixmap bitmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &bitmap))
		return NULL;
	
	impl->setPixmap(QWizard::BackgroundPixmap, bitmap);
})


SL_START_PROXY_DERIVED(WizardPage, Window)
SL_PROPERTY(style)
SL_PROPERTY(title)
SL_PROPERTY(subtitle)
SL_PROPERTY(next_page)

SL_METHOD(set_complete)
SL_METHOD(set_watermark)
SL_METHOD(set_logo)
SL_METHOD(set_banner)
SL_METHOD(set_background)
SL_END_PROXY_DERIVED(WizardPage, Window)


#include "wizardpage.moc"
#include "wizardpage_h.moc"

