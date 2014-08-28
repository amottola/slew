#ifndef __image_h__
#define __image_h__


#include "slew.h"


class Image_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT

public:
	SL_DECLARE_OBJECT(Image)
	
	SL_DECLARE_SET_VISIBLE(QWidget)
	
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const { return sizeHint(); }
	
	void setPixmap(const QPixmap& pixmap);
	void setOpacity(double opacity);
	
protected:
	virtual void paintEvent(QPaintEvent *event);
	
private:
	QPixmap			fPixmap;
	double			fOpacity;
};


#endif
