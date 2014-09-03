#include "slew.h"
#include "objects.h"
#include "constants/gdi.h"

#include <QPrinter>
#include <QTransform>
#include <QStaticText>
#include <QDesktopWidget>
#include <QPageSetupDialog>
#include <QPrintPreviewDialog>
#include <QTemporaryFile>
#include <QFile>
#include <QPrintDialog>
#include <QEventLoop>
#include <QCache>

#ifndef Q_OS_MAC
#define DPI		96.0
#else
#define DPI		72.0
#endif


static QCache<QString, QStaticText> sTextCache(10000);
static QPrinter *sPrinter = NULL;


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
	int						fWidth;
	int						fHeight;
} kPaperSize[] = {
	{	SL_PRINTER_PAPER_A0,			QPrinter::A0,			8410,	11890	},
	{	SL_PRINTER_PAPER_A1,			QPrinter::A1,			5940,	8410	},
	{	SL_PRINTER_PAPER_A2,			QPrinter::A2,			4200,	5940	},
	{	SL_PRINTER_PAPER_A3,			QPrinter::A3,			2970,	4200	},
	{	SL_PRINTER_PAPER_A4,			QPrinter::A4,			2100,	2970	},
	{	SL_PRINTER_PAPER_A5,			QPrinter::A5,			1480,	2100	},
	{	SL_PRINTER_PAPER_A6,			QPrinter::A6,			1050,	1480	},
	{	SL_PRINTER_PAPER_A7,			QPrinter::A7,			740,	1050	},
	{	SL_PRINTER_PAPER_A8,			QPrinter::A8,			520,	740		},
	{	SL_PRINTER_PAPER_A9,			QPrinter::A9,			370,	520		},
	{	SL_PRINTER_PAPER_B1,			QPrinter::B1,			7070,	10000	},
	{	SL_PRINTER_PAPER_B2,			QPrinter::B2,			5000,	7070	},
	{	SL_PRINTER_PAPER_B3,			QPrinter::B3,			3530,	5000	},
	{	SL_PRINTER_PAPER_B4,			QPrinter::B4,			2500,	3530	},
	{	SL_PRINTER_PAPER_B5,			QPrinter::B5,			1760,	2500	},
	{	SL_PRINTER_PAPER_B6,			QPrinter::B6,			1250,	1760	},
	{	SL_PRINTER_PAPER_B7,			QPrinter::B7,			880,	1250	},
	{	SL_PRINTER_PAPER_B8,			QPrinter::B8,			620,	880		},
	{	SL_PRINTER_PAPER_B9,			QPrinter::B9,			330,	620		},
	{	SL_PRINTER_PAPER_B10,			QPrinter::B10,			310,	440		},
	{	SL_PRINTER_PAPER_C5E,			QPrinter::C5E,			1630,	2290	},
	{	SL_PRINTER_PAPER_COMM10E,		QPrinter::Comm10E,		1050,	2410	},
	{	SL_PRINTER_PAPER_DLE,			QPrinter::DLE,			1100,	2200	},
	{	SL_PRINTER_PAPER_EXECUTIVE,		QPrinter::Executive,	1905,	2540	},
	{	SL_PRINTER_PAPER_FOLIO,			QPrinter::Folio,		2100,	3300	},
	{	SL_PRINTER_PAPER_LEDGER,		QPrinter::Ledger,		4318,	2794	},
	{	SL_PRINTER_PAPER_LEGAL,			QPrinter::Legal,		2159,	3556	},
	{	SL_PRINTER_PAPER_LETTER,		QPrinter::Letter,		2159,	2794	},
	{	SL_PRINTER_PAPER_TABLOID,		QPrinter::Tabloid,		2794,	4318	},
	{	SL_PRINTER_PAPER_CUSTOM,		QPrinter::Custom,		0,		0		},
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
	*self->fBaseTransform = transform;
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
	*self->fBaseTransform = transform;
})


SL_DEFINE_DC_METHOD(clear, {
	PyErr_SetString(PyExc_RuntimeError, "cannot clear device context");
	return NULL;
})


