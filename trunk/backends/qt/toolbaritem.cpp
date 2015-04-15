#include "slew.h"

#include "toolbar.h"
#include "toolbaritem.h"

#include <QWidgetAction>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QStyleOption>
#include <QMainWindow>


class Separator : public QWidget
{
	Q_OBJECT
	
public:
	Separator(QWidget *parent)
		: QWidget(parent), fOrientation(Qt::Horizontal), fDragging(false)
	{
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	}
	
	void initStyleOption(QStyleOption *option) const
	{
		option->initFrom(this);
		if (fOrientation == Qt::Horizontal)
			option->state |= QStyle::State_Horizontal;
	}
	
	virtual QSize sizeHint() const
	{
		QStyleOption opt;
		initStyleOption(&opt);
		const int extent = style()->pixelMetric(QStyle::PM_ToolBarSeparatorExtent, &opt, parentWidget());
		return QSize(extent, extent);
	}

	virtual void paintEvent(QPaintEvent *event)
	{
		QPainter p(this);
		QStyleOption opt;
		initStyleOption(&opt);
		style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, &p, parentWidget());
	}
	
#ifdef Q_OS_MAC
	virtual void mousePressEvent(QMouseEvent *event)
	{
		QMainWindow *win = qobject_cast<QMainWindow *>(window());
		if ((win) && (win->unifiedTitleAndToolBarOnMac())) {
			fDragging = true;
			fDragPressPosition = event->pos();
		}
	}
	
	virtual void mouseReleaseEvent(QMouseEvent *event)
	{
		fDragging = false;
	}
	
	virtual void mouseMoveEvent(QMouseEvent *event)
	{
		if (fDragging) {
			QPoint delta = event->pos() - fDragPressPosition;
			QWidget *w = window();
			w->move(w->pos() + delta);
		}
	}
#endif
	
public slots:
	void setOrientation(Qt::Orientation orientation)
	{
		fOrientation = orientation;
		update();
	}
	
protected:
	Qt::Orientation		fOrientation;
	bool				fDragging;
	QPoint				fDragPressPosition;
};


class FlexibleSeparator : public Separator
{
	Q_OBJECT

public:
	FlexibleSeparator(QWidget *parent)
		: Separator(parent)
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		setMinimumSize(QSize(1,1));
	}
	
	virtual void paintEvent(QPaintEvent *event)
	{
	}
};



ToolBarItem_Impl::ToolBarItem_Impl()
	: QWidgetAction(NULL), WidgetInterface(), fSeparator(false), fFlexible(false), fNoFocus(false), fMenu(NULL), fMenuMode(QToolButton::InstantPopup)
{
}


QWidget *
ToolBarItem_Impl::createWidget(QWidget *parent)
{
	if (fSeparator) {
		if (fFlexible)
			return new FlexibleSeparator(parent);
		else
			return new Separator(parent);
	}
	return NULL;
}


void
ToolBarItem_Impl::handleActionTriggered()
{
	if ((!fMenu) || (fMenuMode != QToolButton::InstantPopup))
		EventRunner(this, "onSelect").run();
}


void
ToolBarItem_Impl::update()
{
	QList<QWidget *> list = associatedWidgets();
	if (list.isEmpty())
		return;
	
	ToolBar_Impl *widget = qobject_cast<ToolBar_Impl *>(list.first());
	if (!widget)
		return;
	
	Separator *sep = qobject_cast<Separator *>(defaultWidget());
	if (sep) {
		connect(widget, SIGNAL(orientationChanged(Qt::Orientation)), sep, SLOT(setOrientation(Qt::Orientation)), Qt::UniqueConnection);
		return;
	}

	QToolButton *button = qobject_cast<QToolButton *>(widget->widgetForAction(this));
	if (!button)
		return;
	
	int type = qvariant_cast<int>(property("type"));
	if (type == SL_TOOLBAR_ITEM_TYPE_AUTOREPEAT) {
		button->setAutoRepeat(true);
		button->setAutoRepeatInterval(200);
		button->setAutoRepeatDelay(300);
	}
	
#ifdef Q_OS_MAC
	QObject::disconnect(this, SIGNAL(triggered()), button, SLOT(showMenu()));
	if ((fMenu) && (fMenuMode == QToolButton::InstantPopup)) {
		button->setPopupMode(QToolButton::MenuButtonPopup);
		QObject::connect(this, SIGNAL(triggered()), button, SLOT(showMenu()));
	}
	else
#endif
	button->setPopupMode(fMenuMode);
	button->setMenu(fMenu);
	button->setFocusPolicy(((type == SL_TOOLBAR_ITEM_TYPE_NO_FOCUS) || (fNoFocus)) ? Qt::NoFocus : Qt::ClickFocus);
}


