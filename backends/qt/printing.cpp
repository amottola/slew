#include "slew.h"
#include "objects.h"
#include "constants/gdi.h"

#include <QPrinter>
#include <QTransform>
#include <QDesktopWidget>
#include <QPageSetupDialog>
#include <QPrintPreviewDialog>
#include <QTemporaryFile>
#include <QFile>
#include <QPrintDialog>
#include <QEventLoop>

#ifndef Q_WS_MAC
#define DPI		96.0
#else
#define DPI		72.0
#endif


static const struct {
	int						fSL;
	QPrinter::PaperSource	fQT;
} kBin[] = {
	{	SL_PRINTER_BIN_DEFAULT,			QPrinter::Auto				},
	{	SL_PRINTER_BIN_ONLY_ONE,		QPrinter::OnlyOne			},
	{	SL_PRINTER_BIN_LOWER,			QPrinter::Lower				},
	{	SL_PRINTER_BIN_MIDDLE,			QPrinter::Middle			},
	{	SL_PRINTER_BIN_MANUAL,			QPrinter::Manual			},
	{	SL_PRINTER_BIN_ENVELOPE,		QPrinter::Envelope			},
	{	SL_PRINTER_BIN_ENVELOPE_MANUAL,	QPrinter::EnvelopeManual	},
	{	SL_PRINTER_BIN_TRACTOR,			QPrinter::Tractor			},
	{	SL_PRINTER_BIN_SMALL_FORMAT,	QPrinter::SmallFormat		},
	{	SL_PRINTER_BIN_LARGE_FORMAT,	QPrinter::LargeFormat		},
	{	SL_PRINTER_BIN_LARGE_CAPACITY,	QPrinter::LargeCapacity		},
	{	SL_PRINTER_BIN_CASSETTE,		QPrinter::Cassette			},
	{	SL_PRINTER_BIN_FORM_SOURCE,		QPrinter::FormSource		},
	{	-1,								QPrinter::Auto				},
};


static const struct {
	int						fSL;
	QPrinter::DuplexMode	fQT;
} kDuplex[] = {
	{	SL_PRINTER_DUPLEX_DEFAULT,		QPrinter::DuplexAuto		},
	{	SL_PRINTER_DUPLEX_SIMPLEX,		QPrinter::DuplexNone		},
	{	SL_PRINTER_DUPLEX_HORIZONTAL,	QPrinter::DuplexLongSide	},
	{	SL_PRINTER_DUPLEX_VERTICAL,		QPrinter::DuplexShortSide	},
	{	-1,								QPrinter::DuplexAuto		},
};


static const struct {
	int						fSL;
	QPrinter::PaperSize		fQT;
} kPaperSize[] = {
	{	SL_PRINTER_PAPER_A0,			QPrinter::A0			},
	{	SL_PRINTER_PAPER_A1,			QPrinter::A1			},
	{	SL_PRINTER_PAPER_A2,			QPrinter::A2			},
	{	SL_PRINTER_PAPER_A3,			QPrinter::A3			},
	{	SL_PRINTER_PAPER_A4,			QPrinter::A4			},
	{	SL_PRINTER_PAPER_A5,			QPrinter::A5			},
	{	SL_PRINTER_PAPER_A6,			QPrinter::A6			},
	{	SL_PRINTER_PAPER_A7,			QPrinter::A7			},
	{	SL_PRINTER_PAPER_A8,			QPrinter::A8			},
	{	SL_PRINTER_PAPER_A9,			QPrinter::A9			},
	{	SL_PRINTER_PAPER_B1,			QPrinter::B1			},
	{	SL_PRINTER_PAPER_B2,			QPrinter::B2			},
	{	SL_PRINTER_PAPER_B3,			QPrinter::B3			},
	{	SL_PRINTER_PAPER_B4,			QPrinter::B4			},
	{	SL_PRINTER_PAPER_B5,			QPrinter::B5			},
	{	SL_PRINTER_PAPER_B6,			QPrinter::B6			},
	{	SL_PRINTER_PAPER_B7,			QPrinter::B7			},
	{	SL_PRINTER_PAPER_B8,			QPrinter::B8			},
	{	SL_PRINTER_PAPER_B9,			QPrinter::B9			},
	{	SL_PRINTER_PAPER_B10,			QPrinter::B10			},
	{	SL_PRINTER_PAPER_C5E,			QPrinter::C5E			},
	{	SL_PRINTER_PAPER_COMM10E,		QPrinter::Comm10E		},
	{	SL_PRINTER_PAPER_DLE,			QPrinter::DLE			},
	{	SL_PRINTER_PAPER_EXECUTIVE,		QPrinter::Executive		},
	{	SL_PRINTER_PAPER_FOLIO,			QPrinter::Folio			},
	{	SL_PRINTER_PAPER_LEDGER,		QPrinter::Ledger		},
	{	SL_PRINTER_PAPER_LEGAL,			QPrinter::Legal			},
	{	SL_PRINTER_PAPER_LETTER,		QPrinter::Letter		},
	{	SL_PRINTER_PAPER_TABLOID,		QPrinter::Tabloid		},
	{	SL_PRINTER_PAPER_CUSTOM,		QPrinter::Custom		},
};


