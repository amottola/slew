#include "slew.h"
#include "objects.h"

#include "sceneview.h"

#include "constants/sceneview.h"
#include "constants/window.h"

#include <QResizeEvent>
#include <QKeyEvent>
#include <QGLWidget>
#include <QGLFormat>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneWheelEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsBlurEffect>
#include <QGraphicsColorizeEffect>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QScrollBar>
#include <QTextDocument>
#include <QTextFrame>
#include <QTextOption>
#include <QTextCursor>



SL_DEFINE_DC_METHOD(get_size, {
	SceneItem_Impl *item = (SceneItem_Impl *)device;
	return createVectorObject(item->size());
})


SL_DEFINE_DC_METHOD(clear, {
	SceneItem_Impl *item = (SceneItem_Impl *)device;
	QBrush brush(painter->brush());
	brush.setStyle(Qt::SolidPattern);
	painter->fillRect(QRect(QPoint(0, 0), item->size()), brush);
})


SL_START_METHODS(SceneItemDC)
SL_METHOD(get_size)
SL_METHOD(clear)
SL_END_METHODS()



PyTypeObject SceneItemDC_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"slew._slew.SceneItemDC",					/* tp_name */
	sizeof(DC_Proxy),							/* tp_basicsize */
	0,											/* tp_itemsize */
	0,											/* tp_dealloc */
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
	"SceneItemDC objects",						/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	SceneItemDC::methods,						/* tp_methods */
	0,											/* tp_members */
	0,											/* tp_getset */
	&DC_Type,									/* tp_base */
};


bool
SceneItemDC_type_setup(PyObject *module)
{
	if (PyType_Ready(&SceneItemDC_Type) < 0)
		return false;
	Py_INCREF(&SceneItemDC_Type);
	PyModule_AddObject(module, "SceneItemDC", (PyObject *)&SceneItemDC_Type);
	return true;
}



SceneItem_Impl::SceneItem_Impl()
	: QGraphicsTextItem(), fSize(100, 100), fRepaint(true)
{
	setAcceptDrops(true);
	setAcceptHoverEvents(true);
	setCacheMode(DeviceCoordinateCache);
	setFlags(ItemUsesExtendedStyleOption);
	document()->setDocumentMargin(0);
}


QRectF
SceneItem_Impl::boundingRect() const
{
	return QRectF(QRect(fOrigin, fSize));
}


void
SceneItem_Impl::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	if (fRepaint) {
		QPicture picture;
		QPainter picturePainter(&picture);
		picturePainter.setRenderHints(painter->renderHints());
		EventRunner runner(this, "onPaint");
		if (runner.isValid()) {
			runner.set("dc", createDCObject(&picturePainter, NULL, (PyObject *)&SceneItemDC_Type, (QPaintDevice *)this));
			runner.run();
		}
		fPicture = picture;
		fRepaint = false;
	}
	painter->setClipRect(option->exposedRect);
	painter->drawPicture(0, 0, fPicture);
	
	if ((!toPlainText().isEmpty()) || (hasFocus())) {
		if (hasFocus()) {
			painter->save();
			int item_h = fSize.height();
			int text_h = document()->size().height();
			int offset = 0;
// 			qDebug() << item_h << text_h;
			switch (document()->defaultTextOption().alignment() & Qt::AlignVertical_Mask) {
			case Qt::AlignBottom:		offset = item_h - text_h; break;
			case Qt::AlignVCenter:		offset = (item_h - text_h) / 2; break;
			}
			QStyleOptionGraphicsItem o(*option);
			o.state &= ~(QStyle::State_Selected | QStyle::State_HasFocus);
			QRectF rect = painter->clipBoundingRect();
			rect.adjust(0, 0, 0, offset);
			o.exposedRect.adjust(0, 0, 0, offset);
			o.rect.adjust(0, 0, 0, offset);
			painter->setClipRect(rect);
			painter->translate(0, offset);
			QGraphicsTextItem::paint(painter, &o, widget);
			painter->restore();
		}
		else {
			QGraphicsTextItem::paint(painter, option, widget);
		}
	}
}


