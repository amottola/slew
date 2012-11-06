#include "slew.h"

#include "foldpanel.h"

#include "constants/window.h"

#include <QAbstractButton>
#include <QStyleOptionGroupBox>


class FoldPanel_Expander : public QAbstractButton
{
	Q_OBJECT

public:
	FoldPanel_Expander(FoldPanel_Impl *parent)
		: QAbstractButton(parent)
	{
		setCursor(Qt::ArrowCursor);
		setFocusPolicy(Qt::NoFocus);
	}
	
protected:
	virtual void paintEvent(QPaintEvent *event)
	{
		FoldPanel_Impl *panel = (FoldPanel_Impl *)parent();
		QPainter painter(this);
		QPainterPath path;
		QMatrix matrix;
		painter.setRenderHint(QPainter::Antialiasing);
		matrix.translate(width() / 2, (height() + 4) / 2);
		if (panel->fIsExpanded)
			matrix.rotate(180);
		path.moveTo(-5, -3);
		path.lineTo(0, 2);
		path.lineTo(5, -3);
		painter.setMatrix(matrix);
		painter.setPen(Qt::NoPen);
		painter.setBrush(palette().highlightedText().color());
		painter.drawPath(path);
	}
};



class FoldPanel_Content : public QWidget
{
	Q_OBJECT

public:
	FoldPanel_Content(FoldPanel_Impl *parent)
		: QWidget(parent), fParent(parent)
	{
	}

signals:
	void layoutChanged();
	
protected:
	virtual bool event(QEvent *event)
	{
		bool result = QWidget::event(event);
		switch (event->type()) {
		case QEvent::ChildAdded:
		case QEvent::ChildRemoved:
// 		case QEvent::ChildPolished:
		case QEvent::LayoutRequest:
		case QEvent::HideToParent:
		case QEvent::ShowToParent:
			emit layoutChanged();
			break;
		default:
			break;
		}
		return result;
	}
	
private:
	FoldPanel_Impl	*fParent;
};



class SizeGrip : public QSizeGrip
{
public:
	SizeGrip(FoldPanel_Impl *parent) : QSizeGrip(parent), fParent(parent) {}
	
	virtual void mousePressEvent(QMouseEvent *e)
	{
		if (e->button() == Qt::LeftButton) {
			fPressed = true;
			fOrigin = e->globalPos();
			fSize = fParent->size();
		}
	}
	
	virtual void mouseMoveEvent(QMouseEvent *e)
	{
		if (fPressed) {
			QSize size = qvariant_cast<QSize>(fParent->property("explicitSize"));
			if (fParent->hasHorizontalResize())
				size.setWidth(fSize.width() + (e->globalX() - fOrigin.x()));
			if (fParent->hasVerticalResize())
				size.setHeight(fSize.height() + (e->globalY() - fOrigin.y()));
			fParent->setProperty("explicitSize", QVariant(size));
			fParent->setupLayout();
			
			EventRunner runner(fParent, "onResize");
			runner.set("size", size);
			runner.run();
		}
	}
	
	virtual void mouseReleaseEvent(QMouseEvent *e)
	{
		if (e->button() == Qt::LeftButton) {
			fPressed = false;
			fOrigin = QPoint();
		}
	}
	
private:
	FoldPanel_Impl	*fParent;
	QPoint			fOrigin;
	QSize			fSize;
	bool			fPressed;
};



FoldPanel_Impl::FoldPanel_Impl()
	: QWidget(), WidgetInterface(), fIsExpanded(true), fTimeLine(NULL), fFlat(false), fExpandable(true), fDuration(150), fLastFocus(NULL), fInAnimation(false), fHResize(false), fVResize(false)
{
	fExpander = new FoldPanel_Expander(this);
	fContent = new FoldPanel_Content(this);
	fSizeGrip = new SizeGrip(this);
	fSizeGrip->hide();
	
	setFocusPolicy(Qt::ClickFocus);
	setAttribute(Qt::WA_MacShowFocusRect, false);
	fContent->setFocusProxy(this);
	setAnimated(true);
	
	fShortcut = new QShortcut(QKeySequence("Ctrl+Alt+E"), this, NULL, NULL, Qt::WidgetWithChildrenShortcut);
	connect(fShortcut, SIGNAL(activated()), this, SLOT(toggleExpand()));
	connect(fExpander, SIGNAL(clicked()), this, SLOT(toggleExpand()));
	connect(fContent, SIGNAL(layoutChanged()), this, SLOT(setupLayout()));//, Qt::QueuedConnection);
}


void
FoldPanel_Impl::updatePalette()
{
	QPalette palette = this->palette();
	palette.setColor(QPalette::Inactive, QPalette::HighlightedText, palette.color(QPalette::Disabled, QPalette::HighlightedText));
	setPalette(palette);
	fExpander->setPalette(palette);
}