SL_DEFINE_DC_METHOD(get_size, {
	QPrinter *printer = (QPrinter *)device;
	QSizeF paperSize = printer->paperSize(QPrinter::DevicePixel);
	qreal w, h;
	
// 	if (((paperSize.width() > paperSize.height()) && (printer->orientation() == QPrinter::Portrait)) ||
// 		((paperSize.width() < paperSize.height()) && (printer->orientation() == QPrinter::Landscape)))
// 		paperSize = QSizeF(paperSize.height(), paperSize.width());
	
	painter->worldTransform().inverted().map(paperSize.width(), paperSize.height(), &w, &h);
	
	return createVectorObject(QSize(w, h));
})


SL_DEFINE_DC_METHOD(set_size, {
	QPrinter *printer = (QPrinter *)device;
	QTransform transform;
	QSizeF paperSize = printer->paperSize(QPrinter::DevicePixel);
	QSize size;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
// 	if (((paperSize.width() > paperSize.height()) && (printer->orientation() == QPrinter::Portrait)) ||
// 		((paperSize.width() < paperSize.height()) && (printer->orientation() == QPrinter::Landscape)))
// 		paperSize = QSizeF(paperSize.height(), paperSize.width());
	
	qreal sx = paperSize.width() / size.width();
	qreal sy = paperSize.height() / size.height();
	
	if (sx < sy)
		transform.scale(sx, sx);
	else
		transform.scale(sy, sy);
	painter->setWorldTransform(transform);
})


SL_DEFINE_DC_METHOD(get_dpi, {
	QPrinter *printer = (QPrinter *)device;
	qreal w, h;
	
	painter->worldTransform().inverted().map(printer->logicalDpiX(), printer->logicalDpiY(), &w, &h);
	
	return createVectorObject(QSize(w, h));
})


SL_DEFINE_DC_METHOD(set_dpi, {
	QPrinter *printer = (QPrinter *)device;
	QTransform transform;
	QSize dpi;
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &dpi))
		return NULL;
	
	qreal sx = printer->logicalDpiX() / dpi.width();
	qreal sy = printer->logicalDpiY() / dpi.height();
	
	if (sx < sy)
		transform.scale(sx, sx);
	else
		transform.scale(sy, sy);
	painter->setWorldTransform(transform);
})


SL_DEFINE_DC_METHOD(clear, {
	PyErr_SetString(PyExc_RuntimeError, "cannot clear device context");
	return NULL;
})


