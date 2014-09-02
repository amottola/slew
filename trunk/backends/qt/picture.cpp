#include "slew.h"

#include "objects.h"
#include "constants/gdi.h"

#include <QBuffer>
#include <QPicture>
#include <QSvgRenderer>


extern int qt_defaultDpiX();
extern int qt_defaultDpiY();


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
	QSize size;
	if (!PyArg_ParseTuple(args, "|O&", convertSize, &size))
		return -1;
	
	delete self->fPainter;
	delete self->fDevice;
	
	QPicture *pict = new QPicture();
	if (size.isValid())
		pict->setBoundingRect(QRect(QPoint(0,0), size));
	
	self->fDevice = pict;
	self->fPainter = new QPainter(pict);
// 	self->fPainter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::NonCosmeticDefaultPen);
	
	return 0;
}


static void
_dealloc(DC_Proxy *self)
{
	freeBitmapResources(self->fPainter, self->fDevice);
	self->ob_type->tp_free((PyObject*)self);
}


SL_DEFINE_DC_METHOD(get_size, {
	QPicture *pict = (QPicture *)self->fDevice;
	QRect rect = pict->boundingRect();
	rect.setTopLeft(QPoint(0, 0));
	if (rect.isValid())
		return createVectorObject(rect.size());
	else
		return createVectorObject(QSize(0, 0));
})


SL_DEFINE_DC_METHOD(set_size, {
	QPicture *pict = (QPicture *)self->fDevice;
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	pict->setBoundingRect(QRect(QPoint(0,0), size));
})


SL_DEFINE_DC_METHOD(blit, {
	QPicture *pict = (QPicture *)self->fDevice;
	PyObject *object, *sizeObj, *sourcePosObj, *sourceSizeObj;
	QPoint pos, sourcePos;
	QSize size;
	QRect target, source = pict->boundingRect();
	source.setTopLeft(QPoint(0, 0));
	bool repeat;
	
	if (!PyArg_ParseTuple(args, "O!O&OOOO&", PyDC_Type, &object, convertPoint, &pos, &sizeObj, &sourcePosObj, &sourceSizeObj, convertBool, &repeat))
		return NULL;
	
	target.setTopLeft(pos);
	if (sizeObj == Py_None) {
		target.setSize(source.size());
	}
	else {
		if (!convertSize(sizeObj, &size))
			return NULL;
		target.setSize(size);
	}
	if ((sourcePosObj != Py_None) || (sourceSizeObj != Py_None)) {
		PyErr_SetString(PyExc_ValueError, "Not allowed with Picture source object");
		return NULL;
	}
	
	DC_Proxy *proxy = (DC_Proxy *)PyObject_GetAttrString(object, "_impl");
	if (!proxy)
		return NULL;
	
	if (repeat) {
		PyErr_SetString(PyExc_ValueError, "Not allowed with Picture source object");
		Py_DECREF(proxy);
		return NULL;
	}
	else {
		QPaintDevice *dev = proxy->fPainter->device();
		double xScale, yScale;
		xScale = ((double)target.width() / (double)source.width()) * ((double)qt_defaultDpiX() / dev->logicalDpiX());
		yScale = ((double)target.height() / (double)source.height()) * ((double)qt_defaultDpiY() / dev->logicalDpiY());
		painter->end();
		proxy->fPainter->save();
		proxy->fPainter->translate(target.topLeft());
		proxy->fPainter->scale(xScale, yScale);
		pict->play(proxy->fPainter);
		proxy->fPainter->restore();
		painter->begin(pict);
	}
	
	Py_DECREF(proxy);
})


SL_DEFINE_DC_METHOD(load, {
	QPicture *pict = (QPicture *)self->fDevice;
	QByteArray bytes;
	QBuffer buffer;
	QPicture newPict;
	
	if (!PyArg_ParseTuple(args, "O&", convertBuffer, &bytes))
		return NULL;
	
	buffer.setData(bytes);
	buffer.open(QIODevice::ReadOnly);
	if (newPict.load(&buffer)) {
		painter->end();
		painter->begin(pict);
		painter->drawPicture(0, 0, newPict);
	}
	else {
		QSvgRenderer renderer(bytes);
		if (!renderer.isValid()) {
			PyErr_Format(PyExc_RuntimeError,  "cannot load picture");
			return NULL;
		}
		painter->end();
		*pict = QPicture();
		QRect rect = renderer.viewBox();
		rect.setTopLeft(QPoint(0, 0));
		pict->setBoundingRect(rect);
		painter->begin(pict);
		renderer.render(painter);
	}
})


SL_DEFINE_DC_METHOD(save, {
	QPicture *pict = (QPicture *)self->fDevice;
	QByteArray bytes;
	QBuffer buffer(&bytes);
	buffer.open(QIODevice::WriteOnly);
	
	painter->end();
	pict->save(&buffer);
	painter->begin(pict);
	
	return createBufferObject(bytes);
})


SL_DEFINE_DC_METHOD(copy, {
	QPicture *pict = (QPicture *)self->fDevice;
	return createPictureObject(*pict);
})


SL_START_METHODS(Picture)
SL_PROPERTY(size)

SL_METHOD(blit)
SL_METHOD(load)
SL_METHOD(save)
SL_METHOD(copy)
SL_END_METHODS()


PyTypeObject Picture_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"slew._slew.Picture",						/* tp_name */
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
	"Picture objects",							/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	Picture::methods,							/* tp_methods */
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
Picture_type_setup(PyObject *module)
{
	if (PyType_Ready(&Picture_Type) < 0)
		return false;
	Py_INCREF(&Picture_Type);
	PyModule_AddObject(module, "Picture", (PyObject *)&Picture_Type);
	return true;
}


#include "bitmap.moc"
