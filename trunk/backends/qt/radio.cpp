#include "slew.h"

#include "radio.h"

#include <QKeyEvent>
#include <QMouseEvent>


static QHash<QString, QButtonGroup *> sButtonGroups;



Radio_Impl::Radio_Impl()
	: QRadioButton(), WidgetInterface(), fButtonGroup(NULL)
{
	connect(this, SIGNAL(toggled(bool)), this, SLOT(handleToggled(bool)));
	setGroup("");
}


void
Radio_Impl::clear()
{
	if (fButtonGroup) {
		fButtonGroup->removeButton(this);
		if (fButtonGroup->buttons().size() == 0) {
			sButtonGroups.remove(fButtonGroup->objectName());
			delete fButtonGroup;
		}
	}
}


bool
Radio_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			switch (e->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Space:
			case Qt::Key_Select:
				return true;
			default:
				break;
			}
		}
		break;
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
		{
			QMouseEvent *e = (QMouseEvent *)event;
			return hitButton(e->pos());
		}
		break;
	default:
		break;
	}
	return false;
}


bool
Radio_Impl::event(QEvent *event)
{
	bool result = QRadioButton::event(event);
	
	if (event->type() == QEvent::ParentChange)
		setGroup(fGroup);
	
	return result;
}


void
Radio_Impl::setGroup(const QString& group)
{
	QString groupName;
	QWidget *window = this->window();
	
	fGroup = group;
	if (window)
		groupName = QString("[%1]/%2").arg(window->objectName()).arg(group);
	else
		groupName = QString("[]/%1").arg(group);
	
	if (fButtonGroup) {
		fButtonGroup->removeButton(this);
		if (fButtonGroup->buttons().size() == 0) {
			sButtonGroups.remove(fButtonGroup->objectName());
			delete fButtonGroup;
		}
	}
	
	fButtonGroup = sButtonGroups[groupName];
	if (!fButtonGroup) {
		fButtonGroup = new QButtonGroup();
		fButtonGroup->setObjectName(groupName);
		sButtonGroups[groupName] = fButtonGroup;
	}
	fButtonGroup->addButton(this);
}


void
Radio_Impl::handleToggled(bool toggled)
{
	if (toggled) {
		EventRunner runner(this, "onSelect");
		if (runner.isValid()) {
			runner.set("selection", toggled);
			runner.run();
		}
	}
}


SL_DEFINE_METHOD(Radio, get_group, {
	return createStringObject(impl->group());
})


SL_DEFINE_METHOD(Radio, set_group, {
	QString group;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &group))
		return NULL;
	
	impl->setGroup(group);
})


SL_DEFINE_METHOD(Radio, get_label, {
	return createStringObject(impl->text());
})


SL_DEFINE_METHOD(Radio, set_label, {
	QString label;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &label))
		return NULL;
	
	impl->setText(label);
})


SL_DEFINE_METHOD(Radio, is_selected, {
	return createBoolObject(impl->isChecked());
})


SL_DEFINE_METHOD(Radio, set_selected, {
	bool checked;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &checked))
		return NULL;
	
	impl->setChecked(checked);
})



SL_START_PROXY_DERIVED(Radio, Window)
SL_PROPERTY(group)
SL_PROPERTY(label)
SL_BOOL_PROPERTY(selected)
SL_END_PROXY_DERIVED(Radio, Window)


#include "radio.moc"
#include "radio_h.moc"