bool
SceneItem_Impl::sceneEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::GraphicsSceneMousePress:
	case QEvent::GraphicsSceneMouseRelease:
	case QEvent::GraphicsSceneMouseMove:
	case QEvent::GraphicsSceneMouseDoubleClick:
		{
			QGraphicsSceneMouseEvent *e = (QGraphicsSceneMouseEvent *)event;
			EventRunner runner(this);
			if (runner.isValid()) {
				runner.set("item", getObject(this));
				runner.set("item_pos", e->pos());
				runner.set("scene_pos", e->scenePos());
				runner.set("pos", e->screenPos());
				runner.set("buttons", (int)e->buttons());
				runner.set("modifiers", getKeyModifiers(e->modifiers()));
				
				switch (e->type()) {
				case QEvent::GraphicsSceneMousePress:
					{
						runner.setName("onMouseDown");
						runner.set("down", (int)e->button());
					}
					break;
				case QEvent::GraphicsSceneMouseRelease:
					{
						if (e->button() == Qt::LeftButton) {
							runner.setName("onClick");
							runner.run();
						}
						runner.setName("onMouseUp");
						runner.set("up", (int)e->button());
					}
					break;
				case QEvent::GraphicsSceneMouseMove:
					{
						runner.setName("onMouseMove");
					}
					break;
				case QEvent::GraphicsSceneMouseDoubleClick:
					{
						runner.setName("onDblClick");
						runner.set("down", (int)e->button());
					}
					break;
				default:
					break;
				}
				if (!runner.run())
					return true;
			}
		}
		break;
	
	case QEvent::GraphicsSceneHoverEnter:
	case QEvent::GraphicsSceneHoverLeave:
	case QEvent::GraphicsSceneHoverMove:
		{
			QGraphicsSceneHoverEvent *e = (QGraphicsSceneHoverEvent *)event;
			EventRunner runner(this);
			if (runner.isValid()) {
				runner.set("item", getObject(this));
				runner.set("item_pos", e->pos());
				runner.set("scene_pos", e->scenePos());
				runner.set("pos", e->screenPos());
				runner.set("buttons", (int)QApplication::mouseButtons());
				runner.set("modifiers", getKeyModifiers(e->modifiers()));
				switch (e->type()) {
				case QEvent::GraphicsSceneHoverEnter:
					{
						runner.setName("onMouseEnter");
					}
					break;
				case QEvent::GraphicsSceneHoverLeave:
					{
						runner.setName("onMouseLeave");
					}
					break;
				case QEvent::GraphicsSceneHoverMove:
					{
						runner.setName("onMouseMove");
					}
					break;
				default:
					break;
				}
				if (!runner.run())
					return true;
			}
		}
		break;
	
	case QEvent::GraphicsSceneWheel:
		{
			QGraphicsSceneWheelEvent *e = (QGraphicsSceneWheelEvent *)event;
			EventRunner runner(this, "onMouseWheel");
			if (runner.isValid()) {
				runner.set("item", getObject(this));
				runner.set("item_pos", e->pos());
				runner.set("scene_pos", e->scenePos());
				runner.set("pos", e->screenPos());
				runner.set("buttons", (int)QApplication::mouseButtons());
				runner.set("modifiers", getKeyModifiers(e->modifiers()));
				runner.set("delta", e->delta() / 100);
				if (!runner.run())
					return true;
			}
		}
		break;
	
	case QEvent::GraphicsSceneContextMenu:
		{
			QGraphicsSceneContextMenuEvent *e = (QGraphicsSceneContextMenuEvent *)event;
			EventRunner runner(this, "onContextMenu");
			if (runner.isValid()) {
				runner.set("item", getObject(this));
				runner.set("item_pos", e->pos());
				runner.set("scene_pos", e->scenePos());
				runner.set("pos", e->screenPos());
				runner.set("modifiers", getKeyModifiers(e->modifiers()));
				if (!runner.run())
					return true;
			}
		}
		break;
	
	case QEvent::FocusIn:
		{
			EventRunner(this, "onFocusIn").run();
		}
		break;
	
	case QEvent::FocusOut:
		{
			EventRunner(this, "onFocusOut").run();
		}
		break;
	
	case QEvent::KeyPress:
		{
			if (hasFocus())
				update();
		}
		/* fallthrough */
		
	case QEvent::KeyRelease:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			bool sendCharEvent = false;
			
			int modifiers = getKeyModifiers(e->modifiers());
			QString text = e->text();
			int seq = SL_SEQUENCE_NONE;
			
			EventRunner runner(this);
			if (runner.isValid()) {
				if (event->type() == QEvent::KeyPress) {
					if (text.isEmpty()) {
						if (e->matches(QKeySequence::Undo))
							seq = SL_SEQUENCE_UNDO;
						else if (e->matches(QKeySequence::Redo))
							seq = SL_SEQUENCE_REDO;
						else if (e->matches(QKeySequence::Cut))
							seq = SL_SEQUENCE_CUT;
						else if (e->matches(QKeySequence::Copy))
							seq = SL_SEQUENCE_COPY;
						else if (e->matches(QKeySequence::Paste))
							seq = SL_SEQUENCE_PASTE;
						else if (e->matches(QKeySequence::Close))
							seq = SL_SEQUENCE_CLOSE;
					}
					runner.setName("onKeyDown");
					runner.set("count", e->count());
					sendCharEvent = !text.isEmpty();
				}
				else {
					runner.setName("onKeyUp");
				}
				runner.set("modifiers", modifiers);
				runner.set("code", getKeyCode(e->key()));
				runner.set("char", text);
				runner.set("seq", seq);
				
				if (!runner.run())
					return true;
				
				if (sendCharEvent) {
					runner.setName("onChar");
					if (!runner.run())
						return true;
				}
			}
		}
		break;
	
	default:
		break;
	}
	
	return QGraphicsTextItem::sceneEvent(event);
}


void
SceneItem_Impl::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsTextItem::mousePressEvent(event);
	QGraphicsScene *scene = this->scene();
	if (scene) {
		SceneView_Impl *view = (SceneView_Impl *)scene->parent();
		if (view->dragMode() == QGraphicsView::NoDrag)
			event->accept();
	}
}


QVariant
SceneItem_Impl::itemChange(GraphicsItemChange change, const QVariant& value)
{
	switch (change) {
	case ItemSelectedHasChanged:
		{
// 			fRepaint = true;
		}
		break;
	default:
		break;
	}
	return QGraphicsTextItem::itemChange(change, value);
}


SL_DEFINE_METHOD(SceneItem, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (!isSceneItem(object))
		SL_RETURN_CANNOT_ATTACH;
	
	SceneItem_Impl *item = (SceneItem_Impl *)child;
	
	if (item->parentItem() != impl)
		item->setParentItem(impl);
})


