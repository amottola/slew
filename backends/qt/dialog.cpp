#include "slew.h"

#include "dialog.h"
#include "sizer.h"

#include <QResizeEvent>
#include <QKeyEvent>
#include <QPointer>


Dialog_Impl::Dialog_Impl()
	: QDialog(), WidgetInterface(), fResizeable(true), fValue(Py_None)
{
	Py_INCREF(fValue);
	setAttribute(Qt::WA_QuitOnClose);
}


#ifdef Q_WS_WIN

#include "windows.h"

bool
Dialog_Impl::winEvent(MSG *msg, long *result)
{
	if ((!fResizeable) && (msg->message == WM_SIZING)) {
		GetWindowRect(msg->hwnd, (LPRECT)msg->lParam);
		*result = TRUE;
		return true;
	}
	return false;
}

#endif


void
Dialog_Impl::setResizeable(bool resizeable)
{
	fResizeable = resizeable;
	
#ifdef Q_WS_MAC
	helper_set_resizeable(this, resizeable);
#endif
}


void
Dialog_Impl::moveEvent(QMoveEvent *event)
{
	QDialog::moveEvent(event);
	
	hidePopupMessage();
	Completer::hide();
}


void
Dialog_Impl::resizeEvent(QResizeEvent *event)
{
	QDialog::resizeEvent(event);
	
	hidePopupMessage();
	Completer::hide();
	
	if (event->size() != event->oldSize()) {
		EventRunner runner(this, "onResize");
		if (runner.isValid()) {
			runner.set("size", event->size());
			runner.run();
		}
	}
}


void
Dialog_Impl::done(int result)
{
	EventRunner runner(this, "onClose");
	if (runner.isValid()) {
		runner.set("accepted", result == Accepted);
		if (runner.run()) {
			QDialog::done(result);
		}
	}
	else {
		QDialog::done(result);
	}
}


SL_DEFINE_METHOD(Dialog, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	return insertWindowChild(impl, object);
})


SL_DEFINE_METHOD(Dialog, remove, {
	(void)impl;
	
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	return removeWindowChild(impl, object);
})


SL_DEFINE_METHOD(Dialog, set_visible, {
	bool visible;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &visible))
		return NULL;
	
	if (visible) {
		impl->setModal(false);
		impl->show();
	}
	else {
		impl->hide();
	}
})


#ifdef Q_WS_MAC
#define SHOW		open
#else
#define SHOW		show
#endif

SL_DEFINE_METHOD(Dialog, show_modal, {
	bool blocking;
	int modality;
	Qt::WindowModality qmodality;
	
	if (!PyArg_ParseTuple(args, "O&i", convertBool, &blocking, &modality))
		return NULL;
	
	switch (modality) {
	case SL_DIALOG_MODALITY_WINDOW:
		{
			qmodality = Qt::WindowModal;
		}
		break;
	case SL_DIALOG_MODALITY_APPLICATION:
		{
			qmodality = Qt::ApplicationModal;
		}
		break;
	default:
		{
			if (impl->parentWidget())
				qmodality = Qt::WindowModal;
			else
				qmodality = Qt::ApplicationModal;
		}
		break;
	}
	impl->setWindowModality(qmodality);
	impl->setReturnValue(Py_None);
	
	if (blocking) {
		Py_BEGIN_ALLOW_THREADS
		
		impl->exec();
		
		Py_END_ALLOW_THREADS
	}
	else {
		impl->SHOW();
	}
	
	PyObject *result = impl->returnValue();
	Py_INCREF(result);
	return result;
})

#undef SHOW


SL_DEFINE_METHOD(Dialog, end_modal, {
	PyObject *value;
	
	if (!PyArg_ParseTuple(args, "O", &value))
		return NULL;
	
	impl->setReturnValue(value);
	impl->close();
	impl->hide();
})


SL_DEFINE_METHOD(Dialog, close, {
	impl->close();
	impl->hide();
})


