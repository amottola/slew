#include "slew.h"
#include "objects.h"

#include "textview.h"

#include <QKeyEvent>
#include <QClipboard>
#include <QScrollBar>
#include <QAbstractTextDocumentLayout>
#include <QTextObjectInterface>



const int kCustomObjectType = QTextFormat::UserObject + 1;
const int kCustomObjectPixmapProp = 1;
const int kCustomObjectTextProp = 2;



#ifdef Q_MOC_RUN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
Q_DECLARE_INTERFACE(QTextObjectInterface, "org.qt-project.Qt.QTextObjectInterface")
#else
Q_DECLARE_INTERFACE(QTextObjectInterface, "com.trolltech.Qt.QTextObjectInterface")
#endif
#endif


class CustomObject : public QObject, public QTextObjectInterface
{
	Q_OBJECT
	Q_INTERFACES(QTextObjectInterface)

public:
	QSizeF intrinsicSize(QTextDocument *doc, int posInDocument, const QTextFormat& format)
	{
		QPixmap pixmap = qvariant_cast<QPixmap>(format.property(kCustomObjectPixmapProp));
		return QSizeF(pixmap.size());
	}

	void drawObject(QPainter *painter, const QRectF& rect, QTextDocument *doc, int posInDocument, const QTextFormat& format)
	{
		QPixmap pixmap = qvariant_cast<QPixmap>(format.property(kCustomObjectPixmapProp));
		painter->drawPixmap(rect.toRect(), pixmap);
	}
};