SL_DEFINE_METHOD(SceneItem, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (!isSceneItem(object))
		SL_RETURN_CANNOT_DETACH;
	
	SceneItem_Impl *item = (SceneItem_Impl *)child;
	
	item->setParentItem(NULL);
	if (item->scene())
		item->scene()->removeItem(item);
})


SL_DEFINE_METHOD(SceneItem, repaint, {
	QPointF tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPointF, &tl, convertPointF, &br))
		return NULL;
	
	impl->invalidate();
	if ((tl.isNull()) && (br.isNull())) {
		impl->update();
	}
	else {
		impl->update(QRectF(tl, br));
	}
})


SL_DEFINE_METHOD(SceneItem, map_to_scene, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	return createVectorObject(impl->mapToScene(pos));
})


SL_DEFINE_METHOD(SceneItem, map_from_scene, {
	QPointF pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPointF, &pos))
		return NULL;
	
	return createVectorObject(impl->mapFromScene(pos));
})


SL_DEFINE_METHOD(SceneItem, get_scene_rect, {
	QRectF rect = impl->sceneBoundingRect();
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(rect.topLeft()));
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(rect.bottomRight()));
	return tuple;
})


SL_DEFINE_METHOD(SceneItem, set_effect, {
	int type;
	PyObject *params, *obj;
	
	if (!PyArg_ParseTuple(args, "iO!", &type, &PyDict_Type, &params))
		return NULL;
	
	switch (type) {
	case SL_SCENE_ITEM_EFFECT_BLUR:
		{
			double radius = 5.0;
			
			obj = PyDict_GetItemString(params, "radius");
			if (obj) {
				radius = PyFloat_AsDouble(obj);
				if (PyErr_Occurred())
					return NULL;
			}
			
			QGraphicsBlurEffect *effect = new QGraphicsBlurEffect();
			effect->setBlurRadius(radius);
			
			impl->setGraphicsEffect(effect);
		}
		break;
	
	case SL_SCENE_ITEM_EFFECT_COLORIZE:
		{
			QColor color(0, 0, 192);
			double strength = 1.0;
			
			obj = PyDict_GetItemString(params, "color");
			if (obj) {
				if (!convertColor(obj, &color))
					return NULL;
			}
			obj = PyDict_GetItemString(params, "strength");
			if (obj) {
				strength = PyFloat_AsDouble(obj);
				if (PyErr_Occurred())
					return NULL;
			}
			
			QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
			effect->setColor(color);
			effect->setStrength(strength);
			
			impl->setGraphicsEffect(effect);
		}
		break;
	
	case SL_SCENE_ITEM_EFFECT_SHADOW:
		{
			double radius = 2.0;
			QColor color(63, 63, 63, 180);
			QPointF offset(8, 8);
			
			obj = PyDict_GetItemString(params, "radius");
			if (obj) {
				radius = PyFloat_AsDouble(obj);
				if (PyErr_Occurred())
					return NULL;
			}
			obj = PyDict_GetItemString(params, "color");
			if (obj) {
				if (!convertColor(obj, &color))
					return NULL;
			}
			obj = PyDict_GetItemString(params, "offset");
			if (obj) {
				if (!convertPointF(obj, &offset))
					return NULL;
			}
			
			QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
			effect->setBlurRadius(radius);
			effect->setColor(color);
			effect->setOffset(offset);
			
			impl->setGraphicsEffect(effect);
		}
		break;
	
	case SL_SCENE_ITEM_EFFECT_OPACITY:
		{
			double opacity = 0.7;
			
			obj = PyDict_GetItemString(params, "opacity");
			if (obj) {
				opacity = PyFloat_AsDouble(obj);
				if (PyErr_Occurred())
					return NULL;
			}
			
			QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
			effect->setOpacity(opacity);
			
			impl->setGraphicsEffect(effect);
		}
		break;
	
	default:
		{
			impl->setGraphicsEffect(NULL);
		}
		break;
	}
})


SL_DEFINE_METHOD(SceneItem, set_focus, {
	bool focused;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &focused))
		return NULL;
	
	if (focused) {
		impl->setFocus();
		if (impl->textInteractionFlags() == Qt::TextEditorInteraction) {
			QTextCursor cursor = impl->textCursor();
			cursor.select(QTextCursor::Document);
			impl->setTextCursor(cursor);
		}
	}
	else
		impl->clearFocus();
})


SL_DEFINE_METHOD(SceneItem, set_grab, {
	bool grabbed;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &grabbed))
		return NULL;
	
	if (grabbed)
		impl->grabMouse();
	else
		impl->ungrabMouse();
})


SL_DEFINE_METHOD(SceneItem, ensure_visible, {
	impl->ensureVisible();
})


SL_DEFINE_METHOD(SceneItem, get_pos, {
	return createVectorObject(impl->pos().toPoint());
})


SL_DEFINE_METHOD(SceneItem, set_pos, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	if (pos != impl->pos().toPoint()) {
		impl->setPos(QPointF(pos));
	}
})


SL_DEFINE_METHOD(SceneItem, get_size, {
	return createVectorObject(impl->size());
})


SL_DEFINE_METHOD(SceneItem, set_size, {
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	if (size != impl->size()) {
		impl->invalidate();
		impl->setSize(size);
	}
})


SL_DEFINE_METHOD(SceneItem, get_origin, {
	return createVectorObject(impl->origin());
})


