#include "slew.h"

#include "objects.h"
#include "constants/gdi.h"


#include <QFontMetricsF>



SL_DEFINE_DC_METHOD(get_color, {
	return createColorObject(painter->pen().color());
})


SL_DEFINE_DC_METHOD(set_color, {
	QPen pen = painter->pen();
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (color != pen.color()) {
		if ((color.isValid()) && (pen.style() == Qt::NoPen))
			pen.setStyle(Qt::SolidLine);
		else if (!color.isValid())
			pen.setStyle(Qt::NoPen);
		pen.setColor(color);
		painter->setPen(pen);
	}
})


SL_DEFINE_DC_METHOD(get_bgcolor, {
	if (painter->brush().style() == Qt::NoBrush)
		Py_RETURN_NONE;
	return createColorObject(painter->brush().color());
})


SL_DEFINE_DC_METHOD(set_bgcolor, {
	QBrush brush = painter->brush();
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (color.isValid()) {
		brush.setStyle(Qt::SolidPattern);
		brush.setColor(color);
	}
	else {
		brush.setStyle(Qt::NoBrush);
		brush.setColor(Qt::transparent);
	}
	if (brush != painter->brush())
		painter->setBrush(brush);
})


SL_DEFINE_DC_METHOD(get_font, {
	return createFontObject(painter->font());
})


SL_DEFINE_DC_METHOD(set_font, {
	QFont font;
	int strategy;
	
	if (!PyArg_ParseTuple(args, "O&", convertFont, &font))
		return NULL;
	
	strategy = font.styleStrategy() & ~(QFont::PreferAntialias | QFont::NoAntialias);
	strategy |= painter->renderHints() & QPainter::TextAntialiasing ? QFont::PreferAntialias : QFont::NoAntialias;
	
	font.setStyleHint(font.styleHint(), (QFont::StyleStrategy)strategy);
	font.setStyleStrategy((QFont::StyleStrategy)strategy);
	font.resolve(QApplication::font());
	if (font != painter->font())
		painter->setFont(font);
})


SL_DEFINE_DC_METHOD(get_size, {
	return createVectorObject(painter->viewport().size());
})


SL_DEFINE_DC_METHOD(set_size, {
	PyErr_SetString(PyExc_RuntimeError, "cannot resize device context");
	return NULL;
})


SL_DEFINE_DC_METHOD(get_pensize, {
	return PyInt_FromLong(painter->pen().width());
})


SL_DEFINE_DC_METHOD(set_pensize, {
	QPen pen = painter->pen();
	int size;
	
	if (!PyArg_ParseTuple(args, "i", &size))
		return NULL;
	
	if (size != pen.width()) {
		pen.setWidth(size);
		painter->setPen(pen);
	}
})


SL_DEFINE_DC_METHOD(get_penstyle, {
	int style;
	
	switch (painter->pen().style()) {
	case Qt::DashLine:			style = SL_DC_PEN_STYLE_DASH; break;
	case Qt::DotLine:			style = SL_DC_PEN_STYLE_DOT; break;
	case Qt::DashDotLine:		style = SL_DC_PEN_STYLE_DASH_DOT; break;
	case Qt::DashDotDotLine:	style = SL_DC_PEN_STYLE_DASH_DOT_DOT; break;
	case Qt::CustomDashLine:
		{
			QString pattern;
			QVector<qreal> vector = painter->pen().dashPattern();
			for (int i = 0; i < vector.size(); i += 2) {
				pattern += QString("-").repeated(vector[i]);
				pattern += QString(" ").repeated(vector[i+1]);
			}
			return createStringObject(pattern);
		}
		break;
	case Qt::SolidLine:
	default:					style = SL_DC_PEN_STYLE_SOLID; break;
	}
	
	return PyInt_FromLong(style);
})


