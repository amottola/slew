#ifndef __textfield_h__
#define __textfield_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/textfield.h"

#include <QMenu>


class TextField_Impl : public FormattedLineEdit, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(TextField)
	
	SL_DECLARE_SET_VISIBLE(FormattedLineEdit)
	SL_DECLARE_SIZE_HINT(FormattedLineEdit)
	
	virtual QSize minimumSizeHint() const;
	
	virtual void setDataType(int dataType) { FormattedLineEdit::setDataType(dataType); }
	virtual int dataType() { return FormattedLineEdit::dataType(); }

	virtual bool isModifyEvent(QEvent *event);
	virtual bool canModify(QWidget *widget) { return WidgetInterface::canModify(this); }
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool canFocusOut(QWidget *oldFocus, QWidget *newFocus);
	
	virtual bool modifyClipboardOnPaste(QString& text);

public slots:
	void handleTextModified(const QString& text, int completion);
	void handleReturnPressed();
	void handleIconClicked();
	void handleContextMenu();
};


#endif
