#include "slew.h"

#include "webview.h"

#include <QWebHistory>
#include <QWebSettings>
#include <QUrl>


class QWebFrame;


WebView_Impl::WebView_Impl()
	: QWebView(), WidgetInterface()
{
	QWebSettings *settings = QWebSettings::globalSettings();
	QFont font = QApplication::font();
#ifdef Q_OS_WIN
	font.setPointSizeF(ceil(font.pointSizeF() * 1.3333333333));
#endif
	settings->setFontFamily(QWebSettings::StandardFont, font.family());
	settings->setFontSize(QWebSettings::DefaultFontSize, font.pointSize());
	settings->setAttribute(QWebSettings::AutoLoadImages, true);
	
	connect(this, SIGNAL(linkClicked(const QUrl&)), this, SLOT(handleLinkClicked(const QUrl&)));
	connect(this, SIGNAL(loadFinished(bool)), this, SLOT(handleLoadFinished(bool)));
	connect(this, SIGNAL(loadProgress(int)), this, SLOT(handleLoadProgress(int)));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(handleSelectionChanged()));
	connect(this, SIGNAL(statusBarMessage(const QString&)), this, SLOT(handleStatusBarMessage(const QString&)));
	connect(this, SIGNAL(titleChanged(const QString&)), this, SLOT(handleTitleChanged(const QString&)));
	connect(this, SIGNAL(urlChanged(const QUrl&)), this, SLOT(handleUrlChanged(const QUrl&)));
	connect(page(), SIGNAL(printRequested(QWebFrame *)), this, SLOT(handlePrintRequested(QWebFrame *)));
}


void
WebView_Impl::contextMenuEvent(QContextMenuEvent *event)
{
	if (!hasFocus())
		return;
	bool showDefault = true;
	EventRunner runner(this, "onContextMenu");
	if (runner.isValid()) {
		runner.set("pos", QCursor::pos());
		runner.set("modifiers", getKeyModifiers(QApplication::keyboardModifiers()));
		showDefault = !runner.run();
	}
	if (showDefault) {
		QWebView::contextMenuEvent(event);
	}
}


void
WebView_Impl::handleLinkClicked(const QUrl& url)
{
	EventRunner runner(this, "onActivate");
	if (runner.isValid()) {
		runner.set("url", url.toString());
		runner.run();
	}
}


void
WebView_Impl::handleLoadFinished(bool ok)
{
	EventRunner runner(this, "onLoad");
	if (runner.isValid()) {
		runner.set("complete", ok);
		runner.set("progress", Py_None, false);
		runner.run();
	}
}


void
WebView_Impl::handleLoadProgress(int progress)
{
	EventRunner runner(this, "onLoad");
	if (runner.isValid()) {
		runner.set("complete", false);
		runner.set("progress", progress);
		runner.run();
	}
}


void
WebView_Impl::handleSelectionChanged()
{
	EventRunner runner(this, "onSelect");
	if (runner.isValid()) {
		runner.set("selection", selectedText());
		runner.run();
	}
}


void
WebView_Impl::handleStatusBarMessage(const QString& message)
{
	fStatusMessage = message;
	handleTitleChanged(title());
}


void
WebView_Impl::handleTitleChanged(const QString& title)
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("title", title);
		runner.set("status", fStatusMessage);
		runner.set("url", url().toString());
		runner.run();
	}
}


void
WebView_Impl::handleUrlChanged(const QUrl& url)
{
	handleTitleChanged(title());
}


void
WebView_Impl::handlePrintRequested(QWebFrame *frame)
{
	EventRunner(this, "onPrint").run();
}


SL_DEFINE_METHOD(WebView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	QWebSettings *settings = QWebSettings::globalSettings();
	if (settings->testAttribute(QWebSettings::JavascriptEnabled))
		style |= SL_WEBVIEW_STYLE_JAVASCRIPT;
	if (settings->testAttribute(QWebSettings::PrivateBrowsingEnabled))
		style |= SL_WEBVIEW_STYLE_PRIVATE;
	if (settings->testAttribute(QWebSettings::PluginsEnabled))
		style |= SL_WEBVIEW_STYLE_PLUGINS;
	if (impl->page()->linkDelegationPolicy() != QWebPage::DelegateAllLinks)
		style |= SL_WEBVIEW_STYLE_AUTO_LINKS;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(WebView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	QWebSettings *settings = QWebSettings::globalSettings();
	settings->setAttribute(QWebSettings::JavascriptEnabled, style & SL_WEBVIEW_STYLE_JAVASCRIPT ? true : false);
	settings->setAttribute(QWebSettings::PrivateBrowsingEnabled, style & SL_WEBVIEW_STYLE_PRIVATE ? true : false);
	settings->setAttribute(QWebSettings::PluginsEnabled, style & SL_WEBVIEW_STYLE_PLUGINS ? true : false);
	impl->page()->setLinkDelegationPolicy(style & SL_WEBVIEW_STYLE_AUTO_LINKS ? QWebPage::DontDelegateLinks : QWebPage::DelegateAllLinks);
})


