#include "slew.h"
#include "objects.h"

#include <QStack>
#include <QHeaderView>
#include <QLatin1String>


#define HEADER_CONFIGURED			0x80000000


// class Counter
// {
// public:
// 	Counter() { fCount = 0; }
// 	~Counter() { qDebug() << "final count is" << fCount; }
// 	void inc() { fCount++; }
// 	void dec() { fCount--; }
// 	
// private:
// 	int		fCount;
// };
// 
// static Counter sCounter;



static bool
indexLessThan(const QModelIndex& a, const QModelIndex& b)
{
	int i, level_a = 1, level_b = 1;
	QModelIndex p, aa, bb;
	
	if (a.parent() == b.parent())
		return a.row() < b.row();
	
	for (p = a.parent(); p.isValid(); p = p.parent())
		level_a++;
	for (p = b.parent(); p.isValid(); p = p.parent())
		level_b++;
	
	aa = a;
	bb = b;
	
	for (i = level_a; i > level_b; i--)
		aa = aa.parent();
	for (i = level_b; i > level_a; i--)
		bb = bb.parent();
	
	while ((aa.isValid()) && (bb.isValid())) {
		if (aa.parent() == bb.parent())
			return aa.row() < bb.row();
		aa = aa.parent();
		bb = bb.parent();
	}
	return false;
}



class Node
{
public:
	Node(PyObject *model, int row, int column, Node *parent = NULL);
	~Node();
	
	int row() { return fRow; }
	int column() { return fColumn; }
	Node *parent() { return fParent; }
	PyObject *dataIndex();
	DataSpecifier *dataSpecifier();
	
	void invalidate(bool full = true);
	void resetData();
	
	bool hasChild(int row, int column);
	Node *child(int row, int column);
	
	int rowCount();
	int columnCount();
	bool hasChildren();
	
	void insertRows(int pos, int count);
	void removeRows(int pos, int count);
	void changeRows(int pos, int count);
	
	void insertColumns(int pos, int count);
	void removeColumns(int pos, int count);
	void changeColumns(int pos, int count);
	
private:
	int						fRow;
	int						fColumn;
	int						fRowCount;
	int						fColumnCount;
	Node					*fParent;
	QList<QList<Node *> *>	fChildren;
	DataSpecifier			*fData;
	PyObject				*fModel;
	PyObject				*fIndex;
};


Node::Node(PyObject *model, int row, int column, Node *parent)
	: fRow(row), fColumn(column), fRowCount(-1), fColumnCount(-1), fParent(parent), fData(NULL), fModel(model), fIndex(NULL)
{
	PyAutoLocker locker;
	Py_XINCREF(fModel);
// 	sCounter.inc();
}


Node::~Node()
{
	PyAutoLocker locker;
// 	sCounter.dec();
	invalidate();
	Py_XDECREF(fModel);
}


void
Node::invalidate(bool full)
{
	PyAutoLocker locker;
	
	if (full) {
		foreach(QList<Node *> *row, fChildren) {
			for (int j = 0; j < row->count(); j++) {
				Node *node = row->at(j);
				delete node;
			}
			delete row;
		}
		fChildren.clear();
		fRowCount = -1;
		fColumnCount = -1;
	}
	
	delete fData;
	fData = NULL;
	
	Py_XDECREF(fIndex);
	fIndex = NULL;
}


void
Node::resetData()
{
	PyAutoLocker locker;
	
	foreach(QList<Node *> *row, fChildren) {
		if (row) {
			for (int j = 0; j < row->count(); j++) {
				Node *node = row->at(j);
				if (node)
					node->resetData();
			}
		}
	}
	
	delete fData;
	fData = NULL;
	
	Py_XDECREF(fIndex);
	fIndex = NULL;
}


PyObject *
Node::dataIndex()
{
	PyAutoLocker locker;
	PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
	
	if (fIndex == NULL) {
		if ((fParent == NULL) || (model == Py_None)) {
			fIndex = Py_None;
			Py_INCREF(fIndex);
		}
		else {
			fIndex = PyObject_CallMethod(model, "index", "iiO", fRow, fColumn, fParent->dataIndex());
			if (!fIndex) {
				PyErr_Print();
				PyErr_Clear();
				fIndex = Py_None;
				Py_INCREF(fIndex);
			}
		}
	}
	return fIndex;
}


bool
Node::hasChild(int row, int column)
{
	return ((row >= 0) && (row < rowCount()) && (column >= 0) && (column < columnCount()));
}


Node *
Node::child(int row, int column)
{
// 	qDebug() << "asked for child" << row << column << fChildren;
	Node *node = fChildren[row]->at(column);
	if (node == NULL) {
		node = new Node(fModel, row, column, this);
		fChildren[row]->replace(column, node);
	}
	return node;
}