class SyntaxHighlighter_Python : public QSyntaxHighlighter
{
	Q_OBJECT
	
public:
	SyntaxHighlighter_Python(QTextDocument *document)
		: QSyntaxHighlighter(document)
	{
		const char *keywords[] = { "and", "assert", "break", "class", "continue", "def", "del", "elif", "else", "except", "exec", "finally", "for", "from", "global", "if", "import", "in", "is",
								   "lambda", "not", "or", "pass", "print", "raise", "return", "try", "while", "yield", "None", "True", "False", NULL };
		const char *operators[] = { "=", "==", "!=", "<", "<=", ">", ">=", "\\+", "-", "\\*", "/", "//", "\\%", "\\*\\*", "\\+=", "-=", "\\*=", "/=", "\\%=", "\\^", "\\|", "\\&", "\\~", ">>", "<<", NULL };
		const char *braces[] = { "\\{", "\\}", "\\(", "\\)", "\\[", "\\]", NULL };
		int i;
		QTextCharFormat keywordStyle;
		QTextCharFormat operatorStyle;
		QTextCharFormat braceStyle;
		QTextCharFormat defclassStyle;
		QTextCharFormat stringStyle;
		QTextCharFormat stringMultilineStyle;
		QTextCharFormat commentStyle;
		QTextCharFormat specialCommentStyle;
		
		keywordStyle.setForeground(Qt::blue);
		operatorStyle.setForeground(Qt::red);
		braceStyle.setForeground(Qt::darkGray);
		defclassStyle.setForeground(Qt::black);
		defclassStyle.setFontWeight(QFont::Bold);
		stringStyle.setForeground(Qt::magenta);
		stringMultilineStyle.setForeground(Qt::darkMagenta);
		commentStyle.setForeground(Qt::darkGreen);
		commentStyle.setFontItalic(true);
		specialCommentStyle.setForeground(Qt::darkGreen);
		specialCommentStyle.setFontItalic(true);
		specialCommentStyle.setFontWeight(QFont::Bold);
		
		for (i = 0; keywords[i]; i++)
			fRules.append(Rule(QString("\\b%1\\b").arg(keywords[i]), 0, keywordStyle));
		for (i = 0; operators[i]; i++)
			fRules.append(Rule(operators[i], 0, operatorStyle));
		for (i = 0; braces[i]; i++)
			fRules.append(Rule(braces[i], 0, braceStyle));
		
		fRules.append(Rule("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"", 0, stringStyle));
		fRules.append(Rule("'[^'\\\\]*(\\\\.[^'\\\\]*)*'", 0, stringStyle));
		fRules.append(Rule("\\bdef\\b\\s*(\\w+)", 1, defclassStyle));
		fRules.append(Rule("\\bclass\\b\\s*(\\w+)", 1, defclassStyle));
		fRules.append(Rule("#[^\\n]*", 0, commentStyle));
		fRules.append(Rule("#\\s*\\-\\*\\-\\s*\\w+\\s*\\:(.*)\\-\\*\\-", 1, specialCommentStyle));
		
		fMultiline[0] = Rule("'''", 1, stringMultilineStyle);
		fMultiline[1] = Rule("\"\"\"", 2, stringMultilineStyle);
	}

	virtual void highlightBlock(const QString& text)
	{
		foreach (Rule rule, fRules) {
			int length, index = rule.fRegExp.indexIn(text);
			while (index >= 0) {
				index = rule.fRegExp.pos(rule.fState);
				length = rule.fRegExp.cap(rule.fState).length();
				setFormat(index, length, rule.fStyle);
				index = rule.fRegExp.indexIn(text, index + length);
			}
		}
		
		setCurrentBlockState(0);
		if (!matchMultiline(text, fMultiline[0]))
			matchMultiline(text, fMultiline[1]);
	}
	
private:
	typedef struct Rule
	{
		Rule() {}
		Rule(const QString& pattern, int state, const QTextCharFormat& style)
			: fState(state), fStyle(style) { fRegExp.setPattern(pattern); }
		
		QRegExp				fRegExp;
		int					fState;
		QTextCharFormat		fStyle;
	} Rule;
	
	bool matchMultiline(const QString& text, const Rule& rule)
	{
		int start, end, add, length;
		
		if (rule.fState == previousBlockState()) {
			start = add = 0;
		}
		else {
			start = rule.fRegExp.indexIn(text);
			add = rule.fRegExp.matchedLength();
		}
		while (start >= 0) {
			end = rule.fRegExp.indexIn(text, start + add);
			if (end >= add) {
				length = end - start + add + rule.fRegExp.matchedLength();
				setCurrentBlockState(0);
			}
			else {
				length = text.length() - start + add;
				setCurrentBlockState(rule.fState);
			}
			setFormat(start, length, rule.fStyle);
			start = rule.fRegExp.indexIn(text, start + length);
		}
		return currentBlockState() == rule.fState;
	}
	
	QList<Rule>				fRules;
	Rule					fMultiline[2];
};



class SyntaxHighlighter_XML : public QSyntaxHighlighter
{
	Q_OBJECT
	
	enum HighlightType
	{
		SyntaxChar,
        ElementName,
		Comment,
		AttributeName,
		AttributeValue,
		Error,
		Other
	};
	enum ParsingState
	{
		NoState = 0,
		ExpectElementNameOrSlash,
		ExpectElementName,
		ExpectAttributeOrEndOfElement,
		ExpectEqual,
		ExpectAttributeValue
	};
	enum BlockState
	{
		NoBlock = 0,
		InComment,
		InElement,
		InCDATA,
	};
	
public:
	SyntaxHighlighter_XML(QTextDocument *document)
		: QSyntaxHighlighter(document)
	{
		fNameExp = QRegExp("([A-Za-z_:]|[^\\x00-\\x7F])([A-Za-z0-9_:.-]|[^\\x00-\\x7F])*");
		fCommentExp = QRegExp("<!--[^-]*-([^-][^-]*-)*->");
		fCommentBeginExp = QRegExp("<!--");
		fCommentEndExp = QRegExp("[^-]*-([^-][^-]*-)*->");
		fCDATAExp = QRegExp("<!\\[CDATA\\[[^\\]]*\\]([^\\]][^\\]]*\\])*\\]>");
		fCDATABeginExp = QRegExp("<!\\[CDATA\\[");
		fCDATAEndExp = QRegExp("[^\\]]*\\]([^\\]][^\\]]*\\])*\\]>");
		fAttribExp = QRegExp("\"[^<\"]*\"|'[^<']*'");

		fmtSyntaxChar.setForeground(Qt::blue);
		fmtElementName.setForeground(Qt::blue);
		fmtElementName.setFontWeight(QFont::Bold);
		fmtComment.setForeground(Qt::darkGreen);
		fmtComment.setFontItalic(true);
		fmtAttributeName.setForeground(Qt::darkMagenta);
		fmtAttributeValue.setForeground(Qt::darkRed);
		fmtCDATA.setForeground(Qt::gray);
		fmtError.setForeground(Qt::red);
		fmtOther.setForeground(Qt::black);

	}
	
	void highlightBlock(const QString& text)
	{
		int i = 0;
		int pos = 0;
		int brackets = 0;
		
		fState = (previousBlockState() == InElement ? ExpectAttributeOrEndOfElement : NoState);
		
		if (previousBlockState() == InComment) {
			pos = fCommentEndExp.indexIn(text, i);
			if (pos >= 0) {
				const int iLength = fCommentEndExp.matchedLength();
				setFormat(0, iLength - 3, fmtComment);
				setFormat(iLength - 3, 3, fmtSyntaxChar);
				i = pos + iLength;
				setCurrentBlockState(NoState);
			}
			else {
				setFormat(0, text.length(), fmtComment);
				setCurrentBlockState(InComment);
				return;
			}
		}
		else if (previousBlockState() == InCDATA) {
			pos = fCDATAEndExp.indexIn(text, i);
			if (pos >= 0) {
				const int iLength = fCDATAEndExp.matchedLength();
				setFormat(0, iLength - 3, fmtCDATA);
				setFormat(iLength - 3, 3, fmtSyntaxChar);
				i = pos + iLength;
				setCurrentBlockState(NoState);
			}
			else {
				setFormat(0, text.length(), fmtCDATA);
				setCurrentBlockState(InCDATA);
				return;
			}
		}
		
		for (; i < text.length(); i++) {
			switch (text.at(i).toLatin1()) {
			case '<':
				brackets++;
				if (brackets == 1) {
					setFormat(i, 1, fmtSyntaxChar);
					fState = ExpectElementNameOrSlash;
				}
				else
					setFormat(i, 1, fmtError);
				break;
		
			case '>':
				brackets--;
				if (brackets == 0)
					setFormat(i, 1, fmtSyntaxChar);
				else
					setFormat( i, 1, fmtError);
				fState = NoState;
				break;
		
			case '/':
				if (fState == ExpectElementNameOrSlash) {
					fState = ExpectElementName;
					setFormat(i, 1, fmtSyntaxChar);
				}
				else {
					if (fState == ExpectAttributeOrEndOfElement)
						setFormat(i, 1, fmtSyntaxChar);
					else
						processDefaultText(i, text);
				}
				break;
		
			case '=':
				if (fState == ExpectEqual) {
					fState = ExpectAttributeValue;
					setFormat(i, 1, fmtOther);
				}
				else
					processDefaultText(i, text);  
				break;
		
			case '\'':
			case '\"':
				if (fState == ExpectAttributeValue) {
					pos = fAttribExp.indexIn(text, i);
					if (pos == i) {
						const int iLength = fAttribExp.matchedLength();
						setFormat(i, 1, fmtOther);
						setFormat(i + 1, iLength - 2, fmtAttributeValue);
						setFormat(i + iLength - 1, 1, fmtOther);
						i += iLength - 1;
						fState = ExpectAttributeOrEndOfElement;
					}
					else
						processDefaultText(i, text);
				}
				else
					processDefaultText(i, text);
				break;
		
			case '!':
				if (fState == ExpectElementNameOrSlash) {
					pos = fCommentExp.indexIn(text, i - 1);
					if (pos == i - 1) {
						const int iLength = fCommentExp.matchedLength();
						setFormat(pos, 4, fmtSyntaxChar);
						setFormat(pos + 4, iLength - 7, fmtComment);
						setFormat(pos + iLength - 3, 3, fmtSyntaxChar);
						i += iLength - 2;
						fState = NoState;
						brackets--;
					}
					else {
						pos = fCommentBeginExp.indexIn(text, i - 1);
						if (pos >= i - 1) {
							setFormat(i, 3, fmtSyntaxChar);
							setFormat(i + 3, text.length() - i - 3, fmtComment);
							setCurrentBlockState(InComment);
							return;
						}
						else {
							pos = fCDATAExp.indexIn(text, i - 1);
							if (pos == i - 1) {
								const int iLength = fCDATAExp.matchedLength();
								setFormat(pos, 9, fmtSyntaxChar);
								setFormat(pos + 9, iLength - 12, fmtCDATA);
								setFormat(pos + iLength - 3, 3, fmtSyntaxChar);
								i += iLength - 2;
								fState = NoState;
								brackets--;
							}
							else {
								pos = fCDATABeginExp.indexIn(text, i - 1);
								if (pos >= i - 1) {
									setFormat(i, 8, fmtSyntaxChar);
									setFormat(i + 8, text.length() - i - 9, fmtCDATA);
									setCurrentBlockState(InCDATA);
									return;
								}
								else
									processDefaultText(i, text);
							}
						}
					}
				}
				else
					processDefaultText(i, text);
				break;  
		
			default:
				const int iLength = processDefaultText(i, text);
				if (iLength > 0)
					i += iLength - 1;
				break;
			}
		}
		if (fState == ExpectAttributeOrEndOfElement)
			setCurrentBlockState(InElement);
		else
			setCurrentBlockState(NoState);
	}
	
	int processDefaultText(int i, const QString& text)
	{
		int iLength = 0;
		switch(fState) {
		case ExpectElementNameOrSlash:
		case ExpectElementName:
			{
				const int pos = fNameExp.indexIn(text, i);
				if (pos == i) {
					iLength = fNameExp.matchedLength();
					setFormat(pos, iLength, fmtElementName);
					fState = ExpectAttributeOrEndOfElement;
				}
				else
					setFormat(i, 1, fmtOther);
			}  
			break;
	
		case ExpectAttributeOrEndOfElement:
			{
				const int pos = fNameExp.indexIn(text, i);
				if (pos == i) {
					iLength = fNameExp.matchedLength();
					setFormat(pos, iLength, fmtAttributeName);
					fState = ExpectEqual;
				}
				else
					setFormat(i, 1, fmtOther);
			}
			break;
	
		default:
			setFormat(i, 1, fmtOther);
			break;
		}
		return iLength;
	}

private:
	QRegExp			fCommentExp;
	QRegExp			fCommentBeginExp;
	QRegExp			fCommentEndExp;
	QRegExp			fCDATAExp;
	QRegExp			fCDATABeginExp;
	QRegExp			fCDATAEndExp;
	QRegExp			fAttribExp;
	QRegExp			fNameExp;
	QTextCharFormat fmtSyntaxChar;
    QTextCharFormat fmtElementName;
	QTextCharFormat fmtComment;
	QTextCharFormat fmtAttributeName;
	QTextCharFormat fmtAttributeValue;
	QTextCharFormat fmtCDATA;
	QTextCharFormat fmtError;
	QTextCharFormat fmtOther;
	
	ParsingState	fState;
};



class LineNumberArea : public QWidget
{
	Q_OBJECT
	
public:
	LineNumberArea(TextView_Impl *editor) : QWidget(editor), fCodeEditor(editor) {}
	
	QSize sizeHint() const { return QSize(fCodeEditor->lineNumberAreaWidth(), 0); }

protected:
	void paintEvent(QPaintEvent *event) { fCodeEditor->lineNumberAreaPaintEvent(event); }

private:
	TextView_Impl			*fCodeEditor;
};



TextView_Impl::TextView_Impl()
	: QTextBrowser(), WidgetInterface(), fMaxLength(-1), fValidator(NULL), fLineNumberArea(NULL), fHighlighter(NULL), fUndoAvailable(false), fRedoAvailable(false), fLineHighlightColor(QColor(Qt::yellow).lighter(160)), fHasCustomObjects(false)
{
	connect(this, SIGNAL(textChanged()), this, SLOT(handleTextChanged()));
	connect(this, SIGNAL(textChanged()), this, SLOT(updateLineNumberAreaWidth()));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateLineNumberArea()));
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), this, SLOT(updateLineNumberArea()));
	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightLines()));
	connect(this, SIGNAL(undoAvailable(bool)), this, SLOT(handleUndoAvailable(bool)));
	connect(this, SIGNAL(redoAvailable(bool)), this, SLOT(handleRedoAvailable(bool)));
	connect(this, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(handleAnchorClicked(const QUrl&)));
	setAcceptRichText(false);
	setOpenLinks(false);
	setOpenExternalLinks(false);
	setReadOnly(false);
	setUndoRedoEnabled(true);

	QObject *customInterface = new CustomObject;
	customInterface->setParent(this);
	document()->documentLayout()->registerHandler(kCustomObjectType, customInterface);
}


