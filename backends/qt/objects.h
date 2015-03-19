#ifndef __objects_h__
#define __objects_h__

#include "slew.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QItemDelegate>
#include <QCommonStyle>
#include <QKeyEvent>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QCache>



class DataModel_Impl;


typedef struct DataModel_Proxy {
	PyObject_HEAD
	DataModel_Impl	*fImpl;
	PyObject		*fModel;
} DataModel_Proxy;



class DataSpecifier
{
public:
	DataSpecifier() : fWidget(NULL), fModel(NULL) { fCompleter.fModel = NULL; }
	~DataSpecifier() { Py_XDECREF(fCompleter.fModel); Py_XDECREF(fWidget); Py_XDECREF(fModel); }
	
	int type() { return fFlags & 0xFF; }
	bool isCustom() { return fWidget != NULL; }
	bool isText() { return type() == SL_DATA_SPECIFIER_TEXT; }
	bool isCheckBox() { return type() == SL_DATA_SPECIFIER_CHECKBOX; }
	bool isComboBox() { return type() == SL_DATA_SPECIFIER_COMBOBOX; }
	bool isBrowser() { return type() == SL_DATA_SPECIFIER_BROWSER; }
	bool isSpinField() { return type() == SL_DATA_SPECIFIER_SPINFIELD; }
	bool isCapsOnly() { return (fFlags & SL_DATA_SPECIFIER_CAPS) != 0; }
	bool isReadOnly() { return (fFlags & SL_DATA_SPECIFIER_READONLY) != 0; }
	bool isSelectedOnFocus() { return (fFlags & SL_DATA_SPECIFIER_SELECT_ON_FOCUS) != 0; }
	bool isSelectable() { return (fFlags & SL_DATA_SPECIFIER_SELECTABLE) != 0; }
	bool isEnabled() { return (fFlags & SL_DATA_SPECIFIER_ENABLED) != 0; }
	bool isDraggable() { return (fFlags & SL_DATA_SPECIFIER_DRAGGABLE) != 0; }
	bool isDropTarget() { return (fFlags & SL_DATA_SPECIFIER_DROP_TARGET) != 0; }
	bool isClickableIcon() { return (fFlags & SL_DATA_SPECIFIER_CLICKABLE_ICON) != 0; }
	bool isHTML() { return (fFlags & SL_DATA_SPECIFIER_HTML) != 0; }
	bool isClickableURLs() { return (fFlags & SL_DATA_SPECIFIER_CLICKABLE_URLS) != 0; }
	bool isSeparator() { return (fFlags & SL_DATA_SPECIFIER_SEPARATOR) != 0; }
	bool isNone() { return (fFlags & SL_DATA_SPECIFIER_INVALID) != 0; }
	
	QString					fText;
	int						fDataType;
	QString					fFormat;
	FormatInfo				fFormatInfo[2];
	Qt::Alignment			fAlignment;
	Qt::Alignment			fIconAlignment;
	int						fLength;
	QString					fFilter;
	int						fFlags;
	QIcon					fIcon;
	QColor					fColor;
	QColor					fBGColor;
	QFont					fFont;
	int						fWidth;
	int						fHeight;
	int						fSelection;
	QStringList				fChoices;
	QString					fTip;
	struct {
		PyObject			*fModel;
		int					fColumn;
		QColor				fColor;
		QColor				fBGColor;
		QColor				fHIColor;
		QColor				fHIBGColor;
	}						fCompleter;
	PyObject				*fWidget;
	PyObject				*fModel;
};


class Node;

class DataModel_Impl : public QAbstractItemModel
{
	Q_OBJECT

public:
	DataModel_Impl();
	virtual ~DataModel_Impl();
	
	void initModel(PyObject *model);
	
	QModelIndex index(PyObject *dataIndex) const;
	virtual QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;
	
	virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	
	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	
	virtual bool setData(const QModelIndex &index, const QVariant& value, int role);
	
	virtual bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	bool changeRows(int row, int count, const QModelIndex& parent = QModelIndex());
	