SL_DEFINE_METHOD(SceneItem, set_origin, {
	QPoint origin;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &origin))
		return NULL;
	
	if (origin != impl->origin()) {
		impl->invalidate();
		impl->setOrigin(origin);
	}
})


SL_DEFINE_METHOD(SceneItem, is_visible, {
	return createBoolObject(impl->isVisible());
})


SL_DEFINE_METHOD(SceneItem, set_visible, {
	bool visible;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &visible))
		return NULL;
	
	impl->setVisible(visible);
})


SL_DEFINE_METHOD(SceneItem, get_zorder, {
	return PyInt_FromLong(impl->zValue());
})


SL_DEFINE_METHOD(SceneItem, set_zorder, {
	int z;
	
	if (!PyArg_ParseTuple(args, "i", &z))
		return NULL;
	
	impl->setZValue(z);
})


SL_DEFINE_METHOD(SceneItem, get_tip, {
	return createStringObject(impl->toolTip());
})


SL_DEFINE_METHOD(SceneItem, set_tip, {
	QString tip;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &tip))
		return NULL;
	
	impl->setToolTip(tip);
})


SL_DEFINE_METHOD(SceneItem, is_enabled, {
	return createBoolObject(impl->isEnabled());
})


SL_DEFINE_METHOD(SceneItem, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	impl->setEnabled(enabled);
})


SL_DEFINE_METHOD(SceneItem, get_cursor, {
	QCursor cursor(impl->cursor());
	int id;
	
	switch (cursor.shape()) {
	case Qt::IBeamCursor:			id = SL_CURSOR_IBEAM; break;
	case Qt::PointingHandCursor:	id = SL_CURSOR_HAND; break;
	case Qt::WaitCursor:			id = SL_CURSOR_WAIT; break;
	case Qt::CrossCursor:			id = SL_CURSOR_CROSS; break;
	case Qt::BlankCursor:			id = SL_CURSOR_NONE; break;
	case Qt::ClosedHandCursor:		id = SL_CURSOR_MOVE; break;
	case Qt::SizeAllCursor:			id = SL_CURSOR_RESIZE; break;
	case Qt::SizeVerCursor:			id = SL_CURSOR_RESIZE_VERTICAL; break;
	case Qt::SizeHorCursor:			id = SL_CURSOR_RESIZE_HORIZONTAL; break;
	case Qt::SizeFDiagCursor:		id = SL_CURSOR_RESIZE_DIAGONAL_F; break;
	case Qt::SizeBDiagCursor:		id = SL_CURSOR_RESIZE_DIAGONAL_B; break;
	default:						id = SL_CURSOR_NORMAL; break;
	}

	return PyInt_FromLong(id);
})


SL_DEFINE_METHOD(SceneItem, set_cursor, {
	Qt::CursorShape cursor;
	int id;
	
	if (!PyArg_ParseTuple(args, "i", &id))
		return NULL;
	
	switch (id) {
	case SL_CURSOR_IBEAM:				cursor = Qt::IBeamCursor; break;
	case SL_CURSOR_HAND:				cursor = Qt::PointingHandCursor; break;
	case SL_CURSOR_WAIT:				cursor = Qt::WaitCursor; break;
	case SL_CURSOR_CROSS:				cursor = Qt::CrossCursor; break;
	case SL_CURSOR_NONE:				cursor = Qt::BlankCursor; break;
	case SL_CURSOR_MOVE:				cursor = Qt::ClosedHandCursor; break;
	case SL_CURSOR_RESIZE:				cursor = Qt::SizeAllCursor; break;
	case SL_CURSOR_RESIZE_VERTICAL:		cursor = Qt::SizeVerCursor; break;
	case SL_CURSOR_RESIZE_HORIZONTAL:	cursor = Qt::SizeHorCursor; break;
	case SL_CURSOR_RESIZE_DIAGONAL_F:	cursor = Qt::SizeFDiagCursor; break;
	case SL_CURSOR_RESIZE_DIAGONAL_B:	cursor = Qt::SizeBDiagCursor; break;
	default:
	case SL_CURSOR_NORMAL:				cursor = Qt::ArrowCursor; break;
	}
	
	impl->setCursor(QCursor(cursor));
})


SL_DEFINE_METHOD(SceneItem, get_flags, {
	int flags = 0;
	
	if (impl->flags() & QGraphicsItem::ItemIgnoresTransformations)
		flags |= SL_SCENE_ITEM_FLAG_DONT_SCALE;
	if (impl->flags() & QGraphicsItem::ItemIsSelectable)
		flags |= SL_SCENE_ITEM_FLAG_SELECTABLE;
	if (impl->acceptedMouseButtons() == Qt::NoButton)
		flags |= SL_SCENE_ITEM_FLAG_CLICKABLE;
	if (impl->textInteractionFlags() == Qt::TextEditorInteraction)
		flags |= SL_SCENE_ITEM_FLAG_EDITABLE;
	if (impl->flags() & QGraphicsItem::ItemIsFocusable)
		flags |= SL_SCENE_ITEM_FLAG_FOCUSABLE;
	
	return PyInt_FromLong(flags);
})


