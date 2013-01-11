#ifndef __foldpanel_h__
#define __foldpanel_h__


#include "slew.h"

#include "constants/foldpanel.h"

#include <QTimeLine>
#include <QShortcut>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QSizeGrip>

class FoldPanel_Expander;
class FoldPanel_Content;

class FoldPanel_Impl : public QWidget, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(FoldPanel)
	
	SL_DECLARE_SET_VISIBLE(QWidget)
	
	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	
	QWidget *content() { return (QWidget *)fContent; }
	bool isExpanded() { return fIsExpanded; }
	
	void setLabel(const QString& label) { fLabel = label; update(); }
	QString label() { return fLabel; }
	
	void setAnimated(bool animated);
	bool isAnimated() { return fTimeLine != NULL; }
	
	void setFlat(bool flat) { fFlat = flat; setupLayout(); }
	bool isFlat() { return fFlat; }
	
	void setExpandable(bool expandable) { fExpandable = expandable; setupLayout(); }
	bool isExpandable() { return fExpandable; }
	
	void setHorizontalResize(bool enabled);
	bool hasHorizontalResize() { return fHResize; }
	void setVerticalResize(bool enabled);
	bool hasVerticalResize() { return fVResize; }
	
	void setDuration(int duration) { fDuration = duration; setAnimated(fTimeLine != NULL); }
	int duration() { return fDuration; }
	
	void setShortcut(const QString& key) { fShortcut->setKey(key); }
	QString shortcut() { return fShortcut->key().toString(); }
	
	void updatePalette();

public slots:
	void setupLayout();
	void toggleExpand(bool animate = true);
	void expand(bool animate = true);
	void collapse(bool animate = true);
	void handleFrameChanged(int frame);
	void handleExpanded();
	
protected:
	virtual void focusInEvent(QFocusEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	QSize sizeFromContents(const QSize& size) const;
	QSize animateSize(const QSize& size) const;
	
	FoldPanel_Expander		*fExpander;
	FoldPanel_Content		*fContent;
	bool					fIsExpanded;
	QString					fLabel;
	QTimeLine				*fTimeLine;
	bool					fFlat;
	bool					fExpandable;
	int						fDuration;
	QShortcut				*fShortcut;
	bool					fInAnimation;
	QSizeGrip				*fSizeGrip;
	bool					fHResize;
	bool					fVResize;
	
	friend class FoldPanel_Expander;
	friend class FoldPanel_Content;
};



#endif