SL_DEFINE_DC_METHOD(set_penstyle, {
	QPen pen = painter->pen();
	PyObject *styleObj;
	int style;
	
	if (!PyArg_ParseTuple(args, "O", &styleObj))
		return NULL;
	style = PyInt_AsLong(styleObj);
	if (!PyErr_Occurred()) {
		switch (style) {
		case SL_DC_PEN_STYLE_DASH:			pen.setStyle(Qt::DashLine); break;
		case SL_DC_PEN_STYLE_DOT:			pen.setStyle(Qt::DotLine); break;
		case SL_DC_PEN_STYLE_DASH_DOT:		pen.setStyle(Qt::DashDotLine); break;
		case SL_DC_PEN_STYLE_DASH_DOT_DOT:	pen.setStyle(Qt::DashDotDotLine); break;
		default:							pen.setStyle(Qt::SolidLine); break;
		}
	}
	else {
		PyErr_Clear();
		QString pattern;
		if (!convertString(styleObj, &pattern))
			return NULL;
		QVector<qreal> vector;
		int solid = 0, empty = 0;
		for (int i = 0; i < pattern.size(); i++) {
			if (pattern[i].isSpace())
				empty++;
			else
				solid++;
			if (((i + 1 == pattern.size()) || (pattern[i].isSpace() != pattern[i+1].isSpace())) && (empty) && (solid)) {
				vector.append(solid);
				vector.append(empty);
				solid = empty = 0;
			}
		}
		pen.setDashPattern(vector);
	}
	if (pen != painter->pen())
		painter->setPen(pen);
})


SL_DEFINE_DC_METHOD(get_brushstyle, {
	int style;
	
	switch (painter->brush().style()) {
	case Qt::Dense1Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_90; break;
	case Qt::Dense2Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_80; break;
	case Qt::Dense3Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_70; break;
	case Qt::Dense4Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_50; break;
	case Qt::Dense5Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_40; break;
	case Qt::Dense6Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_20; break;
	case Qt::Dense7Pattern:		style = SL_DC_BRUSH_STYLE_DENSE_10; break;
	case Qt::HorPattern:		style = SL_DC_BRUSH_STYLE_H_LINES; break;
	case Qt::VerPattern:		style = SL_DC_BRUSH_STYLE_V_LINES; break;
	case Qt::CrossPattern:		style = SL_DC_BRUSH_STYLE_CROSS_LINES; break;
	case Qt::BDiagPattern:		style = SL_DC_BRUSH_STYLE_BD_LINES; break;
	case Qt::FDiagPattern:		style = SL_DC_BRUSH_STYLE_FD_LINES; break;
	case Qt::DiagCrossPattern:	style = SL_DC_BRUSH_STYLE_D_CROSS_LINES; break;
	case Qt::SolidPattern:
	default:					style = SL_DC_BRUSH_STYLE_SOLID; break;
	}
	
	return PyInt_FromLong(style);
})