SL_DEFINE_METHOD(SceneItem, set_flags, {
	int flags;
	QGraphicsItem::GraphicsItemFlags qflags = QGraphicsItem::ItemUsesExtendedStyleOption;
	
	if (!PyArg_ParseTuple(args, "i", &flags))
		return NULL;
	
	impl->setTextInteractionFlags(flags & SL_SCENE_ITEM_FLAG_EDITABLE ? Qt::TextEditorInteraction : Qt::NoTextInteraction);
	
	if (flags & SL_SCENE_ITEM_FLAG_DONT_SCALE)
		qflags |= QGraphicsItem::ItemIgnoresTransformations;
	if (flags & SL_SCENE_ITEM_FLAG_SELECTABLE)
		qflags |= QGraphicsItem::ItemIsSelectable;
	if (flags & (SL_SCENE_ITEM_FLAG_EDITABLE | SL_SCENE_ITEM_FLAG_FOCUSABLE))
		qflags |= QGraphicsItem::ItemIsFocusable;
	
	impl->setFlags(qflags);
	impl->setAcceptedMouseButtons(flags & SL_SCENE_ITEM_FLAG_CLICKABLE ? (Qt::LeftButton | Qt::RightButton | Qt::MiddleButton) : Qt::NoButton);
})


SL_DEFINE_METHOD(SceneItem, is_selected, {
	return createBoolObject(impl->isSelected());
})


SL_DEFINE_METHOD(SceneItem, set_selected, {
	bool selected;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &selected))
		return NULL;
	
	impl->setSelected(selected);
})


SL_DEFINE_METHOD(SceneItem, get_font, {
	return createFontObject(impl->font());
})


SL_DEFINE_METHOD(SceneItem, set_font, {
	QFont font;
	
	if (!PyArg_ParseTuple(args, "O&", convertFont, &font))
		return NULL;
	
	impl->setFont(font);
})


SL_DEFINE_METHOD(SceneItem, get_text, {
	return createStringObject(impl->toPlainText());
})


SL_DEFINE_METHOD(SceneItem, set_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->setPlainText(text);
})


SL_DEFINE_METHOD(SceneItem, get_align, {
	int align = toAlign(impl->document()->defaultTextOption().alignment());
	
	return PyInt_FromLong(align);
})


SL_DEFINE_METHOD(SceneItem, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "I", &align))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align);
	
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignVCenter | Qt::AlignLeft;
	
	QTextOption option(alignment);
	impl->document()->setDefaultTextOption(option);
})


SL_DEFINE_METHOD(SceneItem, get_color, {
	return createColorObject(impl->defaultTextColor());
})


SL_DEFINE_METHOD(SceneItem, set_color, {
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	impl->setDefaultTextColor(color);
})


SL_DEFINE_METHOD(SceneItem, get_padding, {
	QTextFrame *frame = impl->document()->rootFrame();
	QTextFrameFormat format = frame->frameFormat(); 
	
	return Py_BuildValue("(iiii)", (int)format.topMargin(), (int)format.rightMargin(), (int)format.bottomMargin(), (int)format.leftMargin());
})


SL_DEFINE_METHOD(SceneItem, set_padding, {
	QTextFrame *frame = impl->document()->rootFrame();
	QTextFrameFormat format = frame->frameFormat(); 
	QList<int> margins;
	int top = 0, right = 0, bottom = 0, left = 0;
	
	if (!PyArg_ParseTuple(args, "O&", convertIntList, &margins))
		return NULL;
	
	if (margins.size() == 1) {
		right = bottom = left = top = margins[0];
	}
	else if (margins.size() == 2) {
		top = bottom = margins[0];
		left = right = margins[1];
	}
	else if (margins.size() == 3) {
		top = margins[0];
		left = right = margins[1];
		bottom = margins[2];
	}
	else if (margins.size() >= 4) {
		top = margins[0];
		right = margins[1];
		bottom = margins[2];
		left = margins[3];
	}
	format.setTopMargin(top);
	format.setRightMargin(right);
	format.setBottomMargin(bottom);
	format.setLeftMargin(left);
	
	frame->setFrameFormat(format);
})


SL_START_PROXY(SceneItem)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(repaint)
SL_METHOD(map_to_scene)
SL_METHOD(map_from_scene)
SL_METHOD(get_scene_rect)
SL_METHOD(set_effect)
SL_METHOD(set_focus)
SL_METHOD(set_grab)
SL_METHOD(ensure_visible)

SL_PROPERTY(pos)
SL_PROPERTY(size)
SL_PROPERTY(origin)
SL_BOOL_PROPERTY(visible)
SL_PROPERTY(zorder)
SL_PROPERTY(tip)
SL_BOOL_PROPERTY(enabled)
SL_PROPERTY(cursor)
SL_PROPERTY(flags)
SL_BOOL_PROPERTY(selected)
SL_PROPERTY(font)
SL_PROPERTY(text)
SL_PROPERTY(align)
SL_PROPERTY(color)
SL_PROPERTY(padding)
SL_END_PROXY(SceneItem)



SceneView_Impl::SceneView_Impl()
	: QGraphicsView(), WidgetInterface()
{
	setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::NonCosmeticDefaultPen);
	setScene(new QGraphicsScene(this));
	setDragMode(NoDrag);
	
	QWidget *corner = new QWidget;
	QPalette palette;
	palette.setColor(QPalette::Base, QApplication::palette(this).color(QPalette::Base));
	corner->setAutoFillBackground(true);
	corner->setPalette(palette);
	setCornerWidget(corner);
	
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(handleScrollBars()));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(handleScrollBars()));
	connect(scene(), SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
}


