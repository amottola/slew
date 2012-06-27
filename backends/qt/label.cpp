#include "slew.h"

#include "label.h"

#include "constants/widget.h"

#include <QStaticText>
#include <QTextOption>


Label_Impl::Label_Impl()
	: QLabel(), WidgetInterface()
{
	fStaticText.setTextFormat(Qt::PlainText);
	fStaticText.setTextWidth(0);
	setTextFormat(Qt::PlainText);
	setTextInteractionFlags(Qt::NoTextInteraction);
	setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
	connect(this, SIGNAL(linkActivated(const QString&)), this, SLOT(handleLinkActivated(const QString&)));
}


void
Label_Impl::paintEvent(QPaintEvent *event)
{
	if ((textFormat() != Qt::PlainText) || (textInteractionFlags() != Qt::NoTextInteraction)) {
		QLabel::paintEvent(event);
	}
	else {
		QPainter painter(this);
		QString text = this->text();
		int width = this->width();
		Qt::Alignment alignment = this->alignment();
		
		painter.setPen(palette().color(isEnabled() ? QPalette::Normal : QPalette::Disabled, QPalette::WindowText));
		painter.setFont(font());
		
		if (((width != fStaticText.textWidth()) || (text != fStaticText.text())) && (!wordWrap())) {
			text = painter.fontMetrics().elidedText(text, Qt::ElideRight, width, Qt::TextShowMnemonic);
		}
		if (text != fStaticText.text()) {
			fStaticText.setText(text);
		}
		if (width != fStaticText.textWidth()) {
			fStaticText.setTextWidth(width);
		}
		if ((alignment & Qt::AlignHorizontal_Mask) != fStaticText.textOption().alignment()) {
			fStaticText.setTextOption(QTextOption(alignment & Qt::AlignHorizontal_Mask));
		}
		
		QRect rect = style()->alignedRect(layoutDirection(), alignment, fStaticText.size().toSize(), QRect(0, 0, width, height()));
		
		painter.drawStaticText(rect.topLeft(), fStaticText);
	}
}


void
Label_Impl::resizeEvent(QResizeEvent *event)
{
	QLabel::resizeEvent(event);
	setIndent(-1);
}


void
Label_Impl::handleLinkActivated(const QString& url)
{
	EventRunner runner(this, "onClick");
	if (runner.isValid()) {
		runner.set("url", url);
		runner.run();
	}
}


SL_DEFINE_METHOD(Label, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->textFormat() == Qt::RichText)
		style |= SL_LABEL_STYLE_HTML;
	if (impl->textInteractionFlags() & Qt::TextSelectableByMouse)
		style |= SL_LABEL_STYLE_SELECTABLE;
	if (impl->textInteractionFlags() & Qt::LinksAccessibleByMouse)
		style |= SL_LABEL_STYLE_FOLLOW_LINKS;
	if (impl->wordWrap())
		style |= SL_LABEL_STYLE_WRAP;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Label, set_style, {
	int style;
	Qt::TextInteractionFlags flags = Qt::NoTextInteraction;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setTextFormat(style & SL_LABEL_STYLE_HTML ? Qt::RichText : Qt::PlainText);
	if (style & SL_LABEL_STYLE_SELECTABLE)
		flags |= Qt::TextSelectableByMouse;
	if (style & SL_LABEL_STYLE_FOLLOW_LINKS)
		flags |= Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard;
	impl->setTextInteractionFlags(flags);
	impl->setWordWrap(style & SL_LABEL_STYLE_WRAP ? true : false);
})


SL_DEFINE_METHOD(Label, get_text, {
	return createStringObject(impl->text());
})


SL_DEFINE_METHOD(Label, set_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setText(text);
})


SL_DEFINE_METHOD(Label, get_align, {
	int align = toAlign(impl->alignment());
	
	return PyInt_FromLong(align);
})


SL_DEFINE_METHOD(Label, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "I", &align))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align);
	
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignVCenter | Qt::AlignLeft;
	
	impl->setAlignment(alignment);
})


SL_START_PROXY_DERIVED(Label, Window)
SL_PROPERTY(style)
SL_PROPERTY(text)
SL_PROPERTY(align)
SL_END_PROXY_DERIVED(Label, Window)


#include "label.moc"
#include "label_h.moc"

