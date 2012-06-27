#include "slew.h"

#include "image.h"



Image_Impl::Image_Impl()
	: QWidget(), WidgetInterface()
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
	
	if (!fPixmap.isNull())
		painter.drawPixmap(0, 0, fPixmap.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}


void
Image_Impl::setPixmap(const QPixmap& pixmap)
{
	fPixmap = pixmap;
	updateGeometry();
	update();
}


SL_DEFINE_METHOD(Image, set_bitmap, {
	QPixmap pixmap;
	
	if (!PyArg_ParseTuple(args, "O&", convertPixmap, &pixmap))
		return NULL;
	
	impl->setPixmap(pixmap);
})


SL_START_PROXY_DERIVED(Image, Window)
SL_METHOD(set_bitmap)
SL_END_PROXY_DERIVED(Image, Window)


#include "image.moc"
#include "image_h.moc"