SL_DEFINE_DC_METHOD(text, {
	QPrinter *printer = (QPrinter *)device;
	const QFontMetrics& fm = painter->fontMetrics();
	QString text;
	QPointF tl, br;
	int flags;
	
	if (!PyArg_ParseTuple(args, "O&O&O&i", convertString, &text, convertPointF, &tl, convertPointF, &br, &flags))
		return NULL;
	
	QTransform transform = painter->worldTransform();
	transform.translate((qreal)tl.x() + 0.5, (qreal)tl.y() + 0.5);
	transform.scale(DPI / printer->logicalDpiX(), DPI / printer->logicalDpiY());
	painter->save();
	painter->setWorldTransform(transform);
	
	if (flags == -1) {
		painter->drawText(0, fm.ascent(), text);
	}
	else {
		int qflags = 0;
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
		QRectF rect(0, 0, (br.x() - tl.x()) * printer->logicalDpiX() / DPI, (br.y() - tl.y()) * printer->logicalDpiY() / DPI);
		QString elided = fm.elidedText(text, mode, rect.width(), 0);
		painter->drawText(rect, qflags, elided);
	}
	painter->restore();
})


SL_DEFINE_DC_METHOD(text_extent, {
	QPrinter *printer = (QPrinter *)device;
	QString text;
	int maxWidth;
	
	if (!PyArg_ParseTuple(args, "O&i", convertString, &text, &maxWidth))
		return NULL;
	
	QSizeF size = painter->fontMetrics().boundingRect(QRect(0, 0, (maxWidth <= 0) ? 0 : maxWidth, 0), ((maxWidth <= 0) ? 0 : Qt::TextWordWrap) | Qt::TextLongestVariant, text).size();
	return createVectorObject(QSize(size.width() * DPI / printer->logicalDpiX(), size.height() * DPI / printer->logicalDpiY()));
})


SL_DEFINE_DC_METHOD(get_page_rect, {
	QPrinter *printer = (QPrinter *)device;
	QTransform transform = painter->worldTransform().inverted();
	QRectF pageRect = transform.mapRect(printer->pageRect(QPrinter::DevicePixel));
	QRectF paperRect = transform.mapRect(printer->paperRect(QPrinter::DevicePixel));
	
	pageRect.translate(-paperRect.x(), -paperRect.y());
	
	PyObject *tuple = PyTuple_New(2);
	PyTuple_SET_ITEM(tuple, 0, createVectorObject(pageRect.topLeft().toPoint()));
	PyTuple_SET_ITEM(tuple, 1, createVectorObject(pageRect.bottomRight().toPoint()));
	return tuple;
})


SL_DEFINE_DC_METHOD(get_printer_dpi, {
	QPrinter *printer = (QPrinter *)device;
	return createVectorObject(QSize(printer->logicalDpiX(), printer->logicalDpiY()));
})


SL_START_METHODS(PrintDC)
SL_PROPERTY(size)
SL_PROPERTY(dpi)

SL_METHOD(clear)
SL_METHOD(text)
SL_METHOD(text_extent)
SL_METHOD(get_page_rect)
SL_METHOD(get_printer_dpi)
SL_END_METHODS()


PyTypeObject PrintDC_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"_slew.PrintDC",							/* tp_name */
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
	"PrintDC objects",							/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	PrintDC::methods,							/* tp_methods */
	0,											/* tp_members */
	0,											/* tp_getset */
	&DC_Type,									/* tp_base */
};


bool
PrintDC_type_setup(PyObject *module)
{
	if (PyType_Ready(&PrintDC_Type) < 0)
		return false;
	Py_INCREF(&PrintDC_Type);
	PyModule_AddObject(module, "PrintDC", (PyObject *)&PrintDC_Type);
	return true;
}



class PrintHandler : public QObject
{
	Q_OBJECT
	
public:
	PrintHandler(PyObject *callback) : fCallback(callback), fAborted(false) { Py_XINCREF(callback); }
	~PrintHandler() { Py_XDECREF(fCallback); }
	
	bool aborted() { return fAborted; }
	
public slots:
	void print(QPrinter *printer)
	{
		QPainter painter(printer);
		int page, fromPage, toPage;
		DC_Proxy *dc = NULL;
		PyObject *iterator = NULL;
		
		if (!painter.isActive()) {
			printer->abort();
			fAborted = true;
			return;
		}
		
		painter.setViewTransformEnabled(true);
		painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform | QPainter::NonCosmeticDefaultPen);
		
		fromPage = printer->fromPage();
		toPage = printer->toPage();
		if ((fromPage == 0) || (toPage == 0)) {
			fromPage = 1;
			toPage = 0x7FFFFFFF;
		}
		