void
FoldPanel_Impl::setAnimated(bool animated)
{
	if (fTimeLine)
		delete fTimeLine;
	
	if (animated) {
		fTimeLine = new QTimeLine();
		fTimeLine->setFrameRange(0, 100);
		fTimeLine->setUpdateInterval(10);
		fTimeLine->setDuration(fDuration);
		fTimeLine->setCurrentTime(fIsExpanded ? fTimeLine->duration() : 0);
		connect(fTimeLine, SIGNAL(frameChanged(int)), this, SLOT(handleFrameChanged(int)));
		connect(fTimeLine, SIGNAL(finished()), this, SLOT(handleExpanded()));
	}
	else {
		fTimeLine = NULL;
	}
}


void
FoldPanel_Impl::setHorizontalResize(bool enabled)
{
	fHResize = enabled;
	fSizeGrip->setVisible(fHResize || fVResize);
}


void
FoldPanel_Impl::setVerticalResize(bool enabled)
{
	fVResize = enabled;
	fSizeGrip->setVisible(fHResize || fVResize);
}


void
FoldPanel_Impl::toggleExpand(bool animate)
{
	if (!fExpandable)
		return;
	
	QWidget *focusw = focusWidget();
	WidgetInterface *focus = dynamic_cast<WidgetInterface *>(focusw);
	if ((focus) && (!focus->canFocusOut(focusw, this)))
		return;
	
	if (fContent->isHidden()) {
// 		fContent->resize(fContent->sizeHint());
		fContent->show();
	}
	
	if ((animate) && (fTimeLine)) {
		fInAnimation = true;
		fTimeLine->stop();
		fIsExpanded = !fIsExpanded;
		
		fTimeLine->setDirection(fIsExpanded ? QTimeLine::Forward : QTimeLine::Backward);
		fTimeLine->setCurveShape(fIsExpanded ? QTimeLine::EaseInCurve : QTimeLine::EaseOutCurve);
		fTimeLine->resume();
	}
	else {
		fIsExpanded = !fIsExpanded;
		handleExpanded();
	}
	
	fSizeGrip->setVisible(fIsExpanded && (fHResize || fVResize));
}


void
FoldPanel_Impl::expand(bool animate)
{
	if (!fIsExpanded)
		toggleExpand(animate);
}


void
FoldPanel_Impl::collapse(bool animate)
{
	if (fIsExpanded)
		toggleExpand(animate);
}


void
FoldPanel_Impl::handleFrameChanged(int frame)
{
	if (fTimeLine) {
		EventRunner runner(this, "onFolding");
		runner.set("value", fTimeLine->currentValue());
		runner.run();
	}
	setupLayout();
}


void
FoldPanel_Impl::handleExpanded()
{
	setupLayout();
	
	if (fIsExpanded) {
		if (fLastFocus) {
			fLastFocus->setFocus();
		}
		if (fTimeLine)
			fTimeLine->setCurrentTime(fTimeLine->duration());
	}
	else {
		fLastFocus = fContent->focusWidget();
		if ((fLastFocus) && (!fLastFocus->hasFocus()))
			fLastFocus = NULL;
		fContent->hide();
		setFocus();
		if (fTimeLine)
			fTimeLine->setCurrentTime(0);
	}
	fInAnimation = false;
	resize(size());
	
	EventRunner(this, fIsExpanded ? "onExpand" : "onCollapse").run();
}


void
FoldPanel_Impl::focusInEvent(QFocusEvent *event)
{
	if (fIsExpanded) {
		if (event->reason() == Qt::BacktabFocusReason)
			focusPreviousChild();
		else if (event->reason() == Qt::TabFocusReason)
			focusNextChild();
		else if (fLastFocus)
			fLastFocus->setFocus();
	}
}


void
FoldPanel_Impl::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->y() < fontMetrics().height() + 6)
		toggleExpand();
	else
		QWidget::mouseDoubleClickEvent(event);
}


void
FoldPanel_Impl::setupLayout()
{
	fExpander->setVisible((!fFlat) && fExpandable);
	updateGeometry();
}


QSize
FoldPanel_Impl::sizeFromContents(const QSize& size) const
{
	QSize csize(size);
	if (!fFlat) {
		QFont font(this->font());
		font.setBold(true);
		
		QStyleOptionGroupBox option;
		option.initFrom(this);
		option.rect = QRect(0,0,100,100);
		option.subControls = 0;
		QFontMetrics metrics = option.fontMetrics;
		QRect rect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxContents, this);
		csize.rheight() += (option.rect.height() - rect.height()) + metrics.height() + 6;
		csize.rwidth() += (option.rect.width() - rect.width());
		csize.rwidth() = qMax(csize.width(), metrics.width(fLabel) + 50);
	}
	
	return csize.expandedTo(QApplication::globalStrut());
}


