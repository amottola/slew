#include "slew.h"

#include "image.h"

#include "constants/image.h"

#include <QStyle>



Image_Impl::Image_Impl()
	: QWidget(), WidgetInterface(), fOpacity(1.0), fFit(false), fScale(true)
{
}


QSize
Image_Impl::sizeHint() const
{
	QSize size = qvariant_cast<QSize>(property("explicitSize"));
	if ((size.isNull()) || (!size.isValid())) {
		if (fPixmap.isNull())
			size = QSize(10, 10);
		else
			size = fPixmap.size();
	}
	return size;
}


void
Image_Impl::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	
	if (!fPixmap.isNull()) {
		QPixmap pixmap;
		QRect dest;
		
		painter.setOpacity(fOpacity);
		
		if (fScale) {
			pixmap = fPixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
			dest = rect();
		}
		else if (fFit) {
			pixmap = fPixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
			dest = QStyle::alignedRect(Qt::LayoutDirectionAuto, fAlignment, fPixmap.size(), rect());
		}
		else {
			pixmap = fPixmap;
			dest = QStyle::alignedRect(Qt::LayoutDirectionAuto, fAlignment, fPixmap.size(), rect());
		}
		painter.drawPixmap(dest, pixmap);
	}
}


void
Image_Impl::setPixmap(const QPixmap& pixmap)
{
	fPixmap = pixmap;
	updateGeometry();
	update();
}


void
Image_Impl::setOpacity(double opacity)
{
	fOpacity = qBound(0.0, opacity, 1.0);
	update();
}


void
Image_Impl::setFit(bool fit)
{
	fFit = fit;
	updateGeometry();
	update();
}


void
Image_Impl::setScale(bool scale)
{
	fScale = scale;
	updateGeometry();
	update();
}


void
Image_Impl::setAlignment(Qt::Alignment alignment)
{
	fAlignment = alignment;
	updateGeometry();
	update();
}


SL_DEFINE_METHOD(Image, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->fit())
		style |= SL_IMAGE_STYLE_FIT;
	if (impl->scale())
		style |= SL_IMAGE_STYLE_SCALE;
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Image, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setFit(style & SL_IMAGE_STYLE_FIT ? true : false);
	impl->setScale(style & SL_IMAGE_STYLE_SCALE ? true : false);
})


SL_DEFINE_METHOD(Image, get_align, {
	int align = toAlign(impl->alignment());
	
	return PyInt_FromLong(align);
})


SL_DEFINE_METHOD(Image, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "I", &align))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align);
	
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignVCenter | Qt::AlignLeft;
	
	impl->setAlignment(alignment);
})


SL_DEFINE_METHOD(Image, set_bitmap, {
	QPixmap pixmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &pixmap))
		return NULL;
	
	impl->setPixmap(pixmap);
})


SL_DEFINE_METHOD(Image, set_opacity, {
	double opacity;
	
	if (!PyArg_ParseTuple(args, "d", &opacity))
		return NULL;
	
	impl->setOpacity(opacity);
})


SL_START_PROXY_DERIVED(Image, Window)
SL_METHOD(set_bitmap)
SL_METHOD(set_opacity)
SL_PROPERTY(style)
SL_PROPERTY(align)
SL_END_PROXY_DERIVED(Image, Window)


#include "image.moc"
#include "image_h.moc"