SL_DEFINE_DC_METHOD(text, {
	QPrinter *printer = (QPrinter *)device;
	QString key, text;
	QPointF tl, br;
	int flags, qflags = 0;
	
	if (!PyArg_ParseTuple(args, "O&O&O&i", convertString, &text, convertPointF, &tl, convertPointF, &br, &flags))
		return NULL;
	
	QTransform transform, orig_transform = painter->worldTransform();
	QTransform user_transform = orig_transform * baseTransform->inverted();
	
	transform = *baseTransform;
// 	qDebug() << transform << user_transform << *baseTransform;
	transform.translate((qreal)(tl.x() + br.x()) / 2.0, (qreal)(tl.y() + br.y()) / 2.0);
	transform.scale(DPI / printer->logicalDpiX(), DPI / printer->logicalDpiY());
	painter->setWorldTransform(transform, false);
	painter->setWorldTransform(user_transform, true);
	
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
		br = QPoint((br.x() - tl.x()) * printer->logicalDpiX() / DPI, (br.y() - tl.y()) * printer->logicalDpiY() / DPI);
		text = QFontMetricsF(painter->fontMetrics()).elidedText(text, mode, br.x(), qflags);
	}
	painter->drawText(QRectF(-QPointF(br) / 2.0, QPointF(br) / 2.0), qflags, text);
// 	painter->setPen(Qt::red);
// 	painter->drawRect(QRectF(-QPointF(br) / 2.0, QPointF(br) / 2.0));
	painter->setWorldTransform(orig_transform);
})


