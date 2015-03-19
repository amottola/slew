#ifndef __proxy_h__
#define __proxy_h__


typedef struct Abstract_Proxy
{
	PyObject_HEAD
	QObject					*fImpl;
} Abstract_Proxy;

extern PyTypeObject Abstract_Type;
bool Abstract_type_setup(PyObject *module);


typedef struct DC_Proxy {
	PyObject_HEAD
	QPaintDevice	*fDevice;
	QPainter		*fPainter;
	QTransform		*fBaseTransform;
} DC_Proxy;



#define SL_START_METHODS(name)						\
namespace name {									\
static PyMethodDef methods[] = {

#define SL_END_METHODS()							\
	{	NULL, NULL, 0, NULL }						\
}; }												\

#define SL_METHOD(name)								{ #name, (PyCFunction)_##name, METH_VARARGS | METH_KEYWORDS, "" },


#define SL_PROPERTY(name)							\
SL_METHOD(get_##name)								\
SL_METHOD(set_##name)

#define SL_BOOL_PROPERTY(name)						\
SL_METHOD(is_##name)								\
SL_METHOD(set_##name)

#define SL_D(name)									\
name *impl = (name *)self->fImpl;					\
(void)impl;

#define SL_DC()										\
QPainter *painter = self->fPainter;					\
QPaintDevice *device = self->fDevice;				\
QTransform *baseTransform = self->fBaseTransform;	\
(void)painter;										\
(void)device;										\
(void)baseTransform;


#define SL_DEFINE_METHOD(type, name, ...)			\
namespace type { static PyObject * _##name (		\
	type##_Proxy *self, PyObject *args) {			\
	SL_D( type##_Impl )								\
	__VA_ARGS__										\
	Py_RETURN_NONE;									\
} }

#define SL_DEFINE_ABSTRACT_METHOD(type, cls, name, ...)	\
namespace type { static PyObject * _##name (		\
	type##_Proxy *self, PyObject *args) {			\
	cls *impl = ( cls *)self->fImpl;				\
	__VA_ARGS__										\
	Py_RETURN_NONE;									\
} }

#define SL_DEFINE_DC_METHOD(name, ...)				\
static PyObject * _##name (							\
	DC_Proxy *self, PyObject *args) {				\
	SL_DC()											\
	__VA_ARGS__										\
	Py_RETURN_NONE;									\
}

#define SL_DEFINE_MODULE_METHOD(name, ...)			\
namespace slew { static PyObject * _##name (		\
	PyObject *self, PyObject *args, PyObject *kwds) { \
	__VA_ARGS__										\
	Py_RETURN_NONE;									\
} }


#define SL_DECLARE_ABSTRACT_PROXY(name)				\
bool name##_type_setup(PyObject *module);			\
bool is##name (PyObject *object);					\
extern PyTypeObject name##_Type;					\
typedef struct name##_Proxy							\
{													\
	PyObject_HEAD									\
	Widget_Impl				*fImpl;					\
	PyObject				*fWidget;				\
} name##_Proxy;


#define SL_DECLARE_PROXY(name)						\
bool name##_type_setup(PyObject *module);			\
bool is##name (PyObject *object);					\
extern PyTypeObject name##_Type;					\
class name##_Impl;									\
typedef struct name##_Proxy							\
{													\
	PyObject_HEAD									\
	name##_Impl				*fImpl;					\
	PyObject				*fWidget;				\
} name##_Proxy;


#define SL_START_ABSTRACT_PROXY(name)				\
bool name##_type_setup(PyObject *module)			\
{													\
	if (PyType_Ready(& name##_Type) < 0)			\
		return false;								\
	Py_INCREF(& name##_Type);						\
	PyModule_AddObject(module, #name,				\
		(PyObject *)& name##_Type);					\
	return true;									\
}													\
bool is##name (PyObject *object)					\
{													\
	Abstract_Proxy *proxy = getProxy(object);		\
	if (!proxy) { PyErr_Clear(); return false; }	\
	return bool(PyObject_TypeCheck(proxy,			\
		& name##_Type));							\
}													\
SL_START_METHODS(name)


#define SL_END_ABSTRACT_PROXY(name)					\
SL_END_METHODS()									\
PyTypeObject name##_Type = {						\
	PyObject_HEAD_INIT(NULL)						\
	0,						/* ob_size */			\
	"slew._slew." #name,	/* tp_name */			\
	sizeof( name##_Proxy),	/* tp_basicsize */		\
	0,						/* tp_itemsize */		\
	0,						/* tp_dealloc */		\
	0,						/* tp_print */			\
	0,						/* tp_getattr */		\
	0,						/* tp_setattr */		\
	0,						/* tp_compare */		\
	0,						/* tp_repr */			\
	0,						/* tp_as_number */		\
	0,						/* tp_as_sequence */	\
	0,						/* tp_as_mapping */		\
	0,						/* tp_hash */			\
	0,						/* tp_call */			\
	0,						/* tp_str */			\
	0,						/* tp_getattro */		\
	0,						/* tp_setattro */		\
	0,						/* tp_as_buffer */		\
	Py_TPFLAGS_DEFAULT,		/* tp_flags */			\
	#name " objects",		/* tp_doc */			\
	0,						/* tp_traverse */		\
	0,						/* tp_clear */			\
	0,						/* tp_richcompare */	\
	0,						/* tp_weaklistoffset */	\
	0,						/* tp_iter */			\
	0,						/* tp_iternext */		\
	name::methods,			/* tp_methods */		\
	0,						/* tp_members */		\
	0,						/* tp_getset */			\
	&Abstract_Type,			/* tp_base */			\
	0,						/* tp_dict */			\
	0,						/* tp_descr_get */		\
	0,						/* tp_descr_set */		\
	0,						/* tp_dictoffset */		\
	0,						/* tp_init */			\
	0,						/* tp_alloc */			\
	0,						/* tp_new */			\
};


#define SL_START_ABSTRACT_PROXY_DERIVED(name,base)	\
SL_START_ABSTRACT_PROXY(name)


#define SL_END_ABSTRACT_PROXY_DERIVED(name,base)	\
SL_END_METHODS()									\
PyTypeObject name##_Type = {						\
	PyObject_HEAD_INIT(NULL)						\
	0,						/* ob_size */			\
	"slew._slew." #name,	/* tp_name */			\
	sizeof( name##_Proxy),	/* tp_basicsize */		\
	0,						/* tp_itemsize */		\
	0,						/* tp_dealloc */		\
	0,						/* tp_print */			\
	0,						/* tp_getattr */		\
	0,						/* tp_setattr */		\
	0,						/* tp_compare */		\
	0,						/* tp_repr */			\
	0,						/* tp_as_number */		\
	0,						/* tp_as_sequence */	\
	0,						/* tp_as_mapping */		\
	0,						/* tp_hash */			\
	0,						/* tp_call */			\
	0,						/* tp_str */			\
	0,						/* tp_getattro */		\
	0,						/* tp_setattro */		\
	0,						/* tp_as_buffer */		\
	Py_TPFLAGS_DEFAULT,		/* tp_flags */			\
	#name " objects",		/* tp_doc */			\
	0,						/* tp_traverse */		\
	0,						/* tp_clear */			\
	0,						/* tp_richcompare */	\
	0,						/* tp_weaklistoffset */	\
	0,						/* tp_iter */			\
	0,						/* tp_iternext */		\
	name::methods,			/* tp_methods */		\
	0,						/* tp_members */		\
	0,						/* tp_getset */			\
	& base##_Type,			/* tp_base */			\
	0,						/* tp_dict */			\
	0,						/* tp_descr_get */		\
	0,						/* tp_descr_set */		\
	0,						/* tp_dictoffset */		\
	0,						/* tp_init */			\
	0,						/* tp_alloc */			\
	0,						/* tp_new */			\
};


#define SL_START_PROXY(name)															\
bool name##_type_setup(PyObject *module)												\
{																						\
	if (PyType_Ready(& name##_Type) < 0)												\
		return false;																	\
	Py_INCREF(& name##_Type);															\
	PyModule_AddObject(module, #name, (PyObject *)& name##_Type);						\
	return true;																		\
}																						\
bool is##name (PyObject *object)														\
{																						\
	Abstract_Proxy *proxy = getProxy(object);											\
	if (!proxy) { PyErr_Clear(); return false; }										\
	return bool(PyObject_TypeCheck(proxy, & name##_Type));								\
}																						\
static PyObject *																		\
name##_new ( PyTypeObject *type, PyObject *args, PyObject *kwds)						\
{																						\
	name##_Proxy *self = ( name##_Proxy *) type->tp_alloc(type, 0);						\
	if (self) {																			\
		self->fImpl = new name##_Impl();												\
		self->fWidget = Py_None;														\
		Py_INCREF(Py_None);																\
		SL_QAPP()->newProxy((Abstract_Proxy *)self);									\
	}																					\
	return (PyObject *)self;															\
}																						\
static void																				\
name##_dealloc ( name##_Proxy *self)													\
{																						\
	QMutexLocker locker(SL_QAPP()->getLock());											\
	SL_QAPP()->deallocProxy((Abstract_Proxy *)self);									\
	Py_DECREF(self->fWidget);															\
	self->ob_type->tp_free((PyObject*)self);											\
}																						\
static int																				\
name##_init ( name##_Proxy *self, PyObject *args, PyObject *kwds)						\
{																						\
	PyObject *object;																	\
	if (!PyArg_ParseTuple(args, "O", &object))											\
		return -1;																		\
	Py_DECREF(self->fWidget);															\
	self->fWidget = PyWeakref_NewProxy(object, NULL);									\
	return 0;																			\
}																						\
SL_START_METHODS(name)


#define SL_END_PROXY(name)							\
SL_END_METHODS()									\
PyTypeObject name##_Type = {						\
	PyObject_HEAD_INIT(NULL)						\
	0,						/* ob_size */			\
	"slew._slew." #name,	/* tp_name */			\
	sizeof( name##_Proxy),	/* tp_basicsize */		\
	0,						/* tp_itemsize */		\
	(destructor) name##_dealloc, /* tp_dealloc */	\
	0,						/* tp_print */			\
	0,						/* tp_getattr */		\
	0,						/* tp_setattr */		\
	0,						/* tp_compare */		\
	0,						/* tp_repr */			\
	0,						/* tp_as_number */		\
	0,						/* tp_as_sequence */	\
	0,						/* tp_as_mapping */		\
	0,						/* tp_hash */			\
	0,						/* tp_call */			\
	0,						/* tp_str */			\
	0,						/* tp_getattro */		\
	0,						/* tp_setattro */		\
	0,						/* tp_as_buffer */		\
	Py_TPFLAGS_DEFAULT,		/* tp_flags */			\
	#name " objects",		/* tp_doc */			\
	0,						/* tp_traverse */		\
	0,						/* tp_clear */			\
	0,						/* tp_richcompare */	\
	0,						/* tp_weaklistoffset */	\
	0,						/* tp_iter */			\
	0,						/* tp_iternext */		\
	name::methods,			/* tp_methods */		\
	0,						/* tp_members */		\
	0,						/* tp_getset */			\
	&Abstract_Type,			/* tp_base */			\
	0,						/* tp_dict */			\
	0,						/* tp_descr_get */		\
	0,						/* tp_descr_set */		\
	0,						/* tp_dictoffset */		\
	(initproc) name##_init,	/* tp_init */			\
	0,						/* tp_alloc */			\
	name##_new,				/* tp_new */			\
};


#define SL_START_PROXY_DERIVED(name,base)			\
SL_START_PROXY(name)


#define SL_END_PROXY_DERIVED(name,base)				\
SL_END_METHODS()									\
PyTypeObject name##_Type = {						\
	PyObject_HEAD_INIT(NULL)						\
	0,						/* ob_size */			\
	"slew._slew." #name,	/* tp_name */			\
	sizeof( name##_Proxy),	/* tp_basicsize */		\
	0,						/* tp_itemsize */		\
	(destructor) name##_dealloc, /* tp_dealloc */	\
	0,						/* tp_print */			\
	0,						/* tp_getattr */		\
	0,						/* tp_setattr */		\
	0,						/* tp_compare */		\
	0,						/* tp_repr */			\
	0,						/* tp_as_number */		\
	0,						/* tp_as_sequence */	\
	0,						/* tp_as_mapping */		\
	0,						/* tp_hash */			\
	0,						/* tp_call */			\
	0,						/* tp_str */			\
	0,						/* tp_getattro */		\
	0,						/* tp_setattro */		\
	0,						/* tp_as_buffer */		\
	Py_TPFLAGS_DEFAULT,		/* tp_flags */			\
	#name " objects",		/* tp_doc */			\
	0,						/* tp_traverse */		\
	0,						/* tp_clear */			\
	0,						/* tp_richcompare */	\
	0,						/* tp_weaklistoffset */	\
	0,						/* tp_iter */			\
	0,						/* tp_iternext */		\
	name::methods,			/* tp_methods */		\
	0,						/* tp_members */		\
	0,						/* tp_getset */			\
	& base##_Type,			/* tp_base */			\
	0,						/* tp_dict */			\
	0,						/* tp_descr_get */		\
	0,						/* tp_descr_set */		\
	0,						/* tp_dictoffset */		\
	(initproc) name##_init,	/* tp_init */			\
	0,						/* tp_alloc */			\
	name##_new,				/* tp_new */			\
};


#define SL_DECLARE_OBJECT(type, ...)				\
type##_Impl();										\
virtual ~ type##_Impl()								\
{													\
	PyAutoLocker locker;							\
	__VA_ARGS__										\
	SL_QAPP()->unregisterObject(this);				\
}													\
Q_DISABLE_COPY(type##_Impl)


#define SL_DECLARE_SET_VISIBLE(type)				\
virtual void setVisible(bool visible)				\
{													\
	if ((visible) && (!parentWidget()))				\
		return;										\
	type::setVisible(visible);						\
}


#define SL_DECLARE_SIZE_HINT(type)					\
virtual QSize sizeHint() const						\
{													\
	QSize size = qvariant_cast<QSize>(property("explicitSize"));\
	QSize tsize = type::sizeHint();					\
	if (size.width() <= 0)							\
		size.rwidth() = tsize.width();				\
	if (size.height() <= 0)							\
		size.rheight() = tsize.height();			\
	return size;									\
}


#define SL_DECLARE_VIEW(_type)													\
protected:																		\
QList<QPersistentModelIndex>		fHighlights;								\
virtual void paintEvent(QPaintEvent *event) {									\
	PyAutoLocker locker;														\
	QModelIndex tl = indexAt(QPoint(0,0));										\
	QModelIndex br = indexAt(QPoint(viewport()->width() - 1,					\
									viewport()->height() - 1));					\
	DataModel_Impl *model = (DataModel_Impl *)this->model();					\
	EventRunner runner(this, "onPaintView");									\
	if (runner.isValid()) {														\
		runner.set("tl", model->getDataIndex(tl), false);						\
		runner.set("br", model->getDataIndex(br), false);						\
		runner.run();															\
	}																			\
	_type::paintEvent(event);													\
	QPainter painter(viewport());												\
	if (qvariant_cast<bool>(property("dragAccepted"))) {						\
		QModelIndex hover = qvariant_cast<QModelIndex>(property("dragHover"));	\
		int where = qvariant_cast<int>(property("dragWhere"));					\
		paintDropTarget(&painter, hover, where);								\
	}																			\
	foreach (QModelIndex index, fHighlights)									\
		paintDropTarget(&painter, index, SL_EVENT_DRAG_ON_ITEM);				\
}																				\
virtual bool viewportEvent(QEvent *event) {										\
	if ((event->type() == QEvent::MouseMove) ||									\
			(event->type() == QEvent::MouseButtonPress)) {						\
		QCursor cursor = Qt::ArrowCursor;										\
		QMouseEvent *e = (QMouseEvent *)event;									\
		QModelIndex index = indexAt(e->pos());									\
		if ((event->type() == QEvent::MouseButtonPress) && (!index.isValid()))	\
			clearSelection();													\
		DataModel_Impl *model = (DataModel_Impl *)this->model();				\
		DataSpecifier *spec = model->getDataSpecifier(index);					\
		if ((spec) && (!spec->fIcon.isNull()) &&								\
				(spec->isEnabled()) && (spec->isClickableIcon())) {				\
			QRect rect = visualRect(index);										\
			rect.adjust(rect.width() - rect.height() + 1, 1, -1, -1);			\
			QSize size = spec->fIcon.actualSize(rect.size());					\
			if (rect.width() > size.width())									\
				rect.setLeft(rect.left() + ((rect.width() - size.width()) / 2));\
			if (rect.height() > size.height())									\
				rect.setTop(rect.top() + ((rect.height() - size.height()) / 2));\
			if (rect.contains(e->pos())) {										\
				cursor = Qt::PointingHandCursor;								\
				if ((event->type() == QEvent::MouseButtonPress) &&				\
						(((QMouseEvent *)event)->button() == Qt::LeftButton) &&	\
						(!(index.flags() & Qt::ItemIsEditable))) {				\
					EventRunner runner(this, "onClick");						\
					if (runner.isValid()) {										\
						runner.set("index", model->getDataIndex(index), false);	\
						runner.run();											\
					}															\
				}																\
			}																	\
		}																		\
		else if ((spec) && (spec->isHTML()) && (spec->isClickableURLs())) {		\
			ItemDelegate *delegate = (ItemDelegate *)itemDelegate();			\
			QTextDocument *doc = delegate->getTextDocument(index);				\
			if (doc) {															\
				QRect rect = visualRect(index);									\
				QPoint pos = e->pos() - rect.topLeft();							\
				if (spec->fAlignment & Qt::AlignVCenter)						\
					pos.ry() -= (rect.height() - doc->size().height()) / 2;		\
				else if (spec->fAlignment & Qt::AlignBottom)					\
					pos.ry() -= (rect.height() - doc->size().height());			\
				QString url = doc->documentLayout()->anchorAt(pos);				\
				if (!url.isEmpty()) {											\
					cursor = Qt::PointingHandCursor;							\
					if ((event->type() == QEvent::MouseButtonPress) &&			\
						(((QMouseEvent *)event)->button() == Qt::LeftButton) &&	\
							(!(index.flags() & Qt::ItemIsEditable))) {			\
						EventRunner runner(this, "onActivate");					\
						if (runner.isValid()) {									\
							runner.set("index", model->getDataIndex(index),		\
								false);											\
							runner.set("url", url);								\
							runner.run();										\
						}														\
					}															\
				}																\
			}																	\
		}																		\
		setCursor(cursor);														\
	}																			\
	return _type::viewportEvent(event);											\
}																				\
virtual void mouseMoveEvent(QMouseEvent *event)									\
{																				\
	if ((selectionModel()->hasSelection()) &&									\
		(event->buttons() != Qt::NoButton) && (state() != DragSelectingState))	\
		setState(DraggingState);												\
	else																		\
		_type::mouseMoveEvent(event);											\
}																				\
virtual void mouseDoubleClickEvent(QMouseEvent *event) {						\
	ItemDelegate *delegate = (ItemDelegate *)itemDelegate();					\
	if (!delegate->isEditValid())												\
		return;																	\
	_type::mouseDoubleClickEvent(event);										\
}																				\
public:																			\
void paintDropTarget(QPainter *painter, const QModelIndex& index, int where);	\
void setHighlightedIndexes(const QList<QModelIndex>& indexes) {					\
	fHighlights.clear();														\
	foreach (QModelIndex index, indexes)										\
		fHighlights.append(QPersistentModelIndex(index));						\
	update();																	\
}


#define SL_START_VIEW_PROXY(type)												\
SL_DEFINE_METHOD(type, repaint, {												\
	QPoint tl, br;																\
																				\
	if (!PyArg_ParseTuple(args, "O&O&", convertPoint, &tl, convertPoint, &br))	\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	model->invalidateDataSpecifiers();											\
})																				\
SL_DEFINE_METHOD(type, popup_message, {											\
	QString text;																\
	int align, buttons, result;													\
	QPoint pos;																	\
	PyObject *textObj, *object;													\
																				\
	if (!PyArg_ParseTuple(args, "OiiO", &textObj, &align,						\
			&buttons, &object))													\
		return NULL;															\
																				\
	if (textObj == Py_None) {													\
		hidePopupMessage();														\
		Py_RETURN_NONE;															\
	}																			\
	else if (!convertString(textObj, &text)) {									\
		PyErr_SetString(PyExc_ValueError, "Expected str or None object");		\
		return NULL;															\
	}																			\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	QModelIndex index = model->index(object);									\
	if (PyErr_Occurred())														\
		return NULL;															\
																				\
	if (index.isValid()) {														\
		QRect rect = impl->visualRect(index);									\
		QWidget *editor = impl->indexWidget(index);								\
		if ((editor) && (Completer::isRunningOn(editor)) &&						\
				(align == SL_BOTTOM))											\
			align = SL_TOP;														\
																				\
		switch (align) {														\
		case SL_LEFT:	pos = QPoint(0, rect.height()/2); break;				\
		case SL_RIGHT:	pos = QPoint(rect.width()-1, rect.height()/2); break;	\
		case SL_TOP:	pos = QPoint(rect.width()/2, 0); break;					\
		default:		pos = QPoint(rect.width()/2, rect.height()-1); break;	\
		}																		\
		rect.moveTopLeft(impl->viewport()->mapToGlobal(rect.topLeft()));		\
		result = showPopupMessage(impl, editor, text, rect,						\
			impl->viewport()->mapToGlobal(rect.topLeft() + pos),				\
			align, buttons);													\
	}																			\
	else {																		\
		switch (align) {														\
		case SL_LEFT:	pos = QPoint(0, impl->height()/2); break;				\
		case SL_RIGHT:	pos = QPoint(impl->width()-1, impl->height()/2); break;	\
		case SL_TOP:	pos = QPoint(impl->width()/2, 0); break;				\
		default:		pos = QPoint(impl->width()/2, impl->height()-1); break;	\
		}																		\
		result = showPopupMessage(impl, NULL, text, QRect(),					\
			impl->mapToGlobal(pos), align, buttons);							\
	}																			\
	return PyInt_FromLong(result);												\
})																				\
SL_DEFINE_METHOD(type, show_index, {											\
	PyObject *object;															\
																				\
	if (!PyArg_ParseTuple(args, "O", &object))									\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	QModelIndex index = model->index(object);									\
	if (PyErr_Occurred())														\
		return NULL;															\
																				\
	if (index.isValid())														\
		impl->scrollTo(index);													\
})																				\
SL_DEFINE_METHOD(type, get_current_index, {										\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	PyObject *index = model->getDataIndex(impl->currentIndex());				\
	Py_INCREF(index);															\
	return index;																\
})																				\
SL_DEFINE_METHOD(type, set_current_index, {										\
	PyObject *object;															\
																				\
	if (!PyArg_ParseTuple(args, "O", &object))									\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	QModelIndex index = model->index(object);									\
	if (PyErr_Occurred())														\
		return NULL;															\
																				\
	impl->setCurrentIndex(index);												\
})																				\
SL_DEFINE_METHOD(type, set_model, {												\
	PyObject *object;															\
																				\
	if (!PyArg_ParseTuple(args, "O", &object))									\
		return NULL;															\
	if ((object != Py_None) &&													\
			(!PyObject_TypeCheck(object, (PyTypeObject *)PyDataModel_Type))) {	\
		PyErr_SetString(PyExc_ValueError, "expected DataModel object");			\
		return NULL;															\
	}																			\
																				\
	DataModel_Impl *model;														\
	if (object == Py_None)														\
		model = NULL;															\
	else {																		\
		model = (DataModel_Impl *)getImpl(object);								\
		if (!model)																\
			SL_RETURN_NO_IMPL;													\
	}																			\
																				\
	impl->setModel(model);														\
})																				\
SL_DEFINE_METHOD(type, get_selection, {											\
	return getViewSelection(impl);												\
})																				\
SL_DEFINE_METHOD(type, set_selection, {											\
	PyObject *object;															\
																				\
	if (!PyArg_ParseTuple(args, "O", &object))									\
		return NULL;															\
																				\
	if (!setViewSelection(impl, object))										\
		return NULL;															\
})																				\
SL_DEFINE_METHOD(type, get_index_at, {											\
	QPoint pos;																	\
																				\
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))						\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	PyObject *index = model->getDataIndex(impl->indexAt(pos));					\
	Py_INCREF(index);															\
	return index;																\
})																				\
SL_DEFINE_METHOD(type, get_index_rect, {										\
	PyObject *object;															\
																				\
	if (!PyArg_ParseTuple(args, "O", &object))									\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	QRect rect = impl->visualRect(model->index(object));						\
	if (PyErr_Occurred())														\
		return NULL;															\
																				\
	QPoint diff = impl->viewport()->mapToParent(QPoint(0,0));					\
	QPoint tl = rect.topLeft() + diff;											\
	QPoint br = rect.bottomRight() + diff;										\
																				\
	PyObject *tuple = PyTuple_New(2);											\
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(tl));							\
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(br));							\
	return tuple;																\
})																				\
SL_DEFINE_METHOD(type, set_highlighted_indexes, {								\
	PyObject *object, *sequence;												\
	Py_ssize_t size, i;															\
	QList<QModelIndex> indexes;													\
																				\
	if (!PyArg_ParseTuple(args, "O!", &PyTuple_Type, &object))					\
		return NULL;															\
																				\
	DataModel_Impl *model = (DataModel_Impl *)impl->model();					\
	sequence = PySequence_Fast(object, "expected tuple object");				\
	if (!sequence)																\
		return NULL;															\
																				\
	size = PySequence_Fast_GET_SIZE(sequence);									\
	for (i = 0; i < size; i++) {												\
		PyObject *item = PySequence_Fast_GET_ITEM(sequence, i);					\
		QModelIndex index;														\
																				\
		if (!PyObject_TypeCheck(item, (PyTypeObject *)PyDataIndex_Type))		\
			PyErr_SetString(PyExc_ValueError, "expected DataIndex object");		\
		else																	\
			index = model->index(item);											\
		if (PyErr_Occurred()) {													\
			Py_DECREF(sequence);												\
			return NULL;														\
		}																		\
		indexes.append(index);													\
	}																			\
	Py_DECREF(sequence);														\
	impl->setHighlightedIndexes(indexes);										\
})																				\
SL_DEFINE_METHOD(type, get_scroll_pos, {										\
	QScrollBar *hbar = impl->horizontalScrollBar();								\
	QScrollBar *vbar = impl->verticalScrollBar();								\
	QPoint pos(hbar ? hbar->value() : 0, vbar ? vbar->value() : 0);				\
	return createVectorObject(pos);												\
})																				\
SL_DEFINE_METHOD(type, set_scroll_pos, {										\
	QScrollBar *hbar = impl->horizontalScrollBar();								\
	QScrollBar *vbar = impl->verticalScrollBar();								\
	QPoint pos;																	\
																				\
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))						\
		return NULL;															\
																				\
	if (hbar) hbar->setValue(pos.x());											\
	if (vbar) vbar->setValue(pos.y());											\
})																				\
SL_DEFINE_METHOD(type, get_scroll_rate, {										\
	return createVectorObject(QPoint(impl->horizontalScrollBar()->singleStep(),	\
									 impl->verticalScrollBar()->singleStep()));	\
})																				\
SL_DEFINE_METHOD(type, set_scroll_rate, {										\
	QPoint rate;																\
																				\
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &rate))						\
		return NULL;															\
																				\
	impl->horizontalScrollBar()->setSingleStep(rate.x());						\
	impl->verticalScrollBar()->setSingleStep(rate.y());							\
})																				\
SL_DEFINE_METHOD(type, get_viewport_size, {										\
	return createVectorObject(impl->viewport()->size());						\
})																				\
SL_DEFINE_METHOD(type, has_focus, {												\
	QWidget *window = impl->window();											\
	if (!window)																\
		Py_RETURN_FALSE;														\
	QWidget *focus = window->focusWidget();										\
	if ((focus) && (impl->isAncestorOf(focus)))									\
		Py_RETURN_TRUE;															\
	return createBoolObject(impl->hasFocus());									\
})																				\
SL_START_PROXY_DERIVED(type, Window)											\
SL_METHOD(repaint)																\
SL_METHOD(popup_message)														\
SL_METHOD(show_index)															\
SL_METHOD(get_current_index)													\
SL_METHOD(set_current_index)													\
SL_METHOD(set_model)															\
SL_METHOD(get_selection)														\
SL_METHOD(set_selection)														\
SL_METHOD(get_index_at)															\
SL_METHOD(get_index_rect)														\
SL_METHOD(set_highlighted_indexes)												\
SL_METHOD(get_scroll_pos)														\
SL_METHOD(set_scroll_pos)														\
SL_METHOD(get_scroll_rate)														\
SL_METHOD(set_scroll_rate)														\
SL_METHOD(get_viewport_size)													\
SL_METHOD(has_focus)

#define SL_END_VIEW_PROXY(type)													\
SL_END_PROXY_DERIVED(type, Window)


#endif