void
TextView_Impl::setLineNumberArea(bool enabled)
{
	if ((fLineNumberArea) && (!enabled)) {
		delete fLineNumberArea;
		fLineNumberArea = NULL;
	}
	else if ((!fLineNumberArea) && (enabled)) {
		fLineNumberArea = new LineNumberArea(this);
	}
	updateLineNumberAreaWidth();
	highlightLines();
}


int
TextView_Impl::lineNumberAreaWidth()
{
	if (!fLineNumberArea)
		return 0;
	
	int digits = 1;
	int max = qMax(1, document()->blockCount());
	while (max >= 10) {
		max /= 10;
		++digits;
	}
	return 13 + fontMetrics().width(QLatin1Char('9')) * digits;
}


void
TextView_Impl::updateLineNumberAreaWidth()
{ 
	if (fLineNumberArea) {
		setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
		fLineNumberArea->update();
	}
}


void
TextView_Impl::updateLineNumberArea()
{
	if (fLineNumberArea) {
		fLineNumberArea->update();
	}
}


void
TextView_Impl::resizeEvent(QResizeEvent *e)
{
	QTextBrowser::resizeEvent(e);
	if (fLineNumberArea) {
		QRect cr = contentsRect();
		fLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
	}
}


void
TextView_Impl::highlightLines()
{
	QList<QTextEdit::ExtraSelection> extraSelections;
	
	if ((!isReadOnly()) && (fLineNumberArea)) {
		QTextEdit::ExtraSelection selection;
		
		int line = textCursor().blockNumber() + 1;
		if (!fHighlightedLines.contains(line)) {
			selection.format.setBackground(fLineHighlightColor);
			selection.format.setProperty(QTextFormat::FullWidthSelection, true);
			selection.cursor = textCursor();
			selection.cursor.clearSelection();
			extraSelections.append(selection);
		}
	}
	
	for (QHash<int, QColor>::const_iterator it = fHighlightedLines.constBegin(); it != fHighlightedLines.constEnd(); it++) {
		QTextEdit::ExtraSelection selection;
		
		selection.format.setBackground(it.value());
		selection.format.setProperty(QTextFormat::FullWidthSelection, true);
		selection.cursor = QTextCursor(document()->findBlockByLineNumber(it.key() - 1));
		selection.cursor.clearSelection();
		extraSelections.append(selection);
	}
	
	setExtraSelections(extraSelections);
}


