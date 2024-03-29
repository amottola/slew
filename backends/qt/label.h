#ifndef __label_h__
#define __label_h__


#include "slew.h"

#include "constants/label.h"

#include <QLabel>
#include <QStaticText>
#include <QFontMetrics>
#include <QPaintEvent>
#include <QResizeEvent>


class Label_Impl : public QLabel, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(Label)
	
	SL_DECLARE_SET_VISIBLE(QLabel)
	
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;

	void setText(const QString& text);

	void setElided(bool elided);
	bool elided() { return fElided; }

	void elideText();

public slots:
	void handleLinkActivated(const QString& url);
	
protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	
	QStaticText		fStaticText;
	QString			fOriginalText;
	bool			fElided;
};


#endif