SL_DEFINE_METHOD(Dialog, get_style, {
	int style = 0;
	Qt::WindowFlags flags = impl->windowFlags();
	
	if ((flags & Qt::WindowType_Mask) == Qt::Sheet)
		style |= SL_FRAME_STYLE_SHEET;
	if (flags & Qt::WindowTitleHint)
		style |= SL_FRAME_STYLE_CAPTION;
	if (flags & Qt::WindowMinimizeButtonHint)
		style |= SL_FRAME_STYLE_MINIMIZE_BOX;
	if (flags & Qt::WindowMaximizeButtonHint)
		style |= SL_FRAME_STYLE_MAXIMIZE_BOX;
	if (flags & Qt::WindowCloseButtonHint)
		style |= SL_FRAME_STYLE_CLOSE_BOX;
	if (flags & Qt::WindowStaysOnTopHint)
		style |= SL_FRAME_STYLE_STAY_ON_TOP;
	if (impl->isSizeGripEnabled())
		style |= SL_FRAME_STYLE_RESIZEABLE;
	if (impl->isWindowModified())
		style |= SL_FRAME_STYLE_MODIFIED;
	if (impl->testAttribute(Qt::WA_TranslucentBackground))
		style |= SL_WINDOW_STYLE_TRANSLUCENT;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Dialog, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	Qt::WindowModality modality = Qt::ApplicationModal;
	Qt::WindowFlags flags = Qt::Dialog | Qt::CustomizeWindowHint;
	
	if (style & SL_FRAME_STYLE_CAPTION)
		flags |= Qt::WindowTitleHint;
	if (style & SL_FRAME_STYLE_MINIMIZE_BOX)
		flags |= Qt::WindowMinimizeButtonHint | Qt::WindowSystemMenuHint;
	if (style & SL_FRAME_STYLE_MAXIMIZE_BOX)
		flags |= Qt::WindowMaximizeButtonHint | Qt::WindowSystemMenuHint;
	if (style & SL_FRAME_STYLE_CLOSE_BOX)
		flags |= Qt::WindowCloseButtonHint | Qt::WindowSystemMenuHint;
	if (style & SL_FRAME_STYLE_STAY_ON_TOP)
		flags |= Qt::WindowStaysOnTopHint;
	if ((style & SL_WINDOW_STYLE_FRAMELESS) || (!(flags & (Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint))))
		flags |= Qt::FramelessWindowHint;
	
	if (style & SL_FRAME_STYLE_SHEET) {
		if (impl->parentWidget())
			modality = Qt::WindowModal;
		flags = (flags & ~Qt::WindowType_Mask) | Qt::Sheet;
	}
	
	bool visible = impl->isVisible();
	impl->setWindowModality(modality);
	if (flags |= impl->windowFlags())
		impl->setWindowFlags(flags);
	
	if (style & SL_FRAME_STYLE_RESIZEABLE) {
		impl->setSizeGripEnabled(true);
		if (!(style & SL_FRAME_STYLE_SHEET))
			impl->setResizeable(true);
	}
	else {
		impl->setSizeGripEnabled(false);
		if (!(style & SL_FRAME_STYLE_SHEET))
			impl->setResizeable(false);
	}
	
	if (style & SL_FRAME_STYLE_MAXIMIZED)
		impl->setWindowState(Qt::WindowMaximized);
	else if (style & SL_FRAME_STYLE_MINIMIZED)
		impl->setWindowState(Qt::WindowMinimized);
	
	impl->setWindowModified(style & SL_FRAME_STYLE_MODIFIED ? true : false);
	impl->setAttribute(Qt::WA_TranslucentBackground, style & SL_WINDOW_STYLE_TRANSLUCENT ? true : false);
	if (visible)
		impl->setVisible(visible);
})


SL_START_PROXY_DERIVED(Dialog, Frame)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(set_visible)

SL_METHOD(show_modal)
SL_METHOD(end_modal)
SL_METHOD(close)

SL_PROPERTY(style)
SL_END_PROXY_DERIVED(Dialog, Frame)


#include "dialog.moc"
#include "dialog_h.moc"