int
Node::rowCount()
{
	PyAutoLocker locker;
	if (fRowCount < 0) {
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (model == Py_None) {
			fRowCount = 0;
			return 0;
		}
		PyObject *result = PyObject_CallMethod(model, "row_count", "O", dataIndex());
		if (result) {
			fRowCount = PyInt_AsLong(result);
			Py_DECREF(result);
		}
		
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
			fRowCount = 0;
		}
		
		if (fChildren.size() < fRowCount) {
			int num_columns = columnCount();
			while (fChildren.size() < fRowCount) {
				QList<Node *> *row = new QList<Node *>();
				for (int i = 0; i < num_columns; i++)
					row->append(NULL);
				fChildren.append(row);
			}
		}
	}
	return fRowCount;
}


int
Node::columnCount()
{
	PyAutoLocker locker;
	if (fColumnCount < 0) {
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (model == Py_None) {
			fColumnCount = 0;
			return 0;
		}
		PyObject *result = PyObject_CallMethod(model, "column_count", NULL);
		if (result) {
			fColumnCount = PyInt_AsLong(result);
			Py_DECREF(result);
		}
		
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
			fColumnCount = 0;
		}
		
		foreach(QList<Node *> *row, fChildren) {
			while (row->size() < fColumnCount)
				row->append(NULL);
		}
	}
	return fColumnCount;
}


bool
Node::hasChildren()
{
	PyAutoLocker locker;
	if (fRowCount < 0) {
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (((fParent != NULL) && (fColumn > 0)) || (fModel == Py_None))
			return false;
		PyObject *result = PyObject_CallMethod(model, "has_children", "O", dataIndex());
		if (!result) {
			PyErr_Print();
			PyErr_Clear();
			return false;
		}
		if (result == Py_None) {
			Py_DECREF(result);
			return rowCount() > 0;
		}
		bool has = PyObject_IsTrue(result) ? true : false;
		Py_DECREF(result);
		return has;
	}
	return (fRowCount > 0) && (fColumnCount > 0);
}


void
Node::insertRows(int pos, int count)
{
	PyAutoLocker locker;
	int i, num_columns = columnCount();
	
	if (fRowCount < 0)
		fRowCount = 0;
	
	for (i = pos; i < fChildren.size(); i++) {
		QList<Node *> *row = fChildren.at(i);
		foreach (Node *node, *row) {
			if (node != NULL) {
				node->fRow += count;
				node->invalidate(false);
			}
		}
	}
	
	fRowCount += count;
	
	while (count > 0) {
		QList<Node *> *row = new QList<Node *>();
		for (i = 0; i < num_columns; i++) {
			row->append(NULL);
		}
		fChildren.insert(pos, row);
		count--;
	}
}


void
Node::removeRows(int pos, int count)
{
	PyAutoLocker locker;
	int i;
	
	for (i = pos + count; i < fChildren.size(); i++) {
		QList<Node *> *row = fChildren.at(i);
		for (int j = 0; j < row->count(); j++) {
			Node *node = row->at(j);
			if (node != NULL) {
				node->fRow -= count;
				node->invalidate(false);
			}
		}
	}
	
	fRowCount -= count;
	
	while ((count > 0) && (fChildren.size() > pos)) {
		QList<Node *> *row = fChildren.takeAt(pos);
		for (int j = 0; j < row->count(); j++) {
			Node *node = row->at(j);
			delete node;
		}
		delete row;
		count--;
	}
}


void
Node::changeRows(int pos, int count)
{
	PyAutoLocker locker;
	for (int i = pos; i < pos + count; i++) {
		QList<Node *> *row = fChildren.value(i);
		if (row) {
			for (int j = 0; j < row->count(); j++) {
				Node *node = row->at(j);
				if (node != NULL) {
					node->invalidate();
				}
			}
		}
	}
}


void
Node::insertColumns(int pos, int count)
{
	PyAutoLocker locker;
	int i;
	
	if (fColumnCount < 0)
		fColumnCount = 0;
	fColumnCount += count;
	
	foreach(QList<Node *> *row, fChildren) {
		for (i = pos; i < row->size(); i++) {
			Node *node = row->at(i);
			if (node != NULL) {
				node->fColumn += count;
				node->invalidate();
			}
		}
		
		while (count > 0) {
			row->insert(pos, NULL);
			count--;
		}
	}
}


void
Node::removeColumns(int pos, int count)
{
	PyAutoLocker locker;
	int i;
	
	fColumnCount -= count;
	
	foreach(QList<Node *> *row, fChildren) {
		for (i = pos + count; i < row->size(); i++) {
			Node *node = row->at(i);
			if (node != NULL) {
				node->fColumn -= count;
				node->invalidate();
			}
		}
		
		while (count > 0) {
			delete row->takeAt(pos);
			count--;
		}
	}
}


void
Node::changeColumns(int pos, int count)
{
	PyAutoLocker locker;
	
	foreach(QList<Node *> *row, fChildren) {
		for (int i = pos; i < pos + count; i++) {
			Node *node = row->at(i);
			if (node != NULL) {
				node->invalidate();
			}
		}
	}
}