void
TextView_Impl::lineNumberAreaPaintEvent(QPaintEvent *event)
{
	QPainter painter(fLineNumberArea);
	painter.fillRect(event->rect(), Qt::lightGray);
	
	QTextDocument *doc = document();
	QAbstractTextDocumentLayout *layout = doc->documentLayout();
	QTextBlock block = doc->findBlock(layout->hitTest(QPointF(fLineNumberArea->width(), verticalScrollBar()->value()), Qt::FuzzyHit));
// 	QTextBlock block = doc->findBlockByLineNumber(verticalScrollBar()->value());
	if (!block.isValid())
		return;
	int blockNumber = block.blockNumber();
	QRectF bounds;
	QPointF offset(doc->documentMargin(), -verticalScrollBar()->value());
	painter.setPen(Qt::black);
	do {
		bounds = layout->blockBoundingRect(block).translated(offset);
		if (block.isVisible() && (bounds.top() <= event->rect().bottom())) {
			painter.drawText(0, bounds.top(), fLineNumberArea->width() - 10, bounds.height(), Qt::AlignRight|Qt::AlignTop, QString::number(blockNumber + 1));
		}
		block = block.next();
		blockNumber++;
	} while (block.isValid());
}


bool
TextView_Impl::validate(const QString& text) const
{
	if (!fValidator)
		return true;
	
	QTextDocument *doc = document()->clone();
	QTextCursor cursor(doc);
	int pos = cursor.position();
	cursor.insertText(text);
	QString test = doc->toPlainText();
	
	delete doc;
	
	return fValidator->validate(test, pos) != QValidator::Invalid;
}


void
TextView_Impl::keyPressEvent(QKeyEvent *event)
{
	QString text(event->text());
	
	if ((fMaxLength >= 0) && (!text.isEmpty()) && ((text[0].isPrint()) || (text[0].isSpace()))) {
		if ((document()->characterCount() + text.length() - 1 > fMaxLength) && (!textCursor().hasSelection()))
			return;
	}
	
	if (fValidator) {
		QString text, oldText = toPlainText();
		QTextCursor cursor = textCursor();
		int pos = cursor.position();
		
		QTextEdit::keyPressEvent(event);
		
		text = toPlainText();
		if (fValidator->validate(text, pos) == QValidator::Invalid) {
			setPlainText(oldText);
			setTextCursor(cursor);
			return;
		}
	}
	else {
		QTextEdit::keyPressEvent(event);
	}
}


bool
TextView_Impl::canInsertFromMimeData(const QMimeData *source) const
{
	if (!validate(source->text()))
		return false;
	return QTextBrowser::canInsertFromMimeData(source);
}


void
TextView_Impl::insertFromMimeData(const QMimeData *source)
{
	if (!fRegExp.isEmpty()) {
		QTextCursor cursor = textCursor();
		QString text = source->text();
		if ((!text.isEmpty()) && (fRegExp.indexIn(text) >= 0)) {
			EventRunner runner(this, "onCreateObject");
			runner.set("text", text);
			runner.set("object", Py_None, false);
			if (runner.run()) {
				PyObject *object;
				if (!runner.get("object", &object))
					object = Py_None;
				if (insertObject(object, cursor).isNull())
					PyErr_Clear();
				else {
					fHasCustomObjects = true;
					return;
				}
			}
		}
	}
	QTextBrowser::insertFromMimeData(source);
}


bool
TextView_Impl::isFocusOutEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			if (!tabChangesFocus())
				return false;
		}
		/* fallthrough */
		
	case QEvent::MouseButtonPress:
	case QEvent::MouseButtonDblClick:
	case QEvent::TouchBegin:
	case QEvent::Wheel:
		return true;
	default:
		break;
	}
	return false;
}


bool
TextView_Impl::isModifyEvent(QEvent *event)
{
	switch (event->type()) {
	case QEvent::KeyPress:
		{
			QKeyEvent *e = (QKeyEvent *)event;
			if ((e->key() == Qt::Key_Delete) || (e->key() == Qt::Key_Backspace))
				return true;
			if (((e == QKeySequence::Undo) && (isUndoAvailable())) ||
				((e == QKeySequence::Redo) && (isRedoAvailable())) ||
				(e == QKeySequence::AddTab) ||
				(e == QKeySequence::Bold) ||
				((e == QKeySequence::Cut) && (canCut())) ||
				(e == QKeySequence::Delete) ||
				(e == QKeySequence::DeleteEndOfLine) ||
				(e == QKeySequence::DeleteEndOfWord) ||
				(e == QKeySequence::DeleteStartOfWord) ||
				(e == QKeySequence::InsertLineSeparator) ||
				(e == QKeySequence::InsertParagraphSeparator) ||
				(e == QKeySequence::Italic) ||
				((e == QKeySequence::Paste) && (canPaste())) ||
				(e == QKeySequence::Underline))
				return true;
			return !e->text().isEmpty();
		}
		break;
	default:
		break;
	}
	return false;
}