SL_DEFINE_DC_METHOD(set_brushstyle, {
	QBrush brush = painter->brush();
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	switch (style) {
	case SL_DC_BRUSH_STYLE_DENSE_90:		brush.setStyle(Qt::Dense1Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_80:		brush.setStyle(Qt::Dense2Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_70:		brush.setStyle(Qt::Dense3Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_60:		brush.setStyle(Qt::Dense3Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_50:		brush.setStyle(Qt::Dense4Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_40:		brush.setStyle(Qt::Dense5Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_30:		brush.setStyle(Qt::Dense5Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_20:		brush.setStyle(Qt::Dense6Pattern); break;
	case SL_DC_BRUSH_STYLE_DENSE_10:		brush.setStyle(Qt::Dense7Pattern); break;
	case SL_DC_BRUSH_STYLE_H_LINES:			brush.setStyle(Qt::HorPattern); break;
	case SL_DC_BRUSH_STYLE_V_LINES:			brush.setStyle(Qt::VerPattern); break;
	case SL_DC_BRUSH_STYLE_CROSS_LINES:		brush.setStyle(Qt::CrossPattern); break;
	case SL_DC_BRUSH_STYLE_BD_LINES:		brush.setStyle(Qt::BDiagPattern); break;
	case SL_DC_BRUSH_STYLE_FD_LINES:		brush.setStyle(Qt::FDiagPattern); break;
	case SL_DC_BRUSH_STYLE_D_CROSS_LINES:	brush.setStyle(Qt::DiagCrossPattern); break;
	default:								brush.setStyle(Qt::SolidPattern); break;
	}
	if (brush != painter->brush())
		painter->setBrush(brush);
})


SL_DEFINE_DC_METHOD(clear, {
	QBrush brush(painter->brush());
	brush.setStyle(Qt::SolidPattern);
	painter->fillRect(painter->viewport(), brush);
})


SL_DEFINE_DC_METHOD(point, {
	QPointF pos;
	
	if (!PyArg_ParseTuple(args, "O&", convertPointF, &pos))
		return NULL;
	
	painter->drawPoint(pos);
})


SL_DEFINE_DC_METHOD(get_point, {
	PyErr_SetString(PyExc_RuntimeError, "cannot get point from device context");
	return NULL;
})


SL_DEFINE_DC_METHOD(line, {
	QPointF start, end;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPointF, &start, convertPointF, &end))
		return NULL;
	
	painter->drawLine(start, end);
})


SL_DEFINE_DC_METHOD(rect, {
	QPointF tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPointF, &tl, convertPointF, &br))
		return NULL;
	
	painter->drawRect(QRectF(tl, br));
})


SL_DEFINE_DC_METHOD(rounded_rect, {
	QPointF tl, br;
	double xr, yr;
	
	if (!PyArg_ParseTuple(args, "O&O&dd", convertPointF, &tl, convertPointF, &br, &xr, &yr))
		return NULL;
	
	painter->drawRoundedRect(QRectF(tl, br), xr, yr);
})


SL_DEFINE_DC_METHOD(ellipse, {
	QPointF pos;
	int a, b;
	
	if (!PyArg_ParseTuple(args, "O&ii", convertPointF, &pos, &a, &b))
		return NULL;
	
	painter->drawEllipse(pos, a, b);
})


SL_DEFINE_DC_METHOD(arc, {
	QPointF pos;
	int a, b;
	double start, end;
	
	if (!PyArg_ParseTuple(args, "O&iidd", convertPointF, &pos, &a, &b, &start, &end))
		return NULL;
	
	if (painter->brush().color() == Qt::transparent)
		painter->drawArc(pos.x() - a, pos.y() - b, a * 2, b * 2, start * 16.0, (end - start) * 16.0);
	else
		painter->drawPie(pos.x() - a, pos.y() - b, a * 2, b * 2, start * 16.0, (end - start) * 16.0);
})


SL_DEFINE_DC_METHOD(text, {
	QString text;
	QPointF tl, br;
	int flags;
	int qflags = 0;
	
	if (!PyArg_ParseTuple(args, "O&O&O&i", convertString, &text, convertPointF, &tl, convertPointF, &br, &flags))
		return NULL;
	
	if (flags == -1) {
		br = QPointF(1000000000, 1000000000);
		qflags = Qt::AlignTop | Qt::AlignLeft;
	}
	else {
		Qt::TextElideMode mode = Qt::ElideNone;
		switch (flags & 0xF00) {
		case SL_DC_TEXT_ELIDE_LEFT:		mode = Qt::ElideLeft; break;
		case SL_DC_TEXT_ELIDE_CENTER:	mode = Qt::ElideMiddle; break;
		case SL_DC_TEXT_ELIDE_RIGHT:	mode = Qt::ElideRight; break;
		default:						mode = Qt::ElideNone; qflags = Qt::TextWordWrap; break;
		}
		switch (flags & SL_ALIGN_HMASK) {
		case SL_ALIGN_LEFT:				qflags |= Qt::AlignLeft; break;
		case SL_ALIGN_HCENTER:			qflags |= Qt::AlignHCenter; break;
		case SL_ALIGN_RIGHT:			qflags |= Qt::AlignRight; break;
		case SL_ALIGN_JUSTIFY:			qflags |= Qt::AlignJustify; break;
		}
		switch (flags & SL_ALIGN_VMASK) {
		case SL_ALIGN_TOP:				qflags |= Qt::AlignTop; break;
		case SL_ALIGN_VCENTER:			qflags |= Qt::AlignVCenter; break;
		case SL_ALIGN_BOTTOM:			qflags |= Qt::AlignBottom; break;
		}
		text = QFontMetricsF(painter->fontMetrics()).elidedText(text, mode, br.x() - tl.x(), qflags);
	}
	painter->drawText(QRectF(tl, br), qflags, text);
})


#ifdef Q_OS_LINUX

SL_DEFINE_DC_METHOD(text_extent, {
	QString text;
	int maxWidth;
	
	if (!PyArg_ParseTuple(args, "O&i", convertString, &text, &maxWidth))
		return NULL;
	
	QSizeF size = QFontMetricsF(painter->fontMetrics()).boundingRect(QRect(0, 0, (maxWidth <= 0) ? 0 : maxWidth, 0), ((maxWidth <= 0) ? 0 : Qt::TextWordWrap), text).size();
	size.rwidth()--;
	return createVectorObject(size);
})

#else

SL_DEFINE_DC_METHOD(text_extent, {
	QString text;
	int maxWidth;
	
	if (!PyArg_ParseTuple(args, "O&i", convertString, &text, &maxWidth))
		return NULL;
	
	return createVectorObject(QFontMetricsF(painter->fontMetrics()).boundingRect(QRect(0, 0, (maxWidth <= 0) ? 0 : maxWidth, 0), ((maxWidth <= 0) ? 0 : Qt::TextWordWrap), text).size());
})

#endif


SL_DEFINE_DC_METHOD(blit, {
	PyErr_SetString(PyExc_RuntimeError, "cannot blit device context");
	return NULL;
})


SL_DEFINE_DC_METHOD(set_clipping, {
	QPointF tl, br;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertPointF, &tl, convertPointF, &br))
		return NULL;
	
	if ((tl.isNull()) && (br.isNull())) {
		painter->setClipping(false);
	}
	else {
		painter->setClipRect(QRectF(tl, br));
	}
})


SL_DEFINE_DC_METHOD(set_antialias, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	QPainter::RenderHints hints = painter->renderHints();
	
	if (enabled)
		hints |= (QPainter::Antialiasing | QPainter::TextAntialiasing);
	else
		hints &= ~(QPainter::Antialiasing | QPainter::TextAntialiasing);
	
	painter->setRenderHints(hints);
})