QSize
FoldPanel_Impl::animateSize(const QSize& _size) const
{
	QSize size(_size);
	QFont font(this->font());
	font.setBold(true);
	int base;
	
	if (fFlat) {
		base = 0;
	}
	else {
	    QFontMetrics metrics(font);
		base = metrics.height() + 6;
	}
	
	if ((fTimeLine) && (fTimeLine->state() == QTimeLine::Running)) {
		size.rheight() = base + (double(size.height() - base) * fTimeLine->currentValue());
	}
	else {
		if (!fIsExpanded) {
			size.rheight() = base;
		}
	}
	
	return size;
}


QSize
FoldPanel_Impl::minimumSizeHint() const
{
	QSize size = qvariant_cast<QSize>(property("explicitSize"));
	QSize tsize = sizeFromContents(fContent->minimumSizeHint());
	
	if ((size.width() <= 0) || (size.width() < tsize.width()))
		size.rwidth() = tsize.width();
	if ((size.height() <= 0) || (size.height() < tsize.height()))
		size.rheight() = tsize.height();
	
	return animateSize(size);
}


QSize
FoldPanel_Impl::sizeHint() const
{
	QSize size = qvariant_cast<QSize>(property("explicitSize"));
	QSize tsize = sizeFromContents(fContent->sizeHint());
	
	if ((size.width() <= 0) || (size.width() < tsize.width()))
		size.rwidth() = tsize.width();
	if ((size.height() <= 0) || (size.height() < tsize.height()))
		size.rheight() = tsize.height();
	
	return animateSize(size);
}


void
FoldPanel_Impl::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	QFont font(this->font());
	font.setBold(true);
    QFontMetrics metrics(font);
	
	QRect rect = this->rect();
	QColor color, bgcolor;
	QWidget *focused = NULL;
	if (window())
		focused = window()->focusWidget();
	if (!focused)
		focused = QApplication::focusWidget();
	if (!focused)
		focused = QApplication::activePopupWidget();
	if ((focused) && (focused->window()->isActiveWindow()) && ((focused == this) || (isAncestorOf(focused)) || ((focused->window()->parentWidget()) && (isAncestorOf(focused->window()->parentWidget()))))) {
		color = palette().highlight().color();
		bgcolor = color;
		bgcolor.setAlpha(32);
	}
	else
		color = palette().color(QPalette::Inactive, QPalette::Highlight).darker(150);
	
	if (fFlat) {
		painter.setPen(Qt::NoPen);
		if (bgcolor.isValid())
			painter.fillRect(rect, bgcolor);
		return;
	}
	
	painter.setPen(Qt::NoPen);
	if (bgcolor.isValid()) {
		painter.setBrush(bgcolor);
		painter.drawRoundedRect(0, 0, rect.width(), rect.height() - 1, 5, 5);
	}
	painter.setBrush(color);
	painter.drawRect(2, 0, rect.width() - 4, 1);
	painter.drawRect(1, 1, rect.width() - 2, 1);
	int h = metrics.height();
	if ((fIsExpanded) || ((fTimeLine) && (fTimeLine->currentValue() > 0))) {
		painter.drawRect(0, 2, rect.width(), h + 4);
	}
	else {
		painter.drawRect(0, 2, rect.width(), h + 2);
		painter.drawRect(1, h + 4, rect.width() - 2, 1);
		painter.drawRect(2, h + 5, rect.width() - 4, 1);
	}
	
	painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
	rect.adjust(1,1,-1,-1);
	color = QColor(0,0,0,32);
	painter.setPen(color);
	painter.setBrush(Qt::NoBrush);
	painter.drawRoundedRect(rect, 2, 2);
	color.setAlpha(64);
	painter.setPen(color);
	rect.adjust(-1,-1,1,1);
	painter.drawRoundedRect(rect, 3, 3);
	
	painter.setPen(palette().highlightedText().color());
	rect = QRect(20, 0, width() - 50, h + 6);
	painter.setFont(font);
	painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, metrics.elidedText(fLabel, Qt::ElideRight, rect.width()));
}


void
FoldPanel_Impl::resizeEvent(QResizeEvent *event)
{
	QSize size = this->size();
	QFont font(this->font());
	font.setBold(true);
    QFontMetrics metrics(font);
    
	fExpander->setGeometry(size.width() - 20, 2, 15, metrics.height());
	
	QRect rect;
	
	if (fFlat) {
		rect = QRect(QPoint(0,0), size);
	}
	else {
		QStyleOptionGroupBox option;
		option.initFrom(this);
		option.subControls = 0;
		
		rect = style()->subControlRect(QStyle::CC_GroupBox, &option, QStyle::SC_GroupBoxContents, this);
		rect.adjust(0, metrics.height() + 6, 0, 0);
	}
	if ((!fInAnimation) && (rect != fContent->geometry())) {
		if (!fIsExpanded)
			rect.setHeight(fContent->height());
		fContent->setGeometry(rect);
	}
	
	QWidget::resizeEvent(event);
	
	fSizeGrip->move(size.width() - fSizeGrip->width() - 2, size.height() - fSizeGrip->height() - 2);
	
// 	setupLayout();
}