	virtual bool insertColumns(int column, int count, const QModelIndex& parent = QModelIndex());
	virtual bool removeColumns(int column, int count, const QModelIndex& parent = QModelIndex());
	bool changeColumns(int column, int count, const QModelIndex& parent = QModelIndex());
	
	void changeCell(int row, int column, const QModelIndex& parent);
	
	void resetAll();
	void resetHeader();
	
	virtual void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
	
	virtual Qt::DropActions supportedDropActions() const { return Qt::CopyAction | Qt::MoveAction; }
	QStringList mimeTypes() const;
	
	DataSpecifier *getDataSpecifier(const QModelIndex& index) const;
	PyObject *getDataIndex(const QModelIndex& index) const;
	
	void invalidateDataSpecifiers();
	
signals:
	void sorted(int column, Qt::SortOrder order);
	void configureHeader(const QPoint& headerPos, Qt::TextElideMode elideMode) const;

private slots:
	void handleReset();
	
private:
	Node									*fRoot;
	QList<DataSpecifier *>					fHeaderData;
	PyObject								*fModel;
};



class ItemDelegate : public QItemDelegate
{
	Q_OBJECT
	
public:
	ItemDelegate(QObject *parent);
	
	virtual void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual void drawDisplay(QPainter *painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const;
	
	virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option, const QModelIndex& index);
	virtual bool eventFilter(QObject *object, QEvent *event);
	virtual void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem& option, const QModelIndex& index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
	
	virtual void setEditorData(QWidget *editor, const QModelIndex& index) const;
	virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex& index) const;
	
	virtual bool isFocusOutEvent(QEvent *event);
	virtual bool canFocusOut(QWidget *oldFocus, QWidget *newFocus);
	
	bool isEditValid();

	QTextDocument *getTextDocument(const QModelIndex& index) const { return fTextDocumentsCache.object(index); };
	
public slots:
	void startEditing(const QModelIndex& index);
	
protected:
	virtual void preparePaint(QStyleOptionViewItem *opt, QStyleOptionViewItem *backOpt, const QModelIndex& index) const {}
	virtual void finishPaint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {}
	
	DataSpecifier						*fCurrentSpec;
	QKeyEvent							*fTabEvent;
	QPixmap								fInvalidPattern;
	QModelIndex							fCurrentIndex;
	QCache<QModelIndex, QTextDocument>	fTextDocumentsCache;
};



class HeaderStyle : public QCommonStyle
{
	Q_OBJECT
	
public:
	HeaderStyle(QAbstractItemView *parent) : QCommonStyle(), fMode(Qt::ElideRight) { setParent(parent); parent->setStyle(this); }
	
	void setMode(Qt::TextElideMode mode) { fMode = mode; }
	
	virtual void drawControl(ControlElement ce, const QStyleOption* opt, QPainter* p, const QWidget* widget) const {
		QStyle *style = QApplication::style();
		
		switch (ce) {
		case QStyle::CE_Header:
			{
				QStyleOptionHeader o(*(QStyleOptionHeader *)opt);
				if (fMode != Qt::ElideNone) {
					QRect rect = subElementRect(SE_HeaderLabel, &o, widget);
					o.text = o.fontMetrics.elidedText(o.text, fMode, rect.width());
				}
				style->drawControl(ce, &o, p, widget);
			}
			break;
		
		default:
			{
				style->drawControl(ce, opt, p, widget);
			}
			break;
		}
	}

private:
	Qt::TextElideMode	fMode;
};



extern PyObject *PyDC_Type;
extern PyObject *PyPrintDC_Type;
extern PyTypeObject DC_Type;

extern PyObject *PyPaper_Type;
extern PyObject *PyEvent_Type;
extern PyObject *PyDataIndex_Type;
extern PyObject *PyDataSpecifier_Type;
extern PyObject *PyDataModel_Type;
extern PyTypeObject DataModel_Type;


bool DC_type_setup(PyObject *module);
bool PrintDC_type_setup(PyObject *module);
bool SceneItemDC_type_setup(PyObject *module);
bool Bitmap_type_setup(PyObject *module);
bool Picture_type_setup(PyObject *module);
bool DataModel_type_setup(PyObject *module);



#endif