SL_DEFINE_DC_METHOD(get_transform, {
	PyObject *object = PyList_New(3);
	PyObject *r0 = PyList_New(3);
	PyObject *r1 = PyList_New(3);
	PyObject *r2 = PyList_New(3);
	
	QTransform matrix = painter->worldTransform();
	
	PyList_SET_ITEM(r0, 0, PyFloat_FromDouble(matrix.m11()));
	PyList_SET_ITEM(r0, 1, PyFloat_FromDouble(matrix.m12()));
	PyList_SET_ITEM(r0, 2, PyFloat_FromDouble(matrix.m13()));
	
	PyList_SET_ITEM(r1, 0, PyFloat_FromDouble(matrix.m21()));
	PyList_SET_ITEM(r1, 1, PyFloat_FromDouble(matrix.m22()));
	PyList_SET_ITEM(r1, 2, PyFloat_FromDouble(matrix.m23()));
	
	PyList_SET_ITEM(r2, 0, PyFloat_FromDouble(matrix.m31()));
	PyList_SET_ITEM(r2, 1, PyFloat_FromDouble(matrix.m32()));
	PyList_SET_ITEM(r2, 2, PyFloat_FromDouble(matrix.m33()));
	
	PyList_SET_ITEM(object, 0, r0);
	PyList_SET_ITEM(object, 1, r1);
	PyList_SET_ITEM(object, 2, r2);
	
	return object;
})