		PyAutoLocker locker;
		
		do {
			PyObject *result, *from, *to;
			
			dc = (DC_Proxy *)createDCObject(&painter, PyPrintDC_Type, (PyObject *)&PrintDC_Type, printer);
			if (!dc)
				break;
			from = PyInt_FromLong(fromPage);
			to = PyInt_FromLong(toPage);
			result = PyObject_CallFunctionObjArgs(fCallback, dc, from, to, NULL);
			Py_DECREF(from);
			Py_DECREF(to);
			if (!result)
				break;
			if (!PyGen_Check(result)) {
				Py_DECREF(result);
				PyErr_SetString(PyExc_ValueError, "expected Generator object");
				break;
			}
			iterator = PyObject_GetIter(result);
			Py_DECREF(result);
			if (!iterator)
				break;
			
			for (page = fromPage; page <= toPage; page++) {
				painter.save();
				result = PyIter_Next(iterator);
				Py_XDECREF(result);
				painter.restore();

				if ((!result) || (result == Py_True)) {
					fAborted = true;
					break;
				}
				else if (result == Py_None) {
					page = toPage;
				}
				
				if (page != toPage)
					printer->newPage();
			}
		} while (0);
		
		Py_XDECREF(dc);
		Py_XDECREF(iterator);
		
		if (PyErr_Occurred()) {
			// The following call causes a crash in Qt 4.5.2 - 4.7.0
			printer->abort();
			fAborted = true;
			PyErr_Print();
			PyErr_Clear();
		}
	}
	
private:
	PyObject		*fCallback;
	bool			fAborted;
};


class PageSetupDialog : public QPageSetupDialog
{
public:
	PageSetupDialog(QPrinter *printer, QWidget *parent)
		: QPageSetupDialog(printer, parent), fEventLoop(NULL)
	{
#ifdef Q_WS_MAC
		if (parent) {
			setWindowModality(Qt::WindowModal);
			fEventLoop = new QEventLoop();
			open();
			setAttribute(Qt::WA_ShowModal, true);
		}
#endif
	}
	
	virtual ~PageSetupDialog()
	{
#ifdef Q_WS_MAC
		delete fEventLoop;
#endif
	}
	
	bool run()
	{
		Py_BEGIN_ALLOW_THREADS
	
#ifdef Q_WS_MAC
		if (fEventLoop)
			fEventLoop->exec(QEventLoop::DialogExec);
		else
#endif
		exec();

		Py_END_ALLOW_THREADS
		
		return result() == QDialog::Accepted;
	}
	
	virtual void setVisible(bool visible)
	{
#ifdef Q_WS_MAC
		if ((!visible) && (fEventLoop))
			fEventLoop->exit();
#endif
		QPageSetupDialog::setVisible(visible);
	}
	
private:
	QEventLoop		*fEventLoop;
};



class PrintDialog : public QPrintDialog
{
public:
	PrintDialog(QPrinter *printer, QWidget *parent)
		: QPrintDialog(printer, parent), fEventLoop(NULL)
	{
#ifdef Q_WS_MAC
		if (parent) {
			setWindowModality(Qt::WindowModal);
			fEventLoop = new QEventLoop();
			open();
			setAttribute(Qt::WA_ShowModal, true);
		}
#endif
	}
	
	virtual ~PrintDialog()
	{
#ifdef Q_WS_MAC
		delete fEventLoop;
#endif
	}
	
	int run()
	{
		setResult(0);
		
		Py_BEGIN_ALLOW_THREADS
		
#ifdef Q_WS_MAC
		if (fEventLoop)
			fEventLoop->exec(QEventLoop::DialogExec);
		else
#endif
		setResult(exec());
		
		Py_END_ALLOW_THREADS
		
		return result();
	}
	
	virtual void setVisible(bool visible)
	{
#ifdef Q_WS_MAC
		if ((!visible) && (fEventLoop))
			fEventLoop->exit();
#endif
		QPrintDialog::setVisible(visible);
	}
	
private:
	QEventLoop		*fEventLoop;
};


