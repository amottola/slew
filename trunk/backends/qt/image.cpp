#include "slew.h"

#include "image.h"



Image_Impl::Image_Impl()
	: QWidget(), WidgetInterface(), fOpacity(1.0)
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
		painter.setOpacity(fOpacity);
		painter.drawPixmap(0, 0, fPixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
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
SL_END_PROXY_DERIVED(Image, Window)


#include "image.moc"
#include "image_h.moc"