DataSpecifier *
Node::dataSpecifier()
{
	if (!fData) {
		PyAutoLocker locker;
		PyObject *index = dataIndex();
		if ((!index) || (index == Py_None))
			return NULL;
		
		fData = new DataSpecifier;
		
		PyObject *dataSpecifier;
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (model == Py_None) {
			fData->fFlags = -1;
			return fData;
		}
		
		dataSpecifier = PyObject_CallMethod(model, "data", "O", index);
		
		for (;;) {
			if (!dataSpecifier)
				break;
			if (!PyObject_TypeCheck(dataSpecifier, (PyTypeObject *)PyDataSpecifier_Type)) {
				PyErr_SetString(PyExc_TypeError, "expected 'DataSpecifier' object");
				break;
			}
			
			int align, icon_align;
			
			if ((!getObjectAttr(dataSpecifier, "text", &fData->fText)) ||
				(!getObjectAttr(dataSpecifier, "datatype", &fData->fDataType)) ||
				(!getObjectAttr(dataSpecifier, "format", &fData->fFormat)) ||
				(!getObjectAttr(dataSpecifier, "align", &align)) ||
				(!getObjectAttr(dataSpecifier, "icon_align", &icon_align)) ||
				(!getObjectAttr(dataSpecifier, "length", &fData->fLength)) ||
				(!getObjectAttr(dataSpecifier, "filter", &fData->fFilter)) ||
				(!getObjectAttr(dataSpecifier, "tip", &fData->fTip)) ||
				(!getObjectAttr(dataSpecifier, "flags", &fData->fFlags)) ||
				(!getObjectAttr(dataSpecifier, "icon", &fData->fIcon)) ||
				(!getObjectAttr(dataSpecifier, "color", &fData->fColor)) ||
				(!getObjectAttr(dataSpecifier, "bgcolor", &fData->fBGColor)) ||
				(!getObjectAttr(dataSpecifier, "font", &fData->fFont)) ||
				(!getObjectAttr(dataSpecifier, "width", &fData->fWidth)) ||
				(!getObjectAttr(dataSpecifier, "height", &fData->fHeight)) ||
				(!getObjectAttr(dataSpecifier, "selection", &fData->fSelection)))
				break;
			
			if (fData->fFilter.isEmpty())
				fData->fFilter = ".*";
			
			fData->fAlignment = fromAlign(align);
			if ((fData->fAlignment & Qt::AlignHorizontal_Mask) == 0)
				fData->fAlignment |= Qt::AlignLeft;
			if ((fData->fAlignment & Qt::AlignVertical_Mask) == 0)
				fData->fAlignment |= Qt::AlignVCenter;
			
			fData->fIconAlignment = fromAlign(icon_align);
			if ((fData->fIconAlignment & Qt::AlignHorizontal_Mask) == 0)
				fData->fIconAlignment |= Qt::AlignHCenter;
			if ((fData->fIconAlignment & Qt::AlignVertical_Mask) == 0)
				fData->fAlignment |= Qt::AlignVCenter;
			
			PyObject *format_vars = PyObject_GetAttrString(dataSpecifier, "format_vars");
			if (!format_vars)
				break;
			if (PyDict_Check(format_vars)) {
				QHash<QString, QString> vars;
				PyObject *key, *value;
				Py_ssize_t pos = 0;
				
				while (PyDict_Next(format_vars, &pos, &key, &value)) {
					QString k, v;
					if (!convertString(key, &k)) {
						PyErr_Clear();
					}
					else {
						if (!convertString(value, &v)) {
							PyErr_Clear();
							PyObject *o = PyObject_Str(value);
							if (!o) {
								PyErr_Clear();
								continue;
							}
							convertString(o, &v);
						}
						vars[k] = v;
					}
				}
				
				fData->fFormat = normalizeFormat(vars, fData->fFormat);
			}
			else if (format_vars != Py_None) {
				Py_DECREF(format_vars);
				PyErr_SetString(PyExc_TypeError, "expected 'dict' or 'None' object for format_vars");
				break;
			}
			Py_DECREF(format_vars);
			parseFormat(fData->fFormat, fData->fDataType, fData->fFormatInfo);
			
			PyObject *choices = PyObject_GetAttrString(dataSpecifier, "choices");
			if (!choices)
				break;
			PyObject *seq = PySequence_Fast(choices, "expected sequence object");
			if (!seq) {
				Py_DECREF(choices);
				break;
			}
			else {
				Py_ssize_t pos, size = PySequence_Fast_GET_SIZE(seq);
				QString choice;
				for (pos = 0; pos < size; pos++) {
					if (convertString(PySequence_Fast_GET_ITEM(seq, pos), &choice)) {
						fData->fChoices.append(choice);
					}
					else {
						PyErr_Clear();
					}
				}
				Py_DECREF(seq);
				Py_DECREF(choices);
			}
			
			PyObject *completer = PyObject_GetAttrString(dataSpecifier, "completer");
			if (!completer)
				break;
			if (completer != Py_None) {
				fData->fCompleter.fModel = PyObject_GetAttrString(completer, "model");
				if ((!fData->fCompleter.fModel) ||
					(!getObjectAttr(completer, "column", &fData->fCompleter.fColumn)) ||
					(!getObjectAttr(completer, "color", &fData->fCompleter.fColor)) ||
					(!getObjectAttr(completer, "bgcolor", &fData->fCompleter.fBGColor)) ||
					(!getObjectAttr(completer, "hicolor", &fData->fCompleter.fHIColor)) ||
					(!getObjectAttr(completer, "hibgcolor", &fData->fCompleter.fHIBGColor))) {
					Py_DECREF(completer);
					break;
				}
				if (fData->fCompleter.fModel == Py_None) {
					Py_DECREF(fData->fCompleter.fModel);
					fData->fCompleter.fModel = NULL;
				}
				else if (!PyObject_TypeCheck(fData->fCompleter.fModel, (PyTypeObject *)PyDataModel_Type)) {
					Py_DECREF(completer);
					PyErr_SetString(PyExc_TypeError, "expected 'Completer' or None object");
					break;
				}
			}
			Py_DECREF(completer);
			
			fData->fWidget = PyObject_GetAttrString(dataSpecifier, "widget");
			if ((fData->fWidget) && (fData->fWidget != Py_None) && (!isWidget(fData->fWidget))) {
				PyErr_SetString(PyExc_ValueError, "excepted 'Widget' or None object");
				break;
			}
			if (fData->fWidget == Py_None) {
				Py_DECREF(Py_None);
				fData->fWidget = NULL;
			}
			
			fData->fModel = PyObject_GetAttrString(dataSpecifier, "model");
			if ((fData->fModel) && (fData->fModel != Py_None) && (!PyObject_TypeCheck(fData->fModel, (PyTypeObject *)PyDataModel_Type))) {
				PyErr_SetString(PyExc_ValueError, "excepted 'DataModel' or None object");
				break;
			}
			if (fData->fModel == Py_None) {
				Py_DECREF(Py_None);
				fData->fModel = NULL;
			}

			Py_DECREF(dataSpecifier);
			return fData;
		}
		
		fData->fFlags = -1;
		PyErr_Print();
		PyErr_Clear();
		Py_XDECREF(dataSpecifier);
	}
	return fData;
}