void
ToolBarItem_Impl::setSeparator(bool separator)
{
	if (separator == fSeparator)
		return;
	
	QWidgetAction::setSeparator(separator);
	
	fSeparator = separator;
	reinsert();
}


void
ToolBarItem_Impl::setFlexible(bool flexible)
{
	if (flexible == fFlexible)
		return;
	
	fFlexible = flexible;
	reinsert();
}


void
ToolBarItem_Impl::setNoFocus(bool nofocus)
{
	if (nofocus == fNoFocus)
		return;
	
	fNoFocus = nofocus;
	reinsert();
}


void
ToolBarItem_Impl::reinsert()
{
	QList<QWidget *> list = associatedWidgets();
	if (!list.isEmpty()) {
		ToolBar_Impl *widget = qobject_cast<ToolBar_Impl *>(list.first());
		if (widget) {
			QAction *before;
			QList<QAction *> list = widget->actions();
			int index = list.indexOf(this);
			if (index == list.count() - 1)
				before = NULL;
			else
				before = list[index + 1];
			widget->removeAction(this);
			widget->insertAction(before, this);
			
			Separator *sep = qobject_cast<Separator *>(defaultWidget());
			if (sep)
				connect(widget, SIGNAL(orientationChanged(Qt::Orientation)), sep, SLOT(setOrientation(Qt::Orientation)), Qt::UniqueConnection);
		}
	}
}


void
ToolBarItem_Impl::setMenu(QMenu *menu, QToolButton::ToolButtonPopupMode mode)
{
	fMenu = menu;
	fMenuMode = mode;
	
	update();
}


SL_DEFINE_METHOD(ToolBarItem, set_autorepeat, {
	int delay, interval;
	
	if (!PyArg_ParseTuple(args, "ii", &delay, &interval))
		return NULL;
	
	QList<QWidget *> list = impl->associatedWidgets();
	if (!list.isEmpty()) {
		ToolBar_Impl *widget = qobject_cast<ToolBar_Impl *>(list.first());
		if (widget) {
			QToolButton *button = qobject_cast<QToolButton *>(widget->widgetForAction(impl));
			if (button) {
				button->setAutoRepeatDelay(delay);
				button->setAutoRepeatInterval(interval);
			}
		}
	}
})


SL_DEFINE_METHOD(ToolBarItem, get_type, {
	return PyInt_FromLong(qvariant_cast<int>(impl->property("type")));
})


SL_DEFINE_METHOD(ToolBarItem, set_type, {
	int type;
	
	if (!PyArg_ParseTuple(args, "i", &type))
		return NULL;
	
	impl->setProperty("type", type);
	
	impl->setCheckable(((type == SL_TOOLBAR_ITEM_TYPE_CHECK) || (type == SL_TOOLBAR_ITEM_TYPE_RADIO)) ? true : false);
	impl->setSeparator(type == SL_TOOLBAR_ITEM_TYPE_SEPARATOR ? true : false);
	
	QList<QWidget *> list = impl->associatedWidgets();
	if (!list.isEmpty())
		relinkActions(list.first());
	
	impl->update();
})


SL_DEFINE_METHOD(ToolBarItem, get_text, {
	return createStringObject(impl->text());
})


SL_DEFINE_METHOD(ToolBarItem, set_text, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	if (!impl->isSeparator()) {
		int pos = text.indexOf('|');
		if ((pos != -1) && (pos + 1 < text.length())) {
			impl->setShortcut(QKeySequence(text.mid(pos + 1)));
			text = text.mid(0, pos);
		}
		impl->setText(text);
	}
})