void
TextView_Impl::contextMenuEvent(QContextMenuEvent *event)
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
		QMenu *menu = createContextMenu();
		menu->setAttribute(Qt::WA_DeleteOnClose);
		menu->popup(QCursor::pos());
	}
}


QMenu *
TextView_Impl::createContextMenu()
{
	QMenu *popup = new QMenu();
	QAction *action;
	
	if (!isReadOnly()) {
		action = popup->addAction(QLineEdit::tr("&Undo") + SL_ACCEL_KEY(QKeySequence::Undo));
		action->setEnabled(isUndoAvailable());
		connect(action, SIGNAL(triggered()), SLOT(handleUndo()));
		
		action = popup->addAction(QLineEdit::tr("&Redo") + SL_ACCEL_KEY(QKeySequence::Redo));
		action->setEnabled(isRedoAvailable());
		connect(action, SIGNAL(triggered()), SLOT(handleRedo()));
		
		popup->addSeparator();
		
		action = popup->addAction(QLineEdit::tr("Cu&t") + SL_ACCEL_KEY(QKeySequence::Cut));
		action->setEnabled(!isReadOnly() && textCursor().hasSelection());
		connect(action, SIGNAL(triggered()), SLOT(handleCut()));
	}
	
	action = popup->addAction(QLineEdit::tr("&Copy") + SL_ACCEL_KEY(QKeySequence::Copy));
	connect(action, SIGNAL(triggered()), SLOT(copy()));
	
	if (!isReadOnly()) {
		action = popup->addAction(QLineEdit::tr("&Paste") + SL_ACCEL_KEY(QKeySequence::Paste));
		action->setEnabled(!isReadOnly() && !QApplication::clipboard()->text().isEmpty());
		connect(action, SIGNAL(triggered()), SLOT(handlePaste()));
		
		action = popup->addAction(QLineEdit::tr("Delete"));
		action->setEnabled(!isReadOnly() && textCursor().hasSelection());
		connect(action, SIGNAL(triggered()), SLOT(handleDelete()));
	}
	
	popup->addSeparator();
	
	action = popup->addAction(QLineEdit::tr("Select All") + SL_ACCEL_KEY(QKeySequence::SelectAll));
	connect(action, SIGNAL(triggered()), SLOT(selectAll()));
	
	return popup;
}


QTextCursor
TextView_Impl::insertObject(PyObject *object, QTextCursor& cursor)
{
	QString text;
	QPixmap pixmap;
	PyObject *method = PyString_FromString("get_text");
	PyObject *result = PyObject_CallMethodObjArgs(object, method, NULL);
	Py_DECREF(method);
	if ((!result) || (!convertString(result, &text))) {
		Py_XDECREF(result);
		return QTextCursor();
	}
	Py_DECREF(result);
	method = PyString_FromString("get_bitmap");
	result = PyObject_CallMethodObjArgs(object, method, NULL);
	Py_DECREF(method);
	if ((!result) || (!convertPixmap(result, &pixmap))) {
		Py_XDECREF(result);
		return QTextCursor();
	}
	Py_DECREF(result);

	QTextCharFormat customCharFormat;
	customCharFormat.setObjectType(kCustomObjectType);
	customCharFormat.setProperty(kCustomObjectTextProp, text);
	customCharFormat.setProperty(kCustomObjectPixmapProp, pixmap);

	cursor.insertText(QString(QChar::ObjectReplacementCharacter), customCharFormat);

	fHasCustomObjects = true;
	return cursor;
}


QTextCursor
TextView_Impl::insertObject(PyObject *object, int start, int length)
{
	QTextCursor cursor = textCursor();
	if (start >= 0)
		cursor.setPosition(start, QTextCursor::MoveAnchor);
	if (length >= 0)
		cursor.setPosition(start + length, QTextCursor::KeepAnchor);
	return insertObject(object, cursor);
}


void
TextView_Impl::fromValue(const QString& value)
{
	if (acceptRichText())
		setHtml(value);
	else
		setPlainText(value);

	fHasCustomObjects = false;
	if (!fRegExp.isEmpty()) {
		QTextDocument *doc = document();
		QTextCursor cursor(doc);

		for (;;) {
			cursor = doc->find(fRegExp, cursor);
			if (cursor.isNull())
				break;
			EventRunner runner(this, "onCreateObject");
			runner.set("text", cursor.selectedText());
			runner.set("object", Py_None, false);
			if (runner.run()) {
				PyObject *object;
				if (!runner.get("object", &object))
					object = Py_None;
				if (insertObject(object, cursor).isNull())
					PyErr_Clear();
				else
					fHasCustomObjects = true;
			}
		}
	}
}


QString
TextView_Impl::toValue()
{
	QString value;
	QTextDocument *doc = document();

	if (fHasCustomObjects) {
		doc = doc->clone();
		QTextCursor cursor(doc);
		for (;;) {
			cursor = doc->find(QString(QChar::ObjectReplacementCharacter), cursor);
			if (cursor.isNull())
				break;
			if (cursor.charFormat().objectType() == kCustomObjectType) {
				QString text = qvariant_cast<QString>(cursor.charFormat().property(kCustomObjectTextProp));
				cursor.insertText(text);
			}
		}
	}

	if (acceptRichText())
		value = doc->toHtml();
	else
		value = doc->toPlainText();

	if (fHasCustomObjects)
		delete doc;

	return value;
}


bool
TextView_Impl::canCut()
{
	if (!fValidator)
		return true;

	QTextDocument *doc = document()->clone();
	QTextCursor cursor(doc);
	int pos = cursor.position();
	cursor.deleteChar();
	QString text = doc->toPlainText();
	
	delete doc;
	
	return fValidator->validate(text, pos) != QValidator::Invalid;
}


void
TextView_Impl::handleUndo()
{
	if (canModify(this))
		undo();
}


void
TextView_Impl::handleRedo()
{
	if (canModify(this))
		redo();
}


void
TextView_Impl::handleCut()
{
	if (!canCut()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		cut();
	}
}


void
TextView_Impl::handlePaste()
{
	if (!canPaste()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		paste();
	}
}


void
TextView_Impl::handleDelete()
{
	if (!canCut()) {
		QApplication::beep();
	}
	else if (canModify(this)) {
		textCursor().deleteChar();
	}
}


