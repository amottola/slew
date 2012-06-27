#ifndef __systrayicon_h__
#define __systrayicon_h__


#include "slew.h"

#include <QSystemTrayIcon>


class SystrayIcon_Impl : public QSystemTrayIcon, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(SystrayIcon)
	
	virtual void setVisible(bool visible);
	
public slots:
	void handleActivated(QSystemTrayIcon::ActivationReason reason);
};


#endif