DataModel_Impl::DataModel_Impl()
	: QAbstractItemModel(), fModel(NULL)
{
	fRoot = new Node(NULL, -1, -1, NULL);
	connect(this, SIGNAL(modelReset()), this, SLOT(handleReset()));
}


DataModel_Impl::~DataModel_Impl()
{
	PyAutoLocker locker;
	delete fRoot;
	SL_QAPP()->unregisterObject(this);
}


void
DataModel_Impl::initModel(PyObject *model)
{
	beginResetModel();
	
	delete fRoot;
	fRoot = new Node(model, -1, -1, NULL);
	fModel = model;
	
	endResetModel();
}


void
DataModel_Impl::resetAll()
{
	reset();
}


void
DataModel_Impl::resetHeader()
{
	foreach (DataSpecifier *data, fHeaderData)
		delete data;
	fHeaderData.clear();
}


void
DataModel_Impl::handleReset()
{
	resetHeader();
	fRoot->invalidate();
}


DataSpecifier *
DataModel_Impl::getDataSpecifier(const QModelIndex& index) const
{
	if (!index.isValid())
		return NULL;
	
	Node *node = (Node *)index.internalPointer();
	return node->dataSpecifier();
}


PyObject *
DataModel_Impl::getDataIndex(const QModelIndex& index) const
{
	if (!index.isValid())
		return Py_None;
	
	Node *node = (Node *)index.internalPointer();
	PyObject *dataIndex = node->dataIndex();
	if (!dataIndex)
		return Py_None;
	return dataIndex;
}


void
DataModel_Impl::invalidateDataSpecifiers()
{
	emit layoutAboutToBeChanged();
	
	QModelIndexList from_list = persistentIndexList();
	QModelIndexList temp = from_list;
	QModelIndexList to_list = from_list;
	QModelIndex idx;
	
	qStableSort(temp.begin(), temp.end(), indexLessThan);
	
	fRoot->resetData();
	for (int i = 0; i < temp.size(); i++) {
		idx = temp.at(i);
		int pos = from_list.indexOf(idx);
		to_list[pos] = index(idx.row(), idx.column(), idx.parent());
	}
	changePersistentIndexList(from_list, to_list);
	emit layoutChanged();
	emit dataChanged(index(0, 0), index(rowCount(), columnCount() - 1));
}