SL_DEFINE_DC_METHOD(set_transform, {
	PyObject *object, *seq, *r0 = NULL, *r1 = NULL, *r2 = NULL;
	qreal m11 = 1.0, m12 = 0.0, m13 = 0.0;
	qreal m21 = 0.0, m22 = 1.0, m23 = 0.0;
	qreal m31 = 0.0, m32 = 0.0, m33 = 1.0;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	if (object == Py_None) {
		painter->resetTransform();
		Py_RETURN_NONE;
	}
	
	seq = PySequence_Fast(object, "Expected sequence object");
	if (!seq)
		return NULL;
	do {
		if (PySequence_Fast_GET_SIZE(seq) != 3) {
			PyErr_SetString(PyExc_ValueError, "Sequence must have 3 elements");
			break;
		}
		object = PySequence_Fast_GET_ITEM(seq, 0);
		r0 = PySequence_Fast(object, "Expected sequence object");
		if (!r0)
			break;
		if (PySequence_Fast_GET_SIZE(r0) != 3) {
			PyErr_SetString(PyExc_ValueError, "Sequence must have 3 elements");
			break;
		}
		m11 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r0, 0));
		if (PyErr_Occurred())
			break;
		m12 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r0, 1));
		if (PyErr_Occurred())
			break;
		m13 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r0, 2));
		if (PyErr_Occurred())
			break;
		
		object = PySequence_Fast_GET_ITEM(seq, 1);
		r1 = PySequence_Fast(object, "Expected sequence object");
		if (!r1)
			break;
		if (PySequence_Fast_GET_SIZE(r1) != 3) {
			PyErr_SetString(PyExc_ValueError, "Sequence must have 3 elements");
			break;
		}
		m21 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r1, 0));
		if (PyErr_Occurred())
			break;
		m22 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r1, 1));
		if (PyErr_Occurred())
			break;
		m23 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r1, 2));
		if (PyErr_Occurred())
			break;
		
		object = PySequence_Fast_GET_ITEM(seq, 2);
		r2 = PySequence_Fast(object, "Expected sequence object");
		if (!r2)
			break;
		if (PySequence_Fast_GET_SIZE(r2) != 3) {
			PyErr_SetString(PyExc_ValueError, "Sequence must have 3 elements");
			break;
		}
		m31 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r2, 0));
		if (PyErr_Occurred())
			break;
		m32 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r2, 1));
		if (PyErr_Occurred())
			break;
		m33 = PyFloat_AsDouble(PySequence_Fast_GET_ITEM(r2, 2));
		if (PyErr_Occurred())
			break;
		
	} while (0);
	
	Py_DECREF(seq);
	Py_XDECREF(r0);
	Py_XDECREF(r1);
	Py_XDECREF(r2);
	
	if (PyErr_Occurred())
		return NULL;
	else
		painter->setWorldTransform(QTransform(m11, m12, m13, m21, m22, m23, m31, m32, m33));
})


SL_START_METHODS(DC)
SL_PROPERTY(color)
SL_PROPERTY(bgcolor)
SL_PROPERTY(font)
SL_PROPERTY(size)
SL_PROPERTY(pensize)
SL_PROPERTY(penstyle)
SL_PROPERTY(brushstyle)

SL_METHOD(clear)
SL_METHOD(point)
SL_METHOD(get_point)
SL_METHOD(line)
SL_METHOD(rect)
SL_METHOD(rounded_rect)
SL_METHOD(ellipse)
SL_METHOD(arc)
SL_METHOD(text)
SL_METHOD(text_extent)
SL_METHOD(blit)
SL_METHOD(set_clipping)
SL_METHOD(set_antialias)
SL_METHOD(get_transform)
SL_METHOD(set_transform)
SL_END_METHODS()


PyTypeObject DC_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"_slew.DC",									/* tp_name */
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
	"DC objects",								/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	DC::methods,								/* tp_methods */
};


bool
DC_type_setup(PyObject *module)
{
	if (PyType_Ready(&DC_Type) < 0)
		return false;
	Py_INCREF(&DC_Type);
	PyModule_AddObject(module, "DC", (PyObject *)&DC_Type);
	return true;
}


#include "gdi.moc"
