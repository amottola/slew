#ifndef __webview_h__
#define __webview_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/webview.h"

#include <QWebView>


class WebView_Impl : public QWebView, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(WebView)
	
	SL_DECLARE_SET_VISIBLE(QWebView)
	SL_DECLARE_SIZE_HINT(QWebView)
	
	void contextMenuEvent(QContextMenuEvent *event);
	
public slots:
	void handleLinkClicked(const QUrl& url);
	void handleLoadFinished(bool ok);
	void handleLoadProgress(int progress);
	void handleSelectionChanged();
	void handleTitleChanged(const QString& title);
	void handleStatusBarMessage(const QString& message);
	void handleUrlChanged(const QUrl& url);
	void handlePrintRequested(QWebFrame *frame);
	
private:
	QString		fStatusMessage;
};


#endif