QModelIndex
DataModel_Impl::index(int row, int column, const QModelIndex& parent) const
{
	Node *parentNode;
	
	if (parent.isValid()) {
		if (parent.column() != 0)
			return QModelIndex();
		parentNode = (Node *)parent.internalPointer();
	}
	else {
		parentNode = fRoot;
	}
	if (!parentNode->hasChild(row, column))
		return QModelIndex();
	return createIndex(row, column, parentNode->child(row, column));
}


QModelIndex
DataModel_Impl::index(PyObject *dataIndex) const
{
	PyAutoLocker locker;
	if ((!dataIndex) || (dataIndex == Py_None))
		return QModelIndex();
	
	if (!PyObject_TypeCheck(dataIndex, (PyTypeObject *)PyDataIndex_Type)) {
		PyErr_SetString(PyExc_ValueError, "expected DataIndex object");
		PyErr_Print();
		PyErr_Clear();
		return QModelIndex();
	}
	QStack<QPair<int, int> > stack;
	
	while ((dataIndex) && (dataIndex != Py_None)) {
		int row, column;
		if ((!getObjectAttr(dataIndex, "row", &row)) ||
			(!getObjectAttr(dataIndex, "column", &column))) {
			PyErr_Print();
			PyErr_Clear();
			return QModelIndex();
		}
		stack.push(qMakePair(row, column));
		
		dataIndex = PyObject_GetAttrString(dataIndex, "parent");
		if (!dataIndex) {
			PyErr_Print();
			PyErr_Clear();
			return QModelIndex();
		}
		Py_DECREF(dataIndex);		// safe as parent is part of DataIndex
	}
	
	Node *node = fRoot;
	QPair<int, int> coord;
	while (!stack.empty()) {
		coord = stack.pop();
		if (!node->hasChild(coord.first, coord.second)) {
			PyErr_Clear();
			PyErr_SetString(PyExc_ValueError, "Index not found in model hierarchy");
			return QModelIndex();
		}
// 		qDebug() << "index from dataindex" << coord.first << coord.second << this;
		node = node->child(coord.first, coord.second);
	}
	return createIndex(node->row(), node->column(), node);
}


QModelIndex
DataModel_Impl::parent(const QModelIndex& index) const
{
	if (!index.isValid())
		return QModelIndex();
	
	Node *node = ((Node *)index.internalPointer())->parent();
	if ((node == fRoot) || (!node))
		return QModelIndex();
	return createIndex(node->row(), node->column(), node);
}


bool
DataModel_Impl::hasChildren(const QModelIndex& parent) const
{
	Node *node;
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	return node->hasChildren();
}


int
DataModel_Impl::rowCount(const QModelIndex &parent) const
{
	Node *node;
	if (parent.isValid()) {
		if (parent.column() > 0)
			return 0;
		node = (Node *)parent.internalPointer();
	}
	else
		node = fRoot;
	return node->rowCount();
}


int
DataModel_Impl::columnCount(const QModelIndex &parent) const
{
	Node *node;
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	return node->columnCount();
}


QVariant
DataModel_Impl::data(const QModelIndex &index, int role) const
{
	PyAutoLocker locker;
	DataSpecifier *spec = getDataSpecifier(index);
	
	if ((!spec) || (spec->isNone()))
		return QVariant();
	
	switch (role) {
	case Qt::DisplayRole:
	case Qt::EditRole:
		{
			if (!spec->fText.isEmpty())
				return spec->fText;
		}
		break;
	
	case Qt::DecorationRole:
		{
			if ((!spec->fIcon.isNull()) && (!spec->isClickableIcon()))
				return spec->fIcon;
		}
		break;
		
	case Qt::FontRole:
		{
			return spec->fFont;
		}
		break;
		
	case Qt::TextAlignmentRole:
		{
			return int(spec->fAlignment);
		}
		break;
		
	case Qt::BackgroundRole:
		{
			if (spec->fBGColor.isValid())
				return spec->fBGColor;
		}
		break;
		
	case Qt::ForegroundRole:
		{
			if (spec->fColor.isValid())
				return spec->fColor;
		}
		break;
	
	case Qt::SizeHintRole:
		{
			if (spec->fHeight)
				return QSize(0, spec->fHeight);
		}
		break;
		
	case Qt::ToolTipRole:
	case Qt::StatusTipRole:
		{
			if (!spec->fTip.isEmpty())
				return spec->fTip;
		}
		break;
	
	case Qt::AccessibleDescriptionRole:
		{
			if (spec->isSeparator())
				return QLatin1String("separator");
		}
		break;
	}
	
	return QVariant();
}


