#include "slew.h"

#include "slider.h"

#include <QKeyEvent>


Slider_Impl::Slider_Impl()
	: QSlider(), WidgetInterface()
{
	connect(this, SIGNAL(valueChanged(int)), this, SLOT(handleValueChanged(int)));
}


bool
Slider_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_Left:
			case Qt::Key_Right:
			case Qt::Key_Up:
			case Qt::Key_Down:
			case Qt::Key_PageUp:
			case Qt::Key_PageDown:
			case Qt::Key_Home:
			case Qt::Key_End:
				return true;
			default:
				break;
			}
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::Wheel:
		return true;
	default:
		break;
	}
	return false;
}


void
Slider_Impl::handleValueChanged(int value)
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", value);
		runner.run();
	}
}


SL_DEFINE_METHOD(Slider, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->orientation() == Qt::Vertical)
		style |= SL_RANGED_STYLE_VERTICAL;
	else
		style |= SL_RANGED_STYLE_HORIZONTAL;
	if (impl->invertedAppearance())
		style |= SL_SLIDER_STYLE_INVERSE;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(Slider, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setOrientation(style & SL_RANGED_STYLE_VERTICAL ? Qt::Vertical : Qt::Horizontal);
	impl->setInvertedAppearance(style & SL_SLIDER_STYLE_INVERSE ? true : false);
})


SL_DEFINE_METHOD(Slider, get_ticks, {
	int ticks = 0;
	
	switch (impl->tickPosition()) {
	case QSlider::TicksAbove:	ticks = (impl->orientation() == Qt::Horizontal ? SL_SLIDER_TICKS_TOP : SL_SLIDER_TICKS_LEFT); break;
	case QSlider::TicksBelow:	ticks = (impl->orientation() == Qt::Horizontal ? SL_SLIDER_TICKS_BOTTOM : SL_SLIDER_TICKS_RIGHT); break;
	default:					ticks = SL_SLIDER_TICKS_NONE; break;
	}
	
	return PyInt_FromLong(ticks);
})


SL_DEFINE_METHOD(Slider, set_ticks, {
	QSlider::TickPosition pos = QSlider::NoTicks;
	int ticks;
	
	if (!PyArg_ParseTuple(args, "i", &ticks))
		return NULL;
	
	switch (ticks) {
	case SL_SLIDER_TICKS_TOP:		pos = QSlider::TicksAbove; break;
	case SL_SLIDER_TICKS_BOTTOM:	pos = QSlider::TicksBelow; break;
	case SL_SLIDER_TICKS_LEFT:		pos = QSlider::TicksLeft; break;
	case SL_SLIDER_TICKS_RIGHT:		pos = QSlider::TicksRight; break;
	}
	impl->setTickPosition(pos);
})



SL_START_PROXY_DERIVED(Slider, Ranged)
SL_PROPERTY(style)
SL_PROPERTY(ticks)
SL_END_PROXY_DERIVED(Slider, Ranged)


#include "slider.moc"
#include "slider_h.moc"

