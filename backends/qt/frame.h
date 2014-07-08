#ifndef __frame_h__
#define __frame_h__


#include "slew.h"
#include "statusbar.h"
#include "menubar.h"
#include "constants/window.h"
#include "constants/frame.h"

#include <QMainWindow>
#include <QDialog>
#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QCloseEvent>
#include <QStatusBar>
#include <QToolBar>
#include <QMenuBar>


class Frame_Impl : public QMainWindow, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Frame, {
		if (fMenuBar) {
			Abstract_Proxy *proxy = getProxy(fMenuBar);
			if (proxy)
				SL_QAPP()->replaceProxyObject(proxy, new MenuBar_Impl());
		}
		if (fStatusBar) {
			Abstract_Proxy *proxy = getProxy(fStatusBar);
			if (proxy)
				SL_QAPP()->replaceProxyObject(proxy, new StatusBar_Impl());
		}
	})
	
	SL_DECLARE_SIZE_HINT(QMainWindow)
	
	void setResizeable(bool resizeable);
	bool isResizeable() { return fResizeable; }
	
	QStatusBar *statusBar() { return fStatusBar; }
	void setStatusBar(QStatusBar *statusBar) {
		if (fStatusBar)
			disconnect(fStatusBar, SIGNAL(destroyed(QObject *)), this, SLOT(handleOwnedDestroyed(QObject *)));
		fStatusBar = statusBar;
		QMainWindow::setStatusBar(statusBar);
		if (statusBar)
			connect(statusBar, SIGNAL(destroyed(QObject *)), this, SLOT(handleOwnedDestroyed(QObject *)), Qt::DirectConnection);
	}
	
	QMenuBar *menuBar() { return fMenuBar; }
	void setMenuBar(QMenuBar *menuBar) {
		if (fMenuBar)
			disconnect(fMenuBar, SIGNAL(destroyed(QObject *)), this, SLOT(handleOwnedDestroyed(QObject *)));
		fMenuBar = menuBar;
		QMainWindow::setMenuBar(menuBar);
		if (menuBar)
			connect(menuBar, SIGNAL(destroyed(QObject *)), this, SLOT(handleOwnedDestroyed(QObject *)), Qt::DirectConnection);
	}
	
	virtual QMenu *createPopupMenu() { return NULL; }
	
public slots:
	void handleOwnedDestroyed(QObject *object) {
		if (object == fStatusBar)
			fStatusBar = NULL;
		else if (object == fMenuBar)
			fMenuBar = NULL;
	}
protected:
	virtual bool event(QEvent *event);
	virtual void moveEvent(QMoveEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	virtual void closeEvent(QCloseEvent *event);

#ifdef Q_OS_WIN
	virtual bool winEvent(MSG *msg, long *result);
#endif

private:
	bool 			fResizeable;
	QStatusBar		*fStatusBar;
	QMenuBar		*fMenuBar;
};


#endif
