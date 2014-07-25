#ifndef __dialog_h__
#define __dialog_h__


#include "slew.h"
#include "constants/window.h"
#include "constants/frame.h"
#include "constants/dialog.h"

#include <QDialog>


class Dialog_Impl : public QDialog, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Dialog, {
		Py_DECREF(fValue);
	})
	
	SL_DECLARE_SIZE_HINT(QDialog)
	
	void setResizeable(bool resizeable);
	bool isResizeable() { return fResizeable; }
	
	void setReturnValue(PyObject *value) { Py_INCREF(value); Py_DECREF(fValue); fValue = value; }
	PyObject *returnValue() { return fValue; }
	
public slots:
	virtual void done(int result);
	
protected:
	virtual void moveEvent(QMoveEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

#ifdef Q_OS_WIN
	virtual void showEvent(QShowEvent *event);
	virtual bool winEvent(MSG *msg, long *result);
#endif

private:
	bool 		fResizeable;
	PyObject	*fValue;
};


#endif
