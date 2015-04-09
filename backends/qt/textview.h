#ifndef __textview_h__
#define __textview_h__


#include "slew.h"

#include "constants/window.h"
#include "constants/textview.h"

#include <QTextBrowser>
#include <QUrl>
#include <QSyntaxHighlighter>
#include <QMenu>
#include <QRegExp>



class TextView_Impl : public QTextBrowser, public WidgetInterface
{
	Q_OBJECT
	
public:
	SL_DECLARE_OBJECT(TextView)
	
	SL_DECLARE_SET_VISIBLE(QTextEdit)
	SL_DECLARE_SIZE_HINT(QTextEdit)

	void setMaxLength(int length) { fMaxLength = length; }
	int maxLength() { return fMaxLength; }
	
	void setValidator(QRegExpValidator *v) { fValidator = v; }
	QRegExpValidator *validator() { return fValidator; }
	
	void setLineNumberArea(bool enabled);
	void lineNumberAreaPaintEvent(QPaintEvent *event);
	int lineNumberAreaWidth();
	
	void setCurrentLineColor(const QColor& color) { fLineHighlightColor = color; highlightLines(); }
	QColor currentLineColor() { return fLineHighlightColor; }
	
	void setHighlighter(QSyntaxHighlighter *highlighter) { delete fHighlighter; fHighlighter = highlighter; }
	void setHighlightedLines(const QHash<int, QColor>& lines) { fHighlightedLines = lines; highlightLines(); }
	
	bool isUndoAvailable() { return fUndoAvailable; }
	bool isRedoAvailable() { return fRedoAvailable; }
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool isModifyEvent(QEvent *event);
	
	QMenu *createContextMenu();

	void fromValue(const QString& value);
	QString toValue();

	QTextCursor insertObject(PyObject *object, int start = -1, int length = -1);
	QTextCursor insertObject(PyObject *object, QTextCursor& cursor);
	void setObjectLookupRegExp(const QString& regexp) { fRegExp = QRegExp(regexp); }

public slots:
	void handleTextChanged();
	void updateLineNumberAreaWidth();
	void highlightLines();
	void updateLineNumberArea();
	void handleUndoAvailable(bool available) { fUndoAvailable = available; }
	void handleRedoAvailable(bool available) { fRedoAvailable = available; }
	
	void handleUndo();
	void handleRedo();
	void handleCut();
	void handlePaste();
	void handleDelete();
	void handleAnchorClicked(const QUrl& link);

protected:
	virtual void keyPressEvent(QKeyEvent *event);
	virtual bool canInsertFromMimeData(const QMimeData *source) const;
	virtual void resizeEvent(QResizeEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	
	bool validate(const QString& text) const;
	bool canCut();

private:
	int						fMaxLength;
	QRegExpValidator		*fValidator;
	QWidget					*fLineNumberArea;
	QSyntaxHighlighter		*fHighlighter;
	QHash<int, QColor>		fHighlightedLines;
	bool					fUndoAvailable;
	bool					fRedoAvailable;
	QColor					fLineHighlightColor;
	QRegExp					fRegExp;
	bool					fHasCustomObjects;
};


#endif
