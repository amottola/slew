#ifndef __wizard_h__
#define __wizard_h__


#include "slew.h"

#include "dialog.h"
#include "constants/window.h"
#include "constants/wizard.h"

#include <QWizard>


class Wizard_Impl : public QWizard, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Wizard)
	
	SL_DECLARE_SIZE_HINT(QWizard)

#ifdef Q_OS_WIN
	virtual void showEvent(QShowEvent *event);
#endif
};


#endif