void
SceneView_Impl::drawBackground(QPainter *painter, const QRectF& rect)
{
	QColor color = palette().base().color();
	painter->fillRect(rect, color);
	
	bool enabled = painter->worldMatrixEnabled();
	bool hasClipping = painter->hasClipping();
	painter->setWorldMatrixEnabled(false);
	painter->setClipping(false);
	
	EventRunner runner(this, "onPaint");
	if (runner.isValid()) {
		runner.set("dc", createDCObject(painter));
		runner.set("background", true);
		runner.run();
	}
	
	painter->setWorldMatrixEnabled(enabled);
	painter->setClipping(hasClipping);
}


void
SceneView_Impl::drawForeground(QPainter *painter, const QRectF& rect)
{
	bool enabled = painter->worldMatrixEnabled();
	bool hasClipping = painter->hasClipping();
	painter->setWorldMatrixEnabled(false);
	painter->setClipping(false);
	
	EventRunner runner(this, "onPaint");
	if (runner.isValid()) {
		runner.set("dc", createDCObject(painter));
		runner.set("background", false);
		runner.run();
	}
	
	painter->setWorldMatrixEnabled(enabled);
	painter->setClipping(hasClipping);
}


void
SceneView_Impl::paintEvent(QPaintEvent *event)
{
	QGraphicsView::paintEvent(event);
}


void
SceneView_Impl::resizeEvent(QResizeEvent *event)
{
	if (event->size() != event->oldSize()) {
		EventRunner runner(this, "onResize");
		if (runner.isValid()) {
			runner.set("size", event->size());
			runner.run();
		}
	}
	QGraphicsView::resizeEvent(event);
}


void
SceneView_Impl::focusOutEvent(QFocusEvent *event)
{
	QGraphicsView::focusOutEvent(event);
	if (event->reason() == Qt::PopupFocusReason) {
		QGraphicsItem *item = scene()->mouseGrabberItem();
		if (item)
			item->ungrabMouse();
	}
}


void
SceneView_Impl::contextMenuEvent(QContextMenuEvent *event)
{
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		SceneItem_Impl *item = (SceneItem_Impl *)itemAt(event->pos());
		if (item)
			runner.set("item", getObject(item));
		else
			runner.set("item", Py_None, false);
		runner.set("pos", event->pos());
		runner.set("modifiers", getKeyModifiers(event->modifiers()));
		if (!runner.run())
			return;
	}
	QGraphicsView::contextMenuEvent(event);
}


void
SceneView_Impl::handleScrollBars()
{
	EventRunner runner(this, "onScroll");
	if (runner.isValid()) {
		QScrollBar *hsb = horizontalScrollBar();
		QScrollBar *vsb = verticalScrollBar();
		
		runner.set("minimum", QPoint(hsb->minimum(), vsb->minimum()));
		runner.set("maximum", QPoint(hsb->maximum(), vsb->maximum()));
		runner.set("value", QPoint(hsb->value(), vsb->value()));
		runner.run();
	}
}


void
SceneView_Impl::handleSelectionChanged()
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		runner.set("selection", selection());
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		runner.run();
	}
}


PyObject *
SceneView_Impl::selection()
{
	QList<QGraphicsItem *> list = scene()->selectedItems();
	PyObject *selection = PyTuple_New(list.count());
	int i = 0;
	
	foreach(QGraphicsItem *item, list) {
		PyObject *object = getObject((QGraphicsObject *)item);
		if (!object) {
			PyErr_Clear();
			object = Py_None;
			Py_INCREF(object);
		}
		PyTuple_SET_ITEM(selection, i, object);
		i++;
	}
	return selection;
}


SL_DEFINE_METHOD(SceneView, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (!isSceneItem(object))
		SL_RETURN_CANNOT_ATTACH;
	
	SceneItem_Impl *item = (SceneItem_Impl *)child;
	if (item->scene() != impl->scene())
		impl->scene()->addItem(item);
})


SL_DEFINE_METHOD(SceneView, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QObject *child = getImpl(object);
	if (!child)
		SL_RETURN_NO_IMPL;
	
	if (!isSceneItem(object))
		SL_RETURN_CANNOT_DETACH;
	
	SceneItem_Impl *item = (SceneItem_Impl *)child;
	
	impl->scene()->removeItem(item);
})


SL_DEFINE_METHOD(SceneView, repaint, {
	QPoint tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPoint, &tl, convertPoint, &br))
		return NULL;
	
	QGraphicsScene *scene = impl->scene();
	if ((tl.isNull()) && (br.isNull())) {
		scene->update();
	}
	else {
		scene->update(impl->mapToScene(QRect(tl, br)).boundingRect());
	}
})


SL_DEFINE_METHOD(SceneView, map_to_scene, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	return createVectorObject(impl->mapToScene(pos));
})


SL_DEFINE_METHOD(SceneView, map_from_scene, {
	QPointF pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPointF, &pos))
		return NULL;
	
	return createVectorObject(impl->mapFromScene(pos));
})


SL_DEFINE_METHOD(SceneView, ensure_visible, {
	QPointF tl, br;
	QPoint margin;
	
	if (!PyArg_ParseTuple(args, "O&O&O&", convertPointF, &tl, convertPointF, &br, convertPoint, &margin))
		return NULL;
	
	impl->ensureVisible(QRectF(tl, br), margin.x(), margin.y());
})


SL_DEFINE_METHOD(SceneView, center_on, {
	QPointF pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPointF, &pos))
		return NULL;
	
	impl->centerOn(pos);
})