QVariant
DataModel_Impl::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant value;
	QPoint headerPos = orientation == Qt::Horizontal ? QPoint(section, -1) : QPoint(-1, section);
	DataSpecifier *data = NULL;
	DataSpecifier temp;
	
	if (orientation == Qt::Horizontal)
		data = fHeaderData.value(section);
	
	if (!data) {
		int align;
		PyAutoLocker locker;
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (model == Py_None)
			return value;
		
		if (orientation == Qt::Horizontal)
			data = new DataSpecifier();
		else
			data = &temp;
		
		PyObject *pos = createVectorObject(headerPos);
		PyObject *spec = PyObject_CallMethod(model, "header", "O", pos);
		Py_DECREF(pos);
		if ((spec) && (!PyObject_TypeCheck(spec, (PyTypeObject *)PyDataSpecifier_Type)))
			PyErr_SetString(PyExc_TypeError, "expected 'DataSpecifier' object");
		
		if (spec) {
			if ((!getObjectAttr(spec, "text", &data->fText)) ||
				(!getObjectAttr(spec, "align", &align)) ||
				(!getObjectAttr(spec, "flags", &data->fFlags)) ||
				(!getObjectAttr(spec, "width", &data->fWidth)) ||
				(!getObjectAttr(spec, "height", &data->fHeight)))
				(void)0;
		}
		if (PyErr_Occurred()) {
			PyErr_Print();
			PyErr_Clear();
			Py_XDECREF(spec);
			if (orientation == Qt::Horizontal)
				delete data;
			return value;
		}
		Py_DECREF(spec);
		
		data->fAlignment = fromAlign(align) & Qt::AlignHorizontal_Mask;
		if (data->fAlignment == 0)
			data->fAlignment = Qt::AlignLeft;
		data->fAlignment |= Qt::AlignVCenter;
		
		if (orientation == Qt::Horizontal) {
			DataModel_Impl *that = (DataModel_Impl *)this;
			while (that->fHeaderData.size() <= section)
				that->fHeaderData.append(NULL);
			that->fHeaderData[section] = data;
		}
	}
	
	if (data->fFlags & SL_DATA_SPECIFIER_AUTO_WIDTH)
		data->fWidth = 1;
	
	switch (role) {
	case Qt::DisplayRole:
		{
			value = data->fText;
			if (!(data->fFlags & HEADER_CONFIGURED)) {
				data->fFlags |= HEADER_CONFIGURED;
				
				Qt::TextElideMode elideMode = Qt::ElideNone;
				QHeaderView::ResizeMode resizeMode = QHeaderView::Interactive;
				
				if (data->fFlags & SL_DATA_SPECIFIER_ELIDE_LEFT)
					elideMode = Qt::ElideLeft;
				else if (data->fFlags & SL_DATA_SPECIFIER_ELIDE_MIDDLE)
					elideMode = Qt::ElideMiddle;
				else if (data->fFlags & SL_DATA_SPECIFIER_ELIDE_RIGHT)
					elideMode = Qt::ElideRight;
				
				if (data->fFlags & SL_DATA_SPECIFIER_AUTO_WIDTH)
					resizeMode = QHeaderView::ResizeToContents;
				
				emit configureHeader(headerPos, elideMode, resizeMode);
			}
		}
		break;
	
	case Qt::TextAlignmentRole:
		{
			value = int(data->fAlignment);
		}
		break;
		
	case Qt::UserRole:
		{
			value = data->fWidth;
		}
		break;
	
	case Qt::SizeHintRole:
		{
			if ((data->fWidth) || (data->fHeight))
				value = QSize(data->fWidth, data->fHeight);
		}
		break;
	}
	
	return value;
}


Qt::ItemFlags
DataModel_Impl::flags(const QModelIndex &index) const
{
	PyAutoLocker locker;
	DataSpecifier *spec = getDataSpecifier(index);
	Qt::ItemFlags flags = 0;
	
	if ((spec) && (!spec->isNone()) && (!spec->isSeparator())) {
		flags |= Qt::ItemIsDropEnabled;
		if (spec->isEnabled())
			flags |= Qt::ItemIsEnabled;
		if (spec->isSelectable())
			flags |= Qt::ItemIsSelectable;
		if (!spec->isReadOnly())
			flags |= Qt::ItemIsEditable;
		if (spec->isDraggable())
			flags |= Qt::ItemIsDragEnabled;
		if (!spec->isDropTarget())
			flags &= ~Qt::ItemIsDropEnabled;
	}
	
	return flags;
}


bool
DataModel_Impl::setData(const QModelIndex &index, const QVariant& value, int role)
{
	PyAutoLocker locker;
	Node *node = (Node *)index.internalPointer();
	PyObject *dataIndex = node->dataIndex();
	PyObject *spec = PyObject_CallFunctionObjArgs(PyDataSpecifier_Type, NULL);
	if (!spec) {
		PyErr_Print();
		PyErr_Clear();
		return false;
	}
	
	if (dataIndex) {
		switch (role) {
		case Qt::EditRole:
			{
				PyObject *object = createStringObject(value.toString());
				PyObject_SetAttrString(spec, "text", object);
				Py_DECREF(object);
			}
			break;
			
		case Qt::UserRole:
			{
				PyObject *object = PyInt_FromLong(value.toInt());
				PyObject_SetAttrString(spec, "selection", object);
				Py_DECREF(object);
			}
			break;
		
		default:
			{
				if (role == Qt::UserRole + 1) {
					PyObject_SetAttrString(spec, "browsed_data", value.value<PyObject *>());
				}
			}
			break;
		}
		
		PyObject *model = fModel ? PyWeakref_GetObject(fModel) : Py_None;
		if (model != Py_None) {
			PyObject *result = PyObject_CallMethod(model, "set_data", "OO", dataIndex, spec);
			Py_XDECREF(result);
		}
	}
	Py_DECREF(spec);
	
	if (PyErr_Occurred()) {
		PyErr_Print();
		PyErr_Clear();
	}
	else {
		changeCell(index.row(), index.column(), index.parent());
	}
	return true;
}