void
TextView_Impl::handleTextChanged()
{
	EventRunner runner(this, "onChange");
	if (runner.isValid()) {
		runner.set("value", toValue());
		runner.run();
	}
}


void
TextView_Impl::handleAnchorClicked(const QUrl& link)
{
	EventRunner runner(this, "onClick");
	if (runner.isValid()) {
		runner.set("url", link.toString());
		runner.run();
	}
}


SL_DEFINE_METHOD(TextView, cut, {
	impl->cut();
})


SL_DEFINE_METHOD(TextView, copy, {
	impl->copy();
})


SL_DEFINE_METHOD(TextView, paste, {
	impl->paste();
})


SL_DEFINE_METHOD(TextView, delete, {
	impl->textCursor().deleteChar();
})


SL_DEFINE_METHOD(TextView, insert, {
	int pos;
	PyObject *object;
	QString text;
	
	if (!PyArg_ParseTuple(args, "iO", &pos, &object))
		return NULL;
	
	QTextCursor cursor = impl->insertObject(object, pos);
	if (!cursor.isNull()) {
		impl->setTextCursor(cursor);
		Py_RETURN_NONE;
	}

	PyErr_Clear();
	if (!convertString(object, &text))
		return NULL;

	if (pos >= 0) {
		QTextCursor cursor = impl->textCursor();
		cursor.setPosition(pos, QTextCursor::MoveAnchor);
		impl->setTextCursor(cursor);
	}
	
	if (impl->acceptRichText())
		impl->insertHtml(text);
	else
		impl->insertPlainText(text);
})


SL_DEFINE_METHOD(TextView, replace, {
	int start = -1, length = -1, oldLength;
	PyObject *object;
	QString text;

	if (!PyArg_ParseTuple(args, "Oii", &object, &start, &length))
		return NULL;

	QTextCursor cursor = impl->insertObject(object, start, length);
	if (!cursor.isNull()) {
		impl->setTextCursor(cursor);
		Py_RETURN_NONE;
	}

	PyErr_Clear();
	if (!convertString(object, &text))
		return NULL;

	cursor = impl->textCursor();
	QTextCursor oldCursor = cursor;
	if (start >= 0)
		cursor.setPosition(start);
	if (length >= 0)
		cursor.setPosition(start + length, QTextCursor::KeepAnchor);
	oldLength = cursor.selectedText().length();

	impl->setTextCursor(cursor);
	if (impl->acceptRichText())
		impl->insertHtml(text);
	else
		impl->insertPlainText(text);
	if (oldCursor.position() > start + length)
		oldCursor.setPosition(oldCursor.position() + (length - oldLength));
	impl->setTextCursor(oldCursor);
})


SL_DEFINE_METHOD(TextView, is_modified, {
	return createBoolObject(impl->document()->isModified());
})


SL_DEFINE_METHOD(TextView, set_modified, {
	bool modified;
	
	if (!PyArg_ParseTuple(args, "O&", convertBool, &modified))
		return NULL;
	
	impl->document()->setModified(modified);
})


SL_DEFINE_METHOD(TextView, undo, {
	impl->undo();
})


SL_DEFINE_METHOD(TextView, redo, {
	impl->redo();
})


SL_DEFINE_METHOD(TextView, is_undo_available, {
	return createBoolObject(impl->isUndoAvailable());
})


SL_DEFINE_METHOD(TextView, is_redo_available, {
	return createBoolObject(impl->isRedoAvailable());
})


SL_DEFINE_METHOD(TextView, set_highlighted_lines, {
	PyObject *lines;
	
	if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &lines))
		return NULL;
	
	PyObject *key, *value;
	Py_ssize_t pos = 0;
	QString var, var_value;
	int lineNum;
	QColor color;
	QHash<int, QColor> data;
	
	while (PyDict_Next(lines, &pos, &key, &value)) {
		if ((!convertInt(key, &lineNum)) ||
			(!convertColor(value, &color)))
			return NULL;
		data[lineNum] = color;
	}
	impl->setHighlightedLines(data);
})


SL_DEFINE_METHOD(TextView, set_syntax, {
	QString syntax;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &syntax))
		return NULL;
	
	syntax = syntax.toLower();
	if (syntax == "python")
		impl->setHighlighter(new SyntaxHighlighter_Python(impl->document()));
	else if (syntax == "xml")
		impl->setHighlighter(new SyntaxHighlighter_XML(impl->document()));
	else
		impl->setHighlighter(NULL);
})


SL_DEFINE_METHOD(TextView, set_color, {
	QPalette palette(impl->palette());
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	if (!color.isValid()) {
		color = QApplication::palette(impl).color(impl->isEnabled() ? QPalette::Normal : QPalette::Disabled, QPalette::WindowText);
	}
	
	palette.setColor(QPalette::Text, color);
	palette.setColor(QPalette::WindowText, color);
	impl->setPalette(palette);
})


SL_DEFINE_METHOD(TextView, get_style, {
	int style = 0;
	
	getWindowStyle(impl, style);
	
	if (impl->isReadOnly())
		style |= SL_TEXTVIEW_STYLE_READONLY;
	if (impl->document()->defaultTextOption().wrapMode() == QTextOption::NoWrap)
		style |= SL_TEXTVIEW_STYLE_NOWRAP;
	if (impl->lineNumberAreaWidth() > 0)
		style |= SL_TEXTVIEW_STYLE_LINENUMS;
	if (impl->acceptRichText())
		style |= SL_TEXTVIEW_STYLE_HTML;
	if (impl->tabChangesFocus())
		style |= SL_TEXTVIEW_STYLE_TABFOCUS;
	
	return PyInt_FromLong(style);
})


