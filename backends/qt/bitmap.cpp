#include "slew.h"

#include "objects.h"
#include "constants/gdi.h"

#include <QBuffer>
#include <QImageReader>


static PyObject *
_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	DC_Proxy *self = ( DC_Proxy *)type->tp_alloc(type, 0);
	if (self) {
		self->fDevice = NULL;
		self->fPainter = NULL;
		self->fBaseTransform = NULL;
	}
	return (PyObject *)self;
}


static int
_init(DC_Proxy *self, PyObject *args, PyObject *kwds)
{
	QSize size(32,32);
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return -1;
	
	delete self->fPainter;
	delete self->fDevice;
	
	QPixmap *pixmap = new QPixmap(size);
	pixmap->fill(QColor(255,255,255,0));
	self->fDevice = pixmap;
	self->fPainter = new QPainter(pixmap);
	self->fPainter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::NonCosmeticDefaultPen);
	self->fPainter->translate(0.5, 0.5);
	
	return 0;
}


static void
_dealloc(DC_Proxy *self)
{
	freeBitmapResources(self->fPainter, self->fDevice);
	self->ob_type->tp_free((PyObject*)self);
}


SL_DEFINE_DC_METHOD(get_size, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	
	return createVectorObject(pixmap->size());
})


SL_DEFINE_DC_METHOD(set_size, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	painter->end();
	*pixmap = pixmap->copy(QRect(QPoint(0,0), size));
	painter->begin(pixmap);
})


SL_DEFINE_DC_METHOD(get_bits, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QByteArray buffer;
	
	QImage image = pixmap->toImage();
	image = image.convertToFormat(QImage::Format_ARGB32);
	
	buffer.resize(image.width() * image.height() * 4);
	
	for (int y = 0; y < image.height(); y++) {
		buffer.replace(image.width() * y * 4, image.width() * 4, (const char *)image.scanLine(y), image.width() * 4);
	}
	
	return createBufferObject(buffer);
})


SL_DEFINE_DC_METHOD(set_bits, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QByteArray buffer;
	
	if (!PyArg_ParseTuple(args, "O&", convertBuffer, &buffer))
		return NULL;
	
	if (pixmap->width() * pixmap->height() * 4 > buffer.size()) {
		PyErr_SetString(PyExc_RuntimeError, "bits buffer is too small");
		return NULL;
	}
	QImage image((const uchar *)buffer.data(), pixmap->width(), pixmap->height(), QImage::Format_ARGB32);
	painter->end();
	*pixmap = QPixmap::fromImage(image);
	painter->begin(pixmap);
})


SL_DEFINE_DC_METHOD(blit, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	PyObject *object, *sizeObj, *sourcePosObj, *sourceSizeObj;
	QPoint pos, sourcePos;
	QSize size;
	QRect target, source = pixmap->rect();
	bool repeat;
	
	if (!PyArg_ParseTuple(args, "O!O&OOOO&", PyDC_Type, &object, convertPoint, &pos, &sizeObj, &sourcePosObj, &sourceSizeObj, convertBool, &repeat))
		return NULL;
	
	target.setTopLeft(pos);
	if (sizeObj == Py_None) {
		target.setSize(pixmap->size());
	}
	else {
		if (!convertSize(sizeObj, &size))
			return NULL;
		target.setSize(size);
	}
	if (sourcePosObj != Py_None) {
		if (!convertPoint(sourcePosObj, &pos))
			return NULL;
		source.setTopLeft(pos);
	}
	if (sourceSizeObj != Py_None) {
		if (!convertSize(sourceSizeObj, &size))
			return NULL;
		source.setSize(size);
	}
	
	DC_Proxy *proxy = (DC_Proxy *)PyObject_GetAttrString(object, "_impl");
	if (!proxy)
		return NULL;
	
	if (repeat) {
		proxy->fPainter->fillRect(target, QBrush(*pixmap));
	}
	else {
		proxy->fPainter->drawPixmap(target, *pixmap, source);
	}
	
	Py_DECREF(proxy);
})