SL_DEFINE_METHOD(WebView, get_url, {
	return createStringObject(impl->url().toString());
})


SL_DEFINE_METHOD(WebView, set_url, {
	QString url;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &url))
		return NULL;
	
	impl->setUrl(QUrl(url));
})


SL_DEFINE_METHOD(WebView, get_title, {
	return createStringObject(impl->title());
})


SL_DEFINE_METHOD(WebView, get_icon, {
	return createIconObject(impl->icon());
})


SL_DEFINE_METHOD(WebView, get_selected_text, {
	return createStringObject(impl->selectedText());
})


SL_DEFINE_METHOD(WebView, get_selected_html, {
	return createStringObject(impl->selectedHtml());
})


SL_DEFINE_METHOD(WebView, set_html, {
	QString html, base_url;
	
	if (!PyArg_ParseTuple(args, "O&O&", convertString, &html, convertString, &base_url))
		return NULL;
	
	impl->setHtml(html, QUrl(base_url));
})


SL_DEFINE_METHOD(WebView, get_history, {
	QWebHistory *history = impl->history();
	PyObject *tuple = PyTuple_New(history->count());
	
	for (int i = 0; i < history->count(); i++) {
		QWebHistoryItem item = history->itemAt(i);
		PyObject *object = PyTuple_New(3);
		PyTuple_SET_ITEM(object, 0, createStringObject(item.url().toString()));
		PyTuple_SET_ITEM(object, 1, createStringObject(item.title()));
		PyTuple_SET_ITEM(object, 2, createIconObject(item.icon()));
		PyTuple_SET_ITEM(tuple, (Py_ssize_t)i, object);
	}
	return tuple;
})


SL_DEFINE_METHOD(WebView, clear_history, {
	QWebHistory *history = impl->history();
	history->clear();
})


SL_DEFINE_METHOD(WebView, get_current_history_index, {
	return PyInt_FromLong(impl->history()->currentItemIndex());
})


SL_DEFINE_METHOD(WebView, set_current_history_index, {
	int index;
	
	if (!PyArg_ParseTuple(args, "i", &index))
		return NULL;
	
	QWebHistory *history = impl->history();
	if ((index < 0) || (index >= history->count())) {
		PyErr_SetString(PyExc_ValueError, "invalid history index");
		return NULL;
	}
	history->goToItem(history->itemAt(index));
})


SL_DEFINE_METHOD(WebView, clear_cache, {
	QWebSettings::clearIconDatabase();
	QWebSettings::clearMemoryCaches();
})


SL_DEFINE_METHOD(WebView, back, {
	impl->back();
})


SL_DEFINE_METHOD(WebView, forward, {
	impl->forward();
})


SL_DEFINE_METHOD(WebView, stop, {
	impl->stop();
})


SL_DEFINE_METHOD(WebView, reload, {
	impl->reload();
})


SL_DEFINE_METHOD(WebView, print_document, {
	int type;
	QString title;
	bool prompt;
	PyObject *settings, *parent;
	
	if (!PyArg_ParseTuple(args, "iO&O&OO", &type, convertString, &title, convertBool, &prompt, &settings, &parent))
		return NULL;
	
	return printDocument(type, title, NULL, prompt, settings, parent, impl);
})


SL_START_PROXY_DERIVED(WebView, Window)
SL_METHOD(get_title)
SL_METHOD(get_icon)
SL_METHOD(get_selected_text)
SL_METHOD(get_selected_html)
SL_METHOD(set_html)
SL_METHOD(get_history)
SL_METHOD(clear_history)
SL_METHOD(get_current_history_index)
SL_METHOD(set_current_history_index)
SL_METHOD(clear_cache)
SL_METHOD(back)
SL_METHOD(forward)
SL_METHOD(stop)
SL_METHOD(reload)
SL_METHOD(print_document)

SL_PROPERTY(style)
SL_PROPERTY(url)
SL_END_PROXY_DERIVED(WebView, Window)


#include "webview.moc"
#include "webview_h.moc"