static bool
loadSettings(PyObject *settings, QPrinter *printer)
{
	QPrinter defaultPrinter;
	int i, bin, duplex, orientation, quality;
	qreal top, right, bottom, left;
	bool color, collate;
	QString name, creator;
	PyObject *paper, *margin;
	int paperType = SL_PRINTER_PAPER_A4;
	QSizeF paperSize;
	
	if ((!getObjectAttr(settings, "bin", &bin)) ||
		(!getObjectAttr(settings, "color", &color)) ||
		(!getObjectAttr(settings, "duplex", &duplex)) ||
		(!getObjectAttr(settings, "orientation", &orientation)) ||
		(!getObjectAttr(settings, "name", &name)) ||
		(!getObjectAttr(settings, "quality", &quality)) ||
		(!getObjectAttr(settings, "collate", &collate)) ||
		(!getObjectAttr(settings, "creator", &creator)))
		return false;
	
	paper = PyObject_GetAttrString(settings, "paper");
	if (!paper)
		return false;
	if (paper != Py_None) {
		if ((!getObjectAttr(paper, "type", &paperType)) ||
			(!getObjectAttr(paper, "size", &paperSize))) {
			Py_DECREF(paper);
			return false;
		}
	}
	Py_DECREF(paper);
	
	if (name.isEmpty()) {
		name = defaultPrinter.printerName();
	}
	if (!name.trimmed().isEmpty())
		printer->setPrinterName(name);
	
#ifdef Q_WS_WIN
	for (i = 0; kBin[i].fSL >= 0; i++) {
		if (kBin[i].fSL == bin) {
			QPrinter::PaperSource source = kBin[i].fQT;
			if (printer->supportedPaperSources().contains(source)) {
				printer->setPaperSource(source);
				break;
			}
		}
	}
#endif
	
	printer->setColorMode(color ? QPrinter::Color : QPrinter::GrayScale);
	printer->setCollateCopies(collate);
	
	for (i = 0; kDuplex[i].fSL >= 0; i++) {
		if (kDuplex[i].fSL == duplex) {
			printer->setDuplex(kDuplex[i].fQT);
			break;
		}
	}
	if (kDuplex[i].fSL == -1)
		printer->setDuplex(QPrinter::DuplexAuto);
	
	QList<int> resolutions = printer->supportedResolutions();
	qSort(resolutions);
	int dpi = resolutions.value((qMin(SL_PRINTER_QUALITY_HIGH, quality) * (resolutions.count() - 1)) / SL_PRINTER_QUALITY_HIGH);
	if (dpi == 0)
		dpi = 300;
	printer->setResolution(dpi);
	
	for (i = 0; kPaperSize[i].fSL != SL_PRINTER_PAPER_CUSTOM; i++) {
		if (kPaperSize[i].fSL == paperType)
			break;
	}
	if (kPaperSize[i].fSL == SL_PRINTER_PAPER_CUSTOM) {
#ifdef Q_WS_WIN
		if (((orientation == SL_PRINTER_ORIENTATION_VERTICAL) && (paperSize.height() < paperSize.width())) ||
			((orientation == SL_PRINTER_ORIENTATION_HORIZONTAL) && (paperSize.height() > paperSize.width()))) {
			paperSize = QSizeF(paperSize.height(), paperSize.width());
		}
#endif
		printer->setPaperSize(paperSize / 10.0, QPrinter::Millimeter);
//		qDebug() << "Custom paper size:" << paperSize;
	}
	else {
		printer->setPaperSize(kPaperSize[i].fQT);
//		qDebug() << "Predefined paper size:" << kPaperSize[i].fSL;
	}
	
	printer->setOrientation(orientation == SL_PRINTER_ORIENTATION_VERTICAL ? QPrinter::Portrait : QPrinter::Landscape);
	printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	
	margin = PyObject_GetAttrString(settings, "margin_top");
	if (!margin)
		return false;
	if (margin != Py_None)
		top = (qreal)PyInt_AsLong(margin) / 10.0;
	Py_DECREF(margin);
	if (PyErr_Occurred())
		return false;

	margin = PyObject_GetAttrString(settings, "margin_right");
	if (!margin)
		return false;
	if (margin != Py_None)
		right = (qreal)PyInt_AsLong(margin) / 10.0;
	Py_DECREF(margin);
	if (PyErr_Occurred())
		return false;

	margin = PyObject_GetAttrString(settings, "margin_bottom");
	if (!margin)
		return false;
	if (margin != Py_None)
		bottom = (qreal)PyInt_AsLong(margin) / 10.0;
	Py_DECREF(margin);
	if (PyErr_Occurred())
		return false;
	
	margin = PyObject_GetAttrString(settings, "margin_left");
	if (!margin)
		return false;
	if (margin != Py_None)
		left = (qreal)PyInt_AsLong(margin) / 10.0;
	Py_DECREF(margin);
	if (PyErr_Occurred())
		return false;
	
	printer->setPageMargins(left, top, right, bottom, QPrinter::Millimeter);
	printer->setCreator(creator);
	return true;
}