bool
DataModel_Impl::insertRows(int row, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	
	resetHeader();
	
	beginInsertRows(parent, row, row + count - 1);
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	node->insertRows(row, count);
	endInsertRows();
	return true;
}


bool
DataModel_Impl::removeRows(int row, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	
	resetHeader();
	
	beginRemoveRows(parent, row, row + count - 1);
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	node->removeRows(row, count);
	endRemoveRows();
	
	return true;
}


bool
DataModel_Impl::changeRows(int row, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	QModelIndexList from_list;
	QModelIndexList to_list;
	QModelIndexList changed_list;
	QModelIndex idx;
	int i;
	
	resetHeader();
	
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	
	from_list = persistentIndexList();
	to_list = from_list;
	
	emit layoutAboutToBeChanged();

	for (i = 0; i < to_list.size(); i++) {
		idx = to_list.at(i);
		bool found = false;
		while (idx.isValid()) {
			if (idx.parent() == parent) {
				found = true;
				break;
			}
			idx = idx.parent();
		}
		if (found)
			changed_list.append(to_list.at(i));
	}

	node->changeRows(row, count);
	
	for (i = 0; i < to_list.size(); i++) {
		idx = to_list.at(i);
		if (changed_list.contains(idx))
			to_list[i] = index(idx.row(), idx.column(), idx.parent());
	}
	changePersistentIndexList(from_list, to_list);
	emit layoutChanged();
	emit dataChanged(index(row, 0, parent), index(row + count - 1, columnCount(parent) - 1, parent));
	
	return true;
}


bool
DataModel_Impl::insertColumns(int column, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	
	resetHeader();
	
	beginInsertColumns(parent, column, column + count - 1);
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	node->insertColumns(column, count);
	endInsertColumns();
	
	return true;
}


bool
DataModel_Impl::removeColumns(int column, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	
	resetHeader();
	
	beginRemoveColumns(parent, column, column + count - 1);
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	node->removeColumns(column, count);
	endRemoveColumns();
	
	return true;
}


bool
DataModel_Impl::changeColumns(int column, int count, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	QModelIndexList from_list;
	QModelIndexList to_list;
	QModelIndexList changed_list;
	QModelIndex idx;
	int i;
	
	resetHeader();
	
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	
	from_list = persistentIndexList();
	to_list = from_list;
	
	emit layoutAboutToBeChanged();

	for (i = 0; i < to_list.size(); i++) {
		idx = to_list.at(i);
		bool found = false;
		while (idx.isValid()) {
			if (idx.parent() == parent) {
				found = true;
				break;
			}
			idx = idx.parent();
		}
		if (found)
			changed_list.append(to_list.at(i));
	}

	node->changeColumns(column, count);
	
	for (i = 0; i < to_list.size(); i++) {
		idx = to_list.at(i);
		if (changed_list.contains(idx))
			to_list[i] = index(idx.row(), idx.column(), idx.parent());
	}
	changePersistentIndexList(from_list, to_list);
	emit layoutChanged();
	emit dataChanged(index(0, column, parent), index(rowCount(parent) - 1, column + count - 1, parent));

	return true;
}


void
DataModel_Impl::changeCell(int row, int column, const QModelIndex& parent)
{
	PyAutoLocker locker;
	Node *node;
	QModelIndexList from_list;
	QModelIndexList to_list;
	QModelIndexList scan, changed;
	QModelIndex idx, thisIndex;
	int i, j, thisIndexPos = -1;
	
	if (parent.isValid())
		node = (Node *)parent.internalPointer();
	else
		node = fRoot;
	if (node->hasChild(row, column)) {
		from_list = persistentIndexList();
		to_list = from_list;
		
		thisIndex = index(row, column, parent);
		scan.append(thisIndex);
		while (!scan.isEmpty()) {
			idx = scan.takeFirst();
			changed.append(idx);
			for (i = 0; i < rowCount(idx); i++) {
				for (j = 0; j < columnCount(idx); j++) {
					scan.append(index(i, j, idx));
				}
			}
		}
		
		emit layoutAboutToBeChanged();
	
		for (i = 0; i < to_list.size(); i++) {
			idx = to_list.at(i);
			if (changed.contains(idx)) {
				to_list[i] = QModelIndex();
				if (idx == thisIndex)
					thisIndexPos = i;
			}
		}
		
		node->child(row, column)->invalidate();
		thisIndex = index(row, column, parent);
		if (thisIndexPos >= 0)
			to_list[thisIndexPos] = thisIndex;
	
		changePersistentIndexList(from_list, to_list);
		
		emit layoutChanged();
		emit dataChanged(thisIndex, thisIndex);
	}
}