SL_DEFINE_METHOD(TextView, set_style, {
	int style;
	
	if (!PyArg_ParseTuple(args, "i", &style))
		return NULL;
	
	setWindowStyle(impl, style);
	
	impl->setReadOnly(style & SL_TEXTVIEW_STYLE_READONLY ? true : false);
	QTextOption option(impl->document()->defaultTextOption());
	option.setWrapMode(style & SL_TEXTVIEW_STYLE_NOWRAP ? QTextOption::NoWrap : QTextOption::WordWrap);
	impl->document()->setDefaultTextOption(option);
	impl->setLineNumberArea(style & SL_TEXTVIEW_STYLE_LINENUMS ? true : false);
	impl->setAcceptRichText(style & SL_TEXTVIEW_STYLE_HTML ? true : false);
	impl->setTabChangesFocus(style & SL_TEXTVIEW_STYLE_TABFOCUS ? true : false);
})


SL_DEFINE_METHOD(TextView, get_selection, {
	QTextCursor cursor = impl->textCursor();
	int start = cursor.selectionStart();
	int end = cursor.selectionEnd();
	if (start == end)
		start = end = -1;
	return PyTuple_Pack(2, PyInt_FromLong(start), PyInt_FromLong(end));
})


SL_DEFINE_METHOD(TextView, set_selection, {
	int start, end;
	
	if (!PyArg_ParseTuple(args, "ii", &start, &end))
		return NULL;
	
	if ((start < 0) && (end < 0))
		impl->selectAll();
	else {
		QTextCursor cursor = impl->textCursor();
		if (start < 0)
			cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
		else
			cursor.setPosition(start, QTextCursor::MoveAnchor);
		if (end < 0)
			cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
		else
			cursor.setPosition(end, QTextCursor::KeepAnchor);
		impl->setTextCursor(cursor);
	}
})


SL_DEFINE_METHOD(TextView, get_insertion_point, {
	return PyInt_FromLong(impl->textCursor().position());
})


SL_DEFINE_METHOD(TextView, set_insertion_point, {
	int pos;
	
	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	if (pos < 0)
		impl->moveCursor(QTextCursor::End);
	else {
		QTextCursor cursor = impl->textCursor();
		cursor.setPosition(pos, QTextCursor::MoveAnchor);
		impl->setTextCursor(cursor);
	}
})


SL_DEFINE_METHOD(TextView, get_position, {
	int line, column;
	
	if (!PyArg_ParseTuple(args, "ii", &line, &column))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	QTextBlock block = impl->document()->findBlockByLineNumber(line - 1);
	return PyInt_FromLong(block.position() + column - 1);
})


SL_DEFINE_METHOD(TextView, set_object_lookup_regexp, {
	QString regexp;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &regexp))
		return NULL;
	
	impl->setObjectLookupRegExp(regexp);
})


SL_DEFINE_METHOD(TextView, get_value, {
	return createStringObject(impl->toValue());
})


SL_DEFINE_METHOD(TextView, set_value, {
	QString text;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &text))
		return NULL;
	
	impl->fromValue(text);
})


SL_DEFINE_METHOD(TextView, get_length, {
	return PyInt_FromLong(impl->maxLength());
})


SL_DEFINE_METHOD(TextView, set_length, {
	int length;
	
	if (!PyArg_ParseTuple(args, "i", &length))
		return NULL;
	
	impl->setMaxLength(length > 0 ? length : -1);
})


SL_DEFINE_METHOD(TextView, get_align, {
	return PyInt_FromLong(toAlign(impl->document()->defaultTextOption().alignment()));
})


SL_DEFINE_METHOD(TextView, set_align, {
	int align;
	
	if (!PyArg_ParseTuple(args, "i", &align))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align);
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignLeft | Qt::AlignTop;

	QTextOption option(impl->document()->defaultTextOption());
	option.setAlignment(alignment);
	impl->document()->setDefaultTextOption(option);
})


SL_DEFINE_METHOD(TextView, get_filter, {
	QRegExpValidator *validator = impl->validator();
	QString filter;
	if (!validator)
		filter = ".*";
	else
		filter = validator->regExp().pattern();
	return createStringObject(filter);
})


SL_DEFINE_METHOD(TextView, set_filter, {
	QString filter;
	
	if (!PyArg_ParseTuple(args, "O&", convertString, &filter))
		return NULL;
	
	if (filter.isEmpty())
		filter = ".*";
	delete impl->validator();
	impl->setValidator(new QRegExpValidator(QRegExp(filter), impl));
})


SL_DEFINE_METHOD(TextView, get_line, {
	PyObject *object;
	int pos;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (object == Py_None) {
		;
	}
	else if (convertInt(object, &pos)) {
		cursor.setPosition(pos, QTextCursor::MoveAnchor);
	}
	else
		return NULL;
		
	return PyInt_FromLong(cursor.blockNumber() + 1);
})


SL_DEFINE_METHOD(TextView, set_line, {
	int line;
	
	if (!PyArg_ParseTuple(args, "i", &line))
		return NULL;
	
	if (line < 0) {
		impl->moveCursor(QTextCursor::End);
	}
	else {
		QTextCursor cursor = impl->textCursor();
		QTextBlock block = impl->document()->findBlockByLineNumber(line - 1);
		cursor.setPosition(block.position() + cursor.positionInBlock());
		impl->setTextCursor(cursor);
	}
})


SL_DEFINE_METHOD(TextView, get_column, {
	PyObject *object;
	int pos;
	
	if (!PyArg_ParseTuple(args, "O", &object))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (object == Py_None) {
		;
	}
	else if (convertInt(object, &pos)) {
		cursor.setPosition(pos, QTextCursor::MoveAnchor);
	}
	else
		return NULL;
	
	return PyInt_FromLong(cursor.positionInBlock() + 1);
})


SL_DEFINE_METHOD(TextView, set_column, {
	int column;
	
	if (!PyArg_ParseTuple(args, "i", &column))
		return NULL;
	
	if (column < 0) {
		impl->moveCursor(QTextCursor::EndOfLine);
	}
	else {
		QTextCursor cursor = impl->textCursor();
		cursor.setPosition(cursor.block().position() + column - 1);
		impl->setTextCursor(cursor);
	}
})


SL_DEFINE_METHOD(TextView, get_current_line_color, {
	return createColorObject(impl->currentLineColor());
})