static bool
saveSettings(PyObject *settings, QPrinter *printer)
{
	PyObject *value;
	int i;
	bool valid = false;
	
	do {
		value = createStringObject(printer->printerName());
		PyObject_SetAttrString(settings, "name", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		for (i = 0; kBin[i].fSL >= 0; i++) {
			if (kBin[i].fQT == printer->paperSource()) {
				value = PyInt_FromLong(kBin[i].fSL);
				PyObject_SetAttrString(settings, "bin", value);
				Py_DECREF(value);
				break;
			}
		}
		if (PyErr_Occurred())
			break;
		
		value = createBoolObject(printer->colorMode() == QPrinter::Color);
		PyObject_SetAttrString(settings, "color", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		value = createBoolObject(printer->collateCopies());
		PyObject_SetAttrString(settings, "collate", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		for (i = 0; kDuplex[i].fSL >= 0; i++) {
			if (kDuplex[i].fQT == printer->duplex()) {
				value = PyInt_FromLong(kDuplex[i].fSL);
				PyObject_SetAttrString(settings, "duplex", value);
				Py_DECREF(value);
				break;
			}
		}
		if (PyErr_Occurred())
			break;
		
		value = PyInt_FromLong(printer->orientation() == QPrinter::Portrait ? SL_PRINTER_ORIENTATION_VERTICAL : SL_PRINTER_ORIENTATION_HORIZONTAL);
		PyObject_SetAttrString(settings, "orientation", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		QSizeF size = printer->paperSize(QPrinter::Millimeter);
		PyObject *paperSize = createVectorObject(QSize(size.width() * 10, size.height() * 10));
		value = PyObject_CallFunction(PyPaper_Type, "OO", Py_None, paperSize);
		Py_DECREF(paperSize);
		if (!value)
			break;
		PyObject_SetAttrString(settings, "paper", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		QList<int> resolutions = printer->supportedResolutions();
		qSort(resolutions);
		for (i = 0; i < resolutions.count(); i++) {
			if (resolutions[i] == printer->resolution())
				break;
		}
		if ((resolutions.count() > 1) && (i != resolutions.count()))
			value = PyInt_FromLong((i * SL_PRINTER_QUALITY_HIGH) / (resolutions.count() - 1));
		else
			value = PyInt_FromLong(SL_PRINTER_QUALITY_HIGH);
		PyObject_SetAttrString(settings, "quality", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
#ifndef Q_WS_MAC
		qreal top, right, bottom, left;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
		value = PyInt_FromLong((int)(left * 10.0));
		PyObject_SetAttrString(settings, "margin_left", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		value = PyInt_FromLong((int)(top * 10.0));
		PyObject_SetAttrString(settings, "margin_top", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		value = PyInt_FromLong((int)(right * 10.0));
		PyObject_SetAttrString(settings, "margin_right", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		value = PyInt_FromLong((int)(bottom * 10.0));
		PyObject_SetAttrString(settings, "margin_bottom", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
#endif
		
		valid = true;
	} while (0);
	
	return valid;
}


bool
pageSetup(PyObject *settings, PyObject *parent, bool *accepted)
{
	QWidget *parentWidget;
	QPrinter printer(QPrinter::HighResolution);
	
	if (parent == Py_None) {
		parentWidget = NULL;
	}
	else if (isFrame(parent)) {
		parentWidget = (QWidget *)getImpl(parent);
		if (!parentWidget)
			SL_RETURN_NO_IMPL;
	}
	else {
		PyErr_SetString(PyExc_ValueError, "expected Frame object or None");
		return false;
	}
	
	if (!loadSettings(settings, &printer))
		return false;
	
	Py_INCREF(settings);
	
	PageSetupDialog dialog(&printer, parentWidget);
	*accepted = dialog.run();
	
	bool valid = saveSettings(settings, &printer);
	
	Py_DECREF(settings);
	return valid;
}


PyObject *
printDocument(int type, const QString& title, PyObject *callback, bool prompt, PyObject *settings, PyObject *parent, QObject *handler)
{
	QPrinter printer(QPrinter::HighResolution);
	QWidget *parentWidget;
	
	if (type == SL_PRINT_PDF)
		printer.setOutputFormat(QPrinter::PdfFormat);
	
	if ((settings != Py_None) && (!loadSettings(settings, &printer)))
		return NULL;
	
	if (parent == Py_None) {
		parentWidget = NULL;
	}
	else if (isFrame(parent)) {
		parentWidget = (QWidget *)getImpl(parent);
		if (!parentWidget)
			SL_RETURN_NO_IMPL;
	}
	else {
		PyErr_SetString(PyExc_ValueError, "expected Frame object or None");
		return NULL;
	}
	
	PrintHandler standard_handler(callback);
	QString docName = title;
	
	if (!handler)
		handler = &standard_handler;
	
	if (docName.isEmpty())
		docName = "Untitled document";
	
	printer.setDocName(docName);
	printer.setFullPage(true);
	
	switch (type) {
	case SL_PRINT_PREVIEW:
		{
			QPrintPreviewDialog dialog(&printer, parentWidget);
			dialog.setWindowTitle(docName);
			if (parentWidget)
				dialog.setWindowModality(Qt::WindowModal);
			
			QObject::connect(&dialog, SIGNAL(paintRequested(QPrinter *)), handler, SLOT(print(QPrinter *)));
			
			Py_BEGIN_ALLOW_THREADS
			
			dialog.exec();
			
			Py_END_ALLOW_THREADS
		}
		break;
	
	case SL_PRINT_PDF:
		{
			QString fileName;
			{
				QTemporaryFile tempFile;
				if (tempFile.open())
					fileName = tempFile.fileName();
				else {
					PyErr_SetString(PyExc_RuntimeError, "cannot create temporary file for PDF output");
					return NULL;
				}
			}
			
			printer.setOutputFileName(fileName);
			
			QMetaObject::invokeMethod(handler, "print", Qt::DirectConnection, Q_ARG(QPrinter *, &printer));
			
			QFile file(fileName);
			if (!file.open(QIODevice::ReadOnly)) {
				PyErr_SetString(PyExc_RuntimeError, "cannot open temporary file holding PDF output");
				return NULL;
			}
			
			QByteArray output(file.readAll());
			file.close();
			QFile::remove(fileName);
			
			return createBufferObject(output);
		}
		break;
	
	default:
		{
			if (prompt) {
				Py_INCREF(settings);
				PrintDialog dialog(&printer, parentWidget);
				if (dialog.run() == QDialog::Rejected)
					Py_RETURN_FALSE;
				printer.setFullPage(true);
				bool valid = (settings == Py_None) || saveSettings(settings, &printer);
				Py_DECREF(settings);
				if (!valid)
					return NULL;
			}
			printer.setPageMargins(0, 0, 0, 0, QPrinter::Millimeter);
			QMetaObject::invokeMethod(handler, "print", Qt::DirectConnection, Q_ARG(QPrinter *, &printer));
			
			if (handler == &standard_handler)
				return createBoolObject(!standard_handler.aborted());
			Py_RETURN_TRUE;
		}
		break;
	}
	
	Py_RETURN_NONE;
}



#include "printing.moc"