void
DataModel_Impl::sort(int column, Qt::SortOrder order)
{
	emit sorted(column, order);
}


QStringList
DataModel_Impl::mimeTypes() const
{
	QStringList list;
	
	list << "text/uri-list";
	list << "text/plain";
	list << "image/*";
	list << "application/x-qt-image";
	list << "application/x-color";
	list << SL_PYOBJECT_MIME_TYPE;
	
	return list;
}



static DataModel_Proxy *
DataModel_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	DataModel_Proxy *self = (DataModel_Proxy *)type->tp_alloc(type, 0);
	if (self) {
		self->fImpl = new DataModel_Impl();
		self->fModel = Py_None;
		Py_INCREF(Py_None);
		SL_QAPP()->newProxy((Abstract_Proxy *)self);
	}
	return self;
}


static void
DataModel_dealloc(DataModel_Proxy *self)
{
	QMutexLocker locker(SL_QAPP()->getLock());
	SL_QAPP()->deallocProxy((Abstract_Proxy *)self);
	Py_DECREF(self->fModel);
	self->ob_type->tp_free((PyObject*)self);
}


static int
DataModel_init(DataModel_Proxy *self, PyObject *args, PyObject *kwds)
{
	PyObject *object;
	if (!PyArg_ParseTuple(args, "O", &object))
		return -1;
	Py_DECREF(self->fModel);
	self->fModel = PyWeakref_NewProxy(object, NULL);
	self->fImpl->initModel(self->fModel);
	return 0;
}


SL_DEFINE_METHOD(DataModel, notify, {
	int what, index, count;
	PyObject *parent;
	
	if (!PyArg_ParseTuple(args, "iiiO", &what, &index, &count, &parent))
		return NULL;
	
	switch (what) {
	case SL_DATA_MODEL_NOTIFY_RESET:
		{
			impl->resetAll();
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_ADDED_COLUMNS:
		{
			impl->insertColumns(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_ADDED_ROWS:
		{
			impl->insertRows(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_CHANGED_COLUMNS:
		{
			impl->changeColumns(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_CHANGED_ROWS:
		{
			impl->changeRows(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_REMOVED_COLUMNS:
		{
			impl->removeColumns(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_REMOVED_ROWS:
		{
			impl->removeRows(index, count, impl->index(parent));
		}
		break;
		
	case SL_DATA_MODEL_NOTIFY_CHANGED_CELL:
		{
			impl->changeCell(index, count, impl->index(parent));
		}
		break;
	}
})



SL_START_METHODS(DataModel)
SL_METHOD(notify)
SL_END_METHODS()


PyTypeObject DataModel_Type =
{
	PyObject_HEAD_INIT(NULL)
	0,											/* ob_size */
	"_slew.DataModel",							/* tp_name */
	sizeof(DataModel_Proxy),					/* tp_basicsize */
	0,											/* tp_itemsize */
	(destructor)DataModel_dealloc,				/* tp_dealloc */
	0,											/* tp_print */
	0,											/* tp_getattr */
	0,											/* tp_setattr */
	0,											/* tp_compare */
	0,											/* tp_repr */
	0,											/* tp_as_number */
	0,											/* tp_as_sequence */
	0,											/* tp_as_mapping */
	0,											/* tp_hash */
	0,											/* tp_call */
	0,											/* tp_str */
	0,											/* tp_getattro */
	0,											/* tp_setattro */
	0,											/* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,							/* tp_flags */
	"DataModel objects",						/* tp_doc */
	0,											/* tp_traverse */
	0,											/* tp_clear */
	0,											/* tp_richcompare */
	0,											/* tp_weaklistoffset */
	0,											/* tp_iter */
	0,											/* tp_iternext */
	DataModel::methods,							/* tp_methods */
	0,											/* tp_members */
	0,											/* tp_getset */
	&Abstract_Type,								/* tp_base */
	0,											/* tp_dict */
	0,											/* tp_descr_get */
	0,											/* tp_descr_set */
	0,											/* tp_dictoffset */
	(initproc)DataModel_init,					/* tp_init */
	0,											/* tp_alloc */
	(newfunc)DataModel_new,						/* tp_new */
};


bool
DataModel_type_setup(PyObject *module)
{
	if (PyType_Ready(&DataModel_Type) < 0)
		return false;
	Py_INCREF(&DataModel_Type);
	PyModule_AddObject(module, "DataModel", (PyObject *)&DataModel_Type);
	return true;
}



#include "data_model.moc"