SL_DEFINE_METHOD(SceneView, fit_in_view, {
	QPointF tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPointF, &tl, convertPointF, &br))
		return NULL;
	
	impl->fitInView(QRectF(tl, br), Qt::KeepAspectRatio);
})


SL_DEFINE_METHOD(SceneView, get_items_bounding_rect, {
	QRectF rect = impl->scene()->itemsBoundingRect();
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(rect.topLeft()));
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(rect.bottomRight()));
	return tuple;
})


SL_DEFINE_METHOD(SceneView, get_items_at, {
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	QList<QGraphicsItem *> list = impl->items(pos);
	PyObject *tuple = PyTuple_New(list.size());
	for (int i = 0; i < list.size(); i++) {
		SceneItem_Impl *item = (SceneItem_Impl *)list[i];
		PyObject *object = getObject(item);
		if (!object) {
			PyErr_Clear();
			object = Py_None;
			Py_INCREF(object);
		}
		PyTuple_SET_ITEM(tuple, i, object);
	}
	return tuple;
})


SL_DEFINE_METHOD(SceneView, get_viewport_size, {
	return createVectorObject(impl->viewport()->size());
})


SL_DEFINE_METHOD(SceneView, set_bgcolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		impl->setAutoFillBackground(false);
		color = QApplication::palette(impl).color(QPalette::Base);
	}
	else if (!qobject_cast<QGLWidget *>(impl->viewport())) {
		impl->setAutoFillBackground(true);
	}
	
	palette.setColor(QPalette::Base, color);
	impl->setPalette(palette);
	impl->cornerWidget()->setPalette(palette);
})


SL_DEFINE_METHOD(SceneView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (qobject_cast<QGLWidget *>(impl->viewport()))
		style |= SL_SCENEVIEW_STYLE_OPENGL;
	if (impl->horizontalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)
		style |= SL_SCENEVIEW_STYLE_SCROLLBARS;
	if (impl->dragMode() == QGraphicsView::ScrollHandDrag)
		style |= SL_SCENEVIEW_STYLE_DRAG_SCROLL;
	else if (impl->dragMode() == QGraphicsView::RubberBandDrag)
		style |= SL_SCENEVIEW_STYLE_DRAG_SELECT;
	if ((impl->renderHints() & (QPainter::Antialiasing | QPainter::TextAntialiasing)) == 0)
		style |= SL_SCENEVIEW_STYLE_NO_ANTIALIAS;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(SceneView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	QWidget *viewport = impl->viewport();
	if ((style & SL_SCENEVIEW_STYLE_OPENGL) && !(qobject_cast<QGLWidget *>(impl->viewport()))) {
		QGLWidget *widget = new QGLWidget();
		if (widget->isValid()) {
			viewport = widget;
		}
	}
	if ((!(style & SL_SCENEVIEW_STYLE_OPENGL) && (qobject_cast<QGLWidget *>(impl->viewport()))) || (!viewport)) {
		viewport = new QWidget();
	}
	
	impl->setViewport(viewport);
	impl->setViewportUpdateMode(qobject_cast<QGLWidget *>(viewport) ? QGraphicsView::FullViewportUpdate : QGraphicsView::SmartViewportUpdate);
	impl->setAutoFillBackground((impl->palette().color(QPalette::Base) != QApplication::palette(impl).color(QPalette::Base)) && (!qobject_cast<QGLWidget *>(viewport)));
	impl->setHorizontalScrollBarPolicy(style & SL_SCENEVIEW_STYLE_SCROLLBARS ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded);
	impl->setVerticalScrollBarPolicy(style & SL_SCENEVIEW_STYLE_SCROLLBARS ? Qt::ScrollBarAlwaysOn : Qt::ScrollBarAsNeeded);
	if (style & SL_SCENEVIEW_STYLE_DRAG_SCROLL)
		impl->setDragMode(QGraphicsView::ScrollHandDrag);
	else if (style & SL_SCENEVIEW_STYLE_DRAG_SELECT)
		impl->setDragMode(QGraphicsView::RubberBandDrag);
	else
		impl->setDragMode(QGraphicsView::NoDrag);
	
	QPainter::RenderHints hints = impl->renderHints();
	if (style & SL_SCENEVIEW_STYLE_NO_ANTIALIAS)
		hints &= ~(QPainter::Antialiasing | QPainter::TextAntialiasing);
	else
		hints |= (QPainter::Antialiasing | QPainter::TextAntialiasing);
	impl->setRenderHints(hints);
})


SL_DEFINE_METHOD(SceneView, get_anchor, {
	int anchor;
	
	switch (impl->transformationAnchor()) {
	case QGraphicsView::AnchorViewCenter:	anchor = SL_SCENEVIEW_ANCHOR_CENTER; break;
	case QGraphicsView::AnchorUnderMouse:	anchor = SL_SCENEVIEW_ANCHOR_MOUSE; break;
	default:								anchor = SL_SCENEVIEW_ANCHOR_NONE; break;
	}
	
	return PyInt_FromLong(anchor);
})


SL_DEFINE_METHOD(SceneView, set_anchor, {
	int anchor;
	QGraphicsView::ViewportAnchor qanchor;
	
	if (!PyArg_ParseTuple(args, "i", &anchor))
		return NULL;
	
	switch (anchor) {
	case SL_SCENEVIEW_ANCHOR_CENTER:		qanchor = QGraphicsView::AnchorViewCenter; break;
	case SL_SCENEVIEW_ANCHOR_MOUSE:			qanchor = QGraphicsView::AnchorUnderMouse; break;
	default:								qanchor = QGraphicsView::NoAnchor; break;
	}
	impl->setTransformationAnchor(qanchor);
})