SL_DEFINE_METHOD(TextView, set_current_line_color, {
	QColor color;
	
	if (!PyArg_ParseTuple(args, "O&", convertColor, &color))
		return NULL;
	
	impl->setCurrentLineColor(color);
})


SL_DEFINE_METHOD(TextView, get_tab_width, {
	return PyInt_FromLong(impl->tabStopWidth());
})


SL_DEFINE_METHOD(TextView, set_tab_width, {
	int width;
	
	if (!PyArg_ParseTuple(args, "i", &width))
		return NULL;
	
	impl->setTabStopWidth(width);
})


SL_DEFINE_METHOD(TextView, get_cursor_width, {
	return PyInt_FromLong(impl->cursorWidth());
})


SL_DEFINE_METHOD(TextView, set_cursor_width, {
	int width;
	
	if (!PyArg_ParseTuple(args, "i", &width))
		return NULL;
	
	impl->setCursorWidth(width);
})


SL_DEFINE_METHOD(TextView, get_fragment_color, {
	int pos = -1;

	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (pos >= 0)
		cursor.setPosition(pos);

	QTextCharFormat format = cursor.charFormat();
	return createColorObject(format.foreground().color());
})


SL_DEFINE_METHOD(TextView, set_fragment_color, {
	QColor color;
	int start = -1, length = -1;
	
	if (!PyArg_ParseTuple(args, "O&ii", convertColor, &color, &start, &length))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (start >= 0)
		cursor.setPosition(start);
	if (length >= 0)
		cursor.setPosition(cursor.position() + length, QTextCursor::KeepAnchor);

	QTextCharFormat format = cursor.charFormat();
	if (color.isValid())
		format.setForeground(color);
	else
		format.clearForeground();
	cursor.setCharFormat(format);
	if ((start < 0) && (length < 0))
		impl->setTextCursor(cursor);
})


SL_DEFINE_METHOD(TextView, get_fragment_bgcolor, {
	int pos = -1;

	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (pos >= 0)
		cursor.setPosition(pos);

	QTextCharFormat format = cursor.charFormat();
	QColor color;
	if (format.background().isOpaque())
		color = format.background().color();
	return createColorObject(color);
})


SL_DEFINE_METHOD(TextView, set_fragment_bgcolor, {
	QColor color;
	int start = -1, length = -1;
	
	if (!PyArg_ParseTuple(args, "O&ii", convertColor, &color, &start, &length))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (start >= 0)
		cursor.setPosition(start);
	if (length >= 0)
		cursor.setPosition(cursor.position() + length, QTextCursor::KeepAnchor);

	QTextCharFormat format = cursor.charFormat();
	if (color.isValid())
		format.setBackground(color);
	else
		format.clearBackground();
	cursor.setCharFormat(format);
	if ((start < 0) && (length < 0))
		impl->setTextCursor(cursor);
})


SL_DEFINE_METHOD(TextView, get_fragment_font, {
	int pos = -1;

	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (pos >= 0)
		cursor.setPosition(pos);

	QTextCharFormat format = cursor.charFormat();
	return createFontObject(format.font());
})


SL_DEFINE_METHOD(TextView, set_fragment_font, {
	QFont font;
	int start = -1, length = -1;
	
	if (!PyArg_ParseTuple(args, "O&ii", convertFont, &font, &start, &length))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (start >= 0)
		cursor.setPosition(start);
	if (length >= 0)
		cursor.setPosition(cursor.position() + length, QTextCursor::KeepAnchor);

	QTextCharFormat format = cursor.charFormat();
	format.setFont(font);
	cursor.setCharFormat(format);
	if ((start < 0) && (length < 0))
		impl->setTextCursor(cursor);
})


SL_DEFINE_METHOD(TextView, get_fragment_align, {
	int pos = -1;

	if (!PyArg_ParseTuple(args, "i", &pos))
		return NULL;
	
	QTextCursor cursor = impl->textCursor();
	if (pos >= 0)
		cursor.setPosition(pos);

	QTextBlockFormat format = cursor.blockFormat();
	return PyInt_FromLong(toAlign(format.alignment()));
})


SL_DEFINE_METHOD(TextView, set_fragment_align, {
	int align;
	int start = -1, length = -1;
	
	if (!PyArg_ParseTuple(args, "iii", &align, &start, &length))
		return NULL;
	
	Qt::Alignment alignment = fromAlign(align);
	if (alignment == (Qt::Alignment)0)
		alignment = Qt::AlignLeft | Qt::AlignTop;

	QTextCursor cursor = impl->textCursor();
	if (start >= 0)
		cursor.setPosition(start);
	if (length >= 0)
		cursor.setPosition(cursor.position() + length, QTextCursor::KeepAnchor);

	QTextBlockFormat format = cursor.blockFormat();
	format.setAlignment(alignment);
	cursor.setBlockFormat(format);
})


SL_START_PROXY_DERIVED(TextView, Window)
SL_METHOD(cut)
SL_METHOD(copy)
SL_METHOD(paste)
SL_METHOD(delete)
SL_METHOD(insert)
SL_METHOD(replace)
SL_METHOD(is_modified)
SL_METHOD(set_modified)
SL_METHOD(undo)
SL_METHOD(redo)
SL_METHOD(is_undo_available)
SL_METHOD(is_redo_available)
SL_METHOD(set_highlighted_lines)
SL_METHOD(set_syntax)
SL_METHOD(set_color)
SL_METHOD(get_position)
SL_METHOD(set_object_lookup_regexp)

SL_PROPERTY(style)
SL_PROPERTY(selection)
SL_PROPERTY(insertion_point)
SL_PROPERTY(value)
SL_PROPERTY(length)
SL_PROPERTY(align)
SL_PROPERTY(filter)
SL_PROPERTY(line)
SL_PROPERTY(column)
SL_PROPERTY(current_line_color)
SL_PROPERTY(tab_width)
SL_PROPERTY(cursor_width)
SL_PROPERTY(fragment_color)
SL_PROPERTY(fragment_bgcolor)
SL_PROPERTY(fragment_font)
SL_PROPERTY(fragment_align)
SL_END_PROXY_DERIVED(TextView, Window)


#include "textview.moc"
#include "textview_h.moc"