SL_DEFINE_METHOD(ToolBarItem, get_description, {
	return createStringObject(impl->statusTip());
})


SL_DEFINE_METHOD(ToolBarItem, set_description, {
	QString description;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &description))
		return NULL;
	
	if (!impl->isSeparator()) {
		impl->setToolTip(description);
		impl->setStatusTip(description);
	}
})


SL_DEFINE_METHOD(ToolBarItem, is_checked, {
	return createBoolObject(impl->isChecked());
})


SL_DEFINE_METHOD(ToolBarItem, set_checked, {
	bool checked;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &checked))
		return NULL;
	
	int type = qvariant_cast<int>(impl->property("type"));
	if ((type == SL_TOOLBAR_ITEM_TYPE_CHECK) || (type == SL_TOOLBAR_ITEM_TYPE_RADIO))
		impl->setChecked(checked);
})


SL_DEFINE_METHOD(ToolBarItem, is_enabled, {
	return createBoolObject(impl->isEnabled());
})


SL_DEFINE_METHOD(ToolBarItem, set_enabled, {
	bool enabled;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &enabled))
		return NULL;
	
	impl->setEnabled(enabled);
})


SL_DEFINE_METHOD(ToolBarItem, get_flags, {
	int flags = 0;
	if (impl->isFlexible())
		flags |= SL_TOOLBAR_ITEM_FLAG_FLEXIBLE;
	if (impl->isNoFocus())
		flags |= SL_TOOLBAR_ITEM_FLAG_NOFOCUS;
	return PyInt_FromLong(flags);
})


SL_DEFINE_METHOD(ToolBarItem, set_flags, {
	int flags;
	
	if (!PyArg_ParseTuple(args, "i", &flags))
		return NULL;
	
	impl->setFlexible(flags & SL_TOOLBAR_ITEM_FLAG_FLEXIBLE ? true : false);
	impl->setNoFocus(flags & SL_TOOLBAR_ITEM_FLAG_NOFOCUS ? true : false);
})


SL_DEFINE_METHOD(ToolBarItem, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_METHOD(ToolBarItem, set_icon, {
	QIcon icon;
	
	if (!PyArg_ParseTuple(args, "O&", convertIcon, &icon))
		return NULL;
	
	impl->setIcon(icon);
})


SL_DEFINE_METHOD(ToolBarItem, get_menu, {
	QMenu *menu = impl->menu();
	if (!menu)
		Py_RETURN_NONE;
	
	PyObject *object = getObject(menu);
	if (!object) {
		PyErr_Clear();
		Py_RETURN_NONE;
	}
	return object;
})


SL_DEFINE_METHOD(ToolBarItem, set_menu, {
	PyObject *object;
	QMenu *menu;
	int mode;
	QToolButton::ToolButtonPopupMode popupMode;
	
	if (!PyArg_ParseTuple(args, "Oi", &object, &mode))
		return NULL;
	
	switch (mode) {
	case SL_TOOLBAR_ITEM_MENU_DELAYED:		popupMode = QToolButton::DelayedPopup; break;
	case SL_TOOLBAR_ITEM_MENU_BUTTON:		popupMode = QToolButton::InstantPopup; break;
	default:								popupMode = QToolButton::MenuButtonPopup; break;
	}
	
	if (object == Py_None) {
		impl->setMenu(NULL, popupMode);
	}
	else {
		menu = (QMenu *)getImpl(object);
		if (!menu)
			return NULL;
		impl->setMenu(menu, popupMode);
	}
})


SL_START_PROXY_DERIVED(ToolBarItem, Widget)
SL_METHOD(set_autorepeat)
SL_PROPERTY(type)
SL_PROPERTY(text)
SL_PROPERTY(description)
SL_BOOL_PROPERTY(checked)
SL_BOOL_PROPERTY(enabled)
SL_PROPERTY(flags)
SL_PROPERTY(icon)
SL_PROPERTY(menu)
SL_END_PROXY_DERIVED(ToolBarItem, Widget)


#include "toolbaritem.moc"
#include "toolbaritem_h.moc"