SL_DEFINE_METHOD(SceneView, get_align, {
	return PyInt_FromLong(toAlign(impl->alignment()));
})


SL_DEFINE_METHOD(SceneView, set_align, {
	int align;
	Qt::Alignment qalign;
	
	if (!PyArg_ParseTuple(args, "i", &align))
		return NULL;
	
	qalign = fromAlign(align);
	if (qalign == 0)
		qalign = Qt::AlignCenter;
	impl->setAlignment(qalign);
})


SL_DEFINE_METHOD(SceneView, get_scale, {
	QTransform transform = impl->transform();
	return PyFloat_FromDouble(transform.m11());
})


SL_DEFINE_METHOD(SceneView, set_scale, {
	double scale;
	
	if (!PyArg_ParseTuple(args, "d", &scale))
		return NULL;
	
	impl->resetTransform();
	impl->scale(scale, scale);
})


SL_DEFINE_METHOD(SceneView, get_scene_rect, {
	QRectF rect = impl->scene()->sceneRect();
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(rect.topLeft()));
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(rect.bottomRight()));
	return tuple;
})


SL_DEFINE_METHOD(SceneView, set_scene_rect, {
	PyObject *objectTl, *objectBr;
	QPointF tl, br;
	QRectF rect;
	
	if (!PyArg_ParseTuple(args, "OO", &objectTl, &objectBr))
		return NULL;
	
	if ((objectTl != Py_None) && (objectBr != Py_None)) {
		if ((!convertPointF(objectTl, &tl)) || (!convertPointF(objectBr, &br)))
			return NULL;
		rect = QRectF(tl, br);
	}
	
	impl->scene()->setSceneRect(rect);
})


SL_DEFINE_METHOD(SceneView, get_selection, {
	return impl->selection();
})


SL_DEFINE_METHOD(SceneView, set_selection_rect, {
	PyObject *objectTl, *objectBr;
	QPointF tl, br;
	QRectF rect;
	
	if (!PyArg_ParseTuple(args, "OO", &objectTl, &objectBr))
		return NULL;
	
	if ((objectTl != Py_None) && (objectBr != Py_None)) {
		if ((!convertPointF(objectTl, &tl)) || (!convertPointF(objectBr, &br)))
			return NULL;
		rect = QRectF(tl, br);
	}
	else if (objectBr == Py_None) {
		if (!convertPointF(objectTl, &tl))
			return NULL;
		rect = QRectF(tl, QSizeF(1, 1));
	}
	
	if (!rect.isValid()) {
		impl->scene()->clearSelection();
	}
	else {
		QPainterPath path;
		path.addRect(rect);
		impl->scene()->setSelectionArea(path, impl->viewportTransform());
	}
})


SL_DEFINE_METHOD(SceneView, clear_selection, {
	impl->scene()->clearSelection();
})


SL_DEFINE_METHOD(SceneView, get_scroll_pos, {
	QScrollBar *hbar = impl->horizontalScrollBar();
	QScrollBar *vbar = impl->verticalScrollBar();
	QPoint pos(hbar ? hbar->value() - hbar->minimum(): 0, vbar ? vbar->value() - vbar->minimum(): 0);
	
	return createVectorObject(pos);
})


SL_DEFINE_METHOD(SceneView, set_scroll_pos, {
	QScrollBar *hbar = impl->horizontalScrollBar();
	QScrollBar *vbar = impl->verticalScrollBar();
	QPoint pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &pos))
		return NULL;
	
	if (hbar) hbar->setValue(hbar->minimum() + pos.x());
	if (vbar) vbar->setValue(vbar->minimum() + pos.y());
})


SL_DEFINE_METHOD(SceneView, get_scroll_rate, {
	return createVectorObject(QPoint(impl->horizontalScrollBar()->singleStep(),
									 impl->verticalScrollBar()->singleStep()));
})


SL_DEFINE_METHOD(SceneView, set_scroll_rate, {
	QPoint rate;
	
	if (!PyArg_ParseTuple(args, "O&", convertPoint, &rate))
		return NULL;
	
	impl->horizontalScrollBar()->setSingleStep(rate.x());
	impl->verticalScrollBar()->setSingleStep(rate.y());
})


SL_START_PROXY_DERIVED(SceneView, Window)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(repaint)
SL_METHOD(map_to_scene)
SL_METHOD(map_from_scene)
SL_METHOD(ensure_visible)
SL_METHOD(center_on)
SL_METHOD(fit_in_view)
SL_METHOD(get_items_bounding_rect)
SL_METHOD(get_items_at)
SL_METHOD(get_viewport_size)
SL_METHOD(get_selection)
SL_METHOD(set_selection_rect)
SL_METHOD(clear_selection)
SL_METHOD(get_scroll_pos)
SL_METHOD(set_scroll_pos)
SL_METHOD(get_scroll_rate)
SL_METHOD(set_scroll_rate)
SL_METHOD(set_bgcolor)

SL_PROPERTY(style)
SL_PROPERTY(anchor)
SL_PROPERTY(align)
SL_PROPERTY(scale)
SL_PROPERTY(scene_rect)
SL_END_PROXY_DERIVED(SceneView, Window)


#include "sceneview.moc"
#include "sceneview_h.moc"