SL_DEFINE_METHOD(FoldPanel, insert, {
	int index;
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "iO", &index, &object))
		return NULL;
	
	return insertWindowChild(impl->content(), object);
})


SL_DEFINE_METHOD(FoldPanel, remove, {
	PyObject *object;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	return removeWindowChild(impl->content(), object);
})


SL_DEFINE_METHOD(FoldPanel, set_expanded, {
	bool animated, expanded;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertBool, &animated, convertBool, &expanded))
		return NULL;
	
	if (expanded)
		impl->expand(animated);
	else
		impl->collapse(animated);
})


SL_DEFINE_METHOD(FoldPanel, is_expanded, {
	return createBoolObject(impl->isExpanded());
})


SL_DEFINE_METHOD(FoldPanel, set_size, {
	QSize size, minSize = impl->minimumSize(), sizeHint = impl->sizeHint();
	
	if (!PyArg_ParseTuple(args, "O&", convertSize, &size))
		return NULL;
	
	if (minSize.isNull())
		minSize = impl->minimumSizeHint();
	
	if (size.width() <= 0)
		size.rwidth() = minSize.width();
	if (size.height() <= 0)
		size.rheight() = minSize.height();
	if (size.width() <= 0)
		size.rwidth() = sizeHint.width();
	if (size.height() <= 0)
		size.rheight() = sizeHint.height();
	
	impl->setProperty("explicitSize", QVariant::fromValue(size));
	impl->setupLayout();
})


SL_DEFINE_METHOD(FoldPanel, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	if (impl->isExpandable())
		style |= SL_FOLDPANEL_STYLE_EXPANDABLE;
	if (impl->isAnimated())
		style |= SL_FOLDPANEL_STYLE_EXPANDABLE;
	if (impl->isFlat())
		style |= SL_FOLDPANEL_STYLE_FLAT;
	if (impl->hasHorizontalResize())
		style |= SL_FOLDPANEL_STYLE_H_RESIZE;
	if (impl->hasVerticalResize())
		style |= SL_FOLDPANEL_STYLE_V_RESIZE;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(FoldPanel, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style | SL_WINDOW_STYLE_NOFOCUS);
	impl->setExpandable(style & SL_FOLDPANEL_STYLE_EXPANDABLE ? true : false);
	impl->setAnimated(style & SL_FOLDPANEL_STYLE_ANIMATED ? true : false);
	impl->setFlat(style & SL_FOLDPANEL_STYLE_FLAT ? true : false);
	impl->setHorizontalResize(style & SL_FOLDPANEL_STYLE_H_RESIZE ? true : false);
	impl->setVerticalResize(style & SL_FOLDPANEL_STYLE_V_RESIZE ? true : false);
})


SL_DEFINE_METHOD(FoldPanel, set_hicolor, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(QPalette::HighlightedText);
	}
	
	palette.setColor(QPalette::HighlightedText, color);
	impl->setPalette(palette);
	impl->updatePalette();
})


SL_DEFINE_METHOD(FoldPanel, get_label, {
	return createStringObject(impl->label());
})


SL_DEFINE_METHOD(FoldPanel, set_label, {
	QString label;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &label))
		return NULL;
	
	impl->setLabel(label);
})


SL_DEFINE_METHOD(FoldPanel, get_duration, {
	return PyInt_FromLong(impl->duration());
})


SL_DEFINE_METHOD(FoldPanel, set_duration, {
	int duration;
	
	if (!PyArg_ParseTuple(args, "i", &duration))
		return NULL;
	
	impl->setDuration(duration);
})


SL_DEFINE_METHOD(FoldPanel, get_toggle_shortcut, {
	return createStringObject(impl->shortcut());
})


SL_DEFINE_METHOD(FoldPanel, set_toggle_shortcut, {
	QString shortcut;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &shortcut))
		return NULL;
	
	impl->setShortcut(shortcut);
})



SL_START_PROXY_DERIVED(FoldPanel, Window)
SL_METHOD(insert)
SL_METHOD(remove)
SL_METHOD(set_expanded)
SL_METHOD(is_expanded)
SL_METHOD(set_size)
SL_METHOD(set_hicolor)

SL_PROPERTY(style)
SL_PROPERTY(label)
SL_PROPERTY(duration)
SL_PROPERTY(toggle_shortcut)
SL_END_PROXY_DERIVED(FoldPanel, Window)


#include "foldpanel.moc"
#include "foldpanel_h.moc"