SL_DEFINE_DC_METHOD(text_extent, {
	QPrinter *printer = (QPrinter *)device;
	QString text;
	int maxWidth;
	
	if (!PyArg_ParseTuple(args, "O&i", convertString, &text, &maxWidth))
		return NULL;
	
	QSizeF size = QFontMetricsF(painter->fontMetrics()).boundingRect(QRect(0, 0, (maxWidth <= 0) ? 0 : maxWidth, 0), ((maxWidth <= 0) ? 0 : Qt::TextWordWrap), text).size();
	size.rwidth()--;
	
	size.rwidth() *= DPI / printer->logicalDpiX();
	size.rheight() *= DPI / printer->logicalDpiY();
	
	return createVectorObject(size);
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
	"slew._slew.PrintDC",						/* tp_name */
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
			QTransform baseTransform;
			
			dc = (DC_Proxy *)createDCObject(&painter, PyPrintDC_Type, (PyObject *)&PrintDC_Type, printer, &baseTransform);
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
#ifdef Q_OS_MAC
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
#ifdef Q_OS_MAC
		delete fEventLoop;
#endif
	}
	
	bool run()
	{
		Py_BEGIN_ALLOW_THREADS
	
#ifdef Q_OS_MAC
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
#ifdef Q_OS_MAC
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
#ifdef Q_OS_MAC
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
#ifdef Q_OS_MAC
		delete fEventLoop;
#endif
	}
	
	int run()
	{
		setResult(0);
		
		Py_BEGIN_ALLOW_THREADS
		
#ifdef Q_OS_MAC
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
#ifdef Q_OS_MAC
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
	
#ifdef Q_OS_WIN
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
		if ((kPaperSize[i].fSL == paperType) ||
			((kPaperSize[i].fWidth == paperSize.width()) && (kPaperSize[i].fHeight == paperSize.height())) ||
			((kPaperSize[i].fWidth == paperSize.height()) && (kPaperSize[i].fHeight == paperSize.width())))
			break;
	}
	if (kPaperSize[i].fSL == SL_PRINTER_PAPER_CUSTOM) {
#ifdef Q_OS_WIN
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
		
		qreal top, right, bottom, left;
		printer->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
#ifdef Q_OS_MAC
		if (printer->paperSize() != QPrinter::Custom) {
			value = Py_None;
			Py_INCREF(value);
		}
		else
#endif
		value = PyInt_FromLong((int)(left * 10.0));
		PyObject_SetAttrString(settings, "margin_left", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
#ifdef Q_OS_MAC
		if (printer->paperSize() != QPrinter::Custom) {
			value = Py_None;
			Py_INCREF(value);
		}
		else
#endif
		value = PyInt_FromLong((int)(top * 10.0));
		PyObject_SetAttrString(settings, "margin_top", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
#ifdef Q_OS_MAC
		if (printer->paperSize() != QPrinter::Custom) {
			value = Py_None;
			Py_INCREF(value);
		}
		else
#endif
		value = PyInt_FromLong((int)(right * 10.0));
		PyObject_SetAttrString(settings, "margin_right", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
#ifdef Q_OS_MAC
		if (printer->paperSize() != QPrinter::Custom) {
			value = Py_None;
			Py_INCREF(value);
		}
		else
#endif
		value = PyInt_FromLong((int)(bottom * 10.0));
		PyObject_SetAttrString(settings, "margin_bottom", value);
		Py_DECREF(value);
		if (PyErr_Occurred())
			break;
		
		valid = true;
	} while (0);
	
	return valid;
}


bool
pageSetup(PyObject *settings, PyObject *parent, bool *accepted)
{
	QWidget *parentWidget;
	
	*accepted = false;
	
	if (parent == Py_None) {
		parentWidget = NULL;
	}
	else if (isFrame(parent)) {
		parentWidget = (QWidget *)getImpl(parent);
		if (!parentWidget) {
			PyErr_SetString(PyExc_RuntimeError, "object has no attached implementation");
			return false;
		}
	}
	else {
		PyErr_SetString(PyExc_ValueError, "expected Frame object or None");
		return false;
	}
	
	if (sPrinter)
		delete sPrinter;
	sPrinter = new QPrinter(QPrinter::HighResolution);
	if (!loadSettings(settings, sPrinter))
		return false;
	
	Py_INCREF(settings);
	
	PageSetupDialog dialog(sPrinter, parentWidget);
	*accepted = dialog.run();
	
	bool valid = (!(*accepted)) || saveSettings(settings, sPrinter);
	
	Py_DECREF(settings);
	return valid;
}


bool
printSetup(PyObject *settings, PyObject *parent, bool *accepted)
{
	QWidget *parentWidget;
	
	*accepted = false;
	
	if (parent == Py_None) {
		parentWidget = NULL;
	}
	else if (isFrame(parent)) {
		parentWidget = (QWidget *)getImpl(parent);
		if (!parentWidget) {
			PyErr_SetString(PyExc_RuntimeError, "object has no attached implementation");
			return false;
		}
	}
	else {
		PyErr_SetString(PyExc_ValueError, "expected Frame object or None");
		return false;
	}
	
	Py_INCREF(settings);
	if (sPrinter)
		delete sPrinter;
	sPrinter = new QPrinter(QPrinter::HighResolution);
	if (!loadSettings(settings, sPrinter))
		return false;
	
	PrintDialog dialog(sPrinter, parentWidget);
	*accepted = (dialog.run() == QDialog::Accepted);
	
	bool valid = (!(*accepted)) || (settings == Py_None) || saveSettings(settings, sPrinter);
	
	Py_DECREF(settings);
	return valid;
}


PyObject *
printDocument(int type, const QString& title, PyObject *callback, bool prompt, PyObject *settings, PyObject *parent, QObject *handler)
{
	QWidget *parentWidget;
	PyObject *result = NULL;
	
	if (!sPrinter)
		sPrinter = new QPrinter(QPrinter::HighResolution);
	if (type == SL_PRINT_PDF)
		sPrinter->setOutputFormat(QPrinter::PdfFormat);
	else if (type == SL_PRINT_PS)
		sPrinter->setOutputFormat(QPrinter::PostScriptFormat);
	
	if ((settings != Py_None) && (!loadSettings(settings, sPrinter)))
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
	
	sPrinter->setDocName(docName);
	sPrinter->setFullPage(true);
	
	switch (type) {
	case SL_PRINT_PREVIEW:
		{
			QPrintPreviewDialog dialog(sPrinter, parentWidget);
			dialog.setWindowTitle(docName);
			if (parentWidget)
				dialog.setWindowModality(Qt::WindowModal);
			
			QObject::connect(&dialog, SIGNAL(paintRequested(QPrinter *)), handler, SLOT(print(QPrinter *)));
			
			Py_BEGIN_ALLOW_THREADS
			
			dialog.exec();
			
			Py_END_ALLOW_THREADS
			
			result = Py_None;
			Py_INCREF(result);
		}
		break;
	
	case SL_PRINT_PDF:
	case SL_PRINT_PS:
		{
			QString fileName;
			QTemporaryFile tempFile;
			tempFile.setAutoRemove(false);
			if (tempFile.open())
				fileName = tempFile.fileName();
			else {
				PyErr_SetString(PyExc_RuntimeError, "cannot create temporary file for PDF output");
				break;
			}
			tempFile.close();
			
			sPrinter->setOutputFileName(fileName);
			sPrinter->setFontEmbeddingEnabled(true);
			
			QMetaObject::invokeMethod(handler, "print", Qt::DirectConnection, Q_ARG(QPrinter *, sPrinter));
			
			result = createStringObject(fileName);
		}
		break;
	
	default:
		{
			if (prompt) {
				Py_INCREF(settings);
				PrintDialog dialog(sPrinter, parentWidget);
				if (dialog.run() == QDialog::Rejected)
					Py_RETURN_FALSE;
				sPrinter->setFullPage(true);
				bool valid = (settings == Py_None) || saveSettings(settings, sPrinter);
				Py_DECREF(settings);
				if (!valid)
					break;
			}
			sPrinter->setPageMargins(0, 0, 0, 0, QPrinter::Millimeter);
			QMetaObject::invokeMethod(handler, "print", Qt::DirectConnection, Q_ARG(QPrinter *, sPrinter));
			
			if (handler == &standard_handler)
				result = createBoolObject(!standard_handler.aborted());
			else
				result = createBoolObject(true);
		}
		break;
	}
	
	delete sPrinter;
	sPrinter = NULL;
	
	return result;
}



#include "printing.moc"