SL_DEFINE_DC_METHOD(load, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QByteArray bytes;
	QBuffer buffer;
	
	if (!PyArg_ParseTuple(args, "O&", convertBuffer, &bytes))
		return NULL;
	
	buffer.setData(bytes);
	QImageReader reader(&buffer);
	QImage image = reader.read();
	if (image.isNull()) {
		PyErr_Format(PyExc_RuntimeError,  "cannot load bitmap (%s)", (const char *)reader.errorString().toUtf8());
		return NULL;
	}
	painter->end();
	*pixmap = QPixmap::fromImage(image);
	painter->begin(pixmap);
})


SL_DEFINE_DC_METHOD(save, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QByteArray bytes;
	QBuffer buffer(&bytes);
	
	painter->end();
	pixmap->save(&buffer, "PNG");
	painter->begin(pixmap);
	
	return createBufferObject(bytes);
})


SL_DEFINE_DC_METHOD(copy, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	return createBitmapObject(*pixmap);
})


SL_DEFINE_DC_METHOD(resized, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QSize size;
	bool smooth;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertSize, &size, convertBool, &smooth))
		return NULL;
	
	return createBitmapObject(pixmap->scaled(size, Qt::IgnoreAspectRatio, smooth ? Qt::SmoothTransformation : Qt::FastTransformation));
})


SL_DEFINE_DC_METHOD(colorized, {
	QPixmap *pixmap = (QPixmap *)self->fDevice;
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	QImage image = pixmap->toImage();
	for (int y = 0; y < image.height(); y++) {
		QRgb *rgb = (QRgb *)image.scanLine(y);
		for (int x = 0; x < image.width(); x++) {
			int hue = qGray(*rgb);
			int alpha = qAlpha(*rgb);
			*rgb++ = qRgba((color.red() * hue) >> 8, (color.green() * hue) >> 8, (color.blue() * hue) >> 8, alpha);
		}
	}
	return createBitmapObject(QPixmap::fromImage(image));
})


SL_START_METHODS(Bitmap)
SL_PROPERTY(size)
SL_PROPERTY(bits)

SL_METHOD(blit)
SL_METHOD(load)
SL_METHOD(save)
SL_METHOD(copy)
SL_METHOD(resized)
SL_METHOD(colorized)
SL_END_METHODS()


PyTypeObject Bitmap_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"slew._slew.Bitmap",						/* tp_name */
	sizeof(DC_Proxy),							/* tp_basicsize */
	0,											/* tp_itemsize */
	(destructor)_dealloc,						/* tp_dealloc */
	0,											/* tp_print */
	0,											/* tp_getattr */
	0,											/* tp_setattr */
	0,											/* tp_compare */
	0,											/* tp_repr */
	0,											/* tp_as_number */
	0,											/* tp_as_sequence */
	0,											/* tp_as_mapping */
	0,											/* tp_hash */
	0,											/* tp_call */
	0,											/* tp_str */
	0,											/* tp_getattro */
	0,											/* tp_setattro */
	0,											/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,							/* tp_flags */
	"Bitmap objects",							/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	Bitmap::methods,							/* tp_methods */
	0,											/* tp_members */
	0,											/* tp_getset */
	&DC_Type,									/* tp_base */
	0,											/* tp_dict */
	0,											/* tp_descr_get */
	0,											/* tp_descr_set */
	0,											/* tp_dictoffset */
	(initproc)_init,							/* tp_init */
	0,											/* tp_alloc */
	(newfunc)_new,								/* tp_new */
};


bool
Bitmap_type_setup(PyObject *module)
{
	if (PyType_Ready(&Bitmap_Type) < 0)
		return false;
	Py_INCREF(&Bitmap_Type);
	PyModule_AddObject(module, "Bitmap", (PyObject *)&Bitmap_Type);
	return true;
}


#include "bitmap.moc"
