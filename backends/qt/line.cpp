#include "slew.h"

#include "line.h"

#include "constants/widget.h"



Line_Impl::Line_Impl()
	: QFrame(), WidgetInterface()
{
	setFrameStyle(QFrame::Sunken | QFrame::HLine);
}


SL_DEFINE_METHOD(Line, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->frameStyle() & QFrame::VLine)
		style |= SL_LINE_STYLE_VERTICAL;
	else
		style |= SL_LINE_STYLE_HORIZONTAL;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Line, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	int shape = QFrame::Sunken;
	if (style & SL_LINE_STYLE_VERTICAL)
		shape |= QFrame::VLine;
	else
		shape |= QFrame::HLine;
	impl->setFrameStyle(shape);
})



SL_START_PROXY_DERIVED(Line, Window)
SL_PROPERTY(style)
SL_END_PROXY_DERIVED(Line, Window)


#include "line.moc"
#include "line_h.moc"

