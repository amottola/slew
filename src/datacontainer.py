import slew

from utils import *



class DataItem(object): pass

class ListDataItem(DataItem):
	def __init__(self, text, value="", enabled=True):
		self.text = text
		self.value = value
		self.enabled = enabled
	
	def __cmp__(self, other):
		if isinstance(other, basestring):
			return cmp(self.text, other)
		elif isinstance(other, ListDataItem):
			if (self.text == other.text):
				return cmp(self.value, other.value)
			return cmp(self.text, other.text)
		return 1
	
	def __str__(self):
		return 'ListDataItem("%s", "%s")' % (self.text, self.value)
	
	@classmethod
	def ensure(cls, value):
		if isinstance(value, basestring):
			value = ListDataItem(value)
		if not isinstance(value, ListDataItem):
			raise ValueError("expecting 'ListDataItem' object")
		return value



class ListDataModel(slew.DataModel):
	def __init__(self):
		self.__items = []
	
	def row_count(self, index=None):
		if index is None:
			return len(self.__items)
		return 0
	
	def data(self, index):
		d = slew.DataSpecifier()
		if index.row < len(self.__items):
			item = self.__items[index.row]
			d.text = item.text
			if not item.enabled:
				d.flags &= ~d.ENABLED
		if not d.text:
			d.flags |= d.SEPARATOR
		d.flags |= d.DROP_TARGET
		return d
	
	def get_items(self):
		return self.__items
	
	def set_items(self, items):
		self.__items = items



class DataContainer(slew.Window):
	
	STYLE_SORTED				= 0x80000000
	
	STYLE_BITS = merge(slew.Window.STYLE_BITS, {
		'sorted':				STYLE_SORTED,					# TODO!!!
	})
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'model':				Property(),
		'model_column':			IntProperty(),
		'items':				Property(),
		'selection':			Property(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.set_model(None)
	
	def load(self, xml='', globals=None, locals=None, node=None):
		if node is None:
			node = parse_xml(xml)
		
		if node.tag != self.NAME:
			raise ValueError("tag mismatch, expecting '%s'" % self.NAME)
		
		self._load_properties(node, globals, locals)
		
		items = []
		selection = []
		for child in node.getchildren():
			if child.tag == 'option':
				if BoolProperty.is_true(child.attrib.get('selected', 'false')):
					selection.append(len(items))
				items.append(ListDataItem(child.text or '', child.attrib.get('value', ''), child.attrib.get('enabled', 'true').lower() in ('t', 'true', 'on', '1')))
		self.set_items(items)
		self.set_selection(selection or None)
	
# methods
	
	def item_at(self, index):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		return self.__model.get_items()[index]
	
	def index_of(self, item):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		return self.__model.get_items().index(ListDataItem.ensure(item))
	
	def append(self, item):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		index = len(self.__model.get_items())
		self.insert(index, item)
	
	def insert(self, index, item):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		items = self.__model.get_items()
		items.insert(index, ListDataItem.ensure(item))
		self.__model.notify(ListDataModel.NOTIFY_ADDED_ROWS, index, 1)
	
	def replace(self, index, item):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		self.__model.get_items()[index] = ListDataItem.ensure(item)
		self.__model.notify(ListDataModel.NOTIFY_CHANGED_ROWS, index, 1)
	
	def remove(self, index):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		self.__model.get_items().pop(index)
		self.__model.notify(ListDataModel.NOTIFY_REMOVED_ROWS, index, 1)
	
	def count(self):
		return self.__model.row_count()
	
	def refresh(self):
		self.__model.notify(ListDataModel.NOTIFY_RESET)
	
	@deprecated
	def get_index(self, text=None, value=None):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		items = self.__model.get_items()
		if text is not None:
			for index, item in enumerate(items):
				if unicode(item.text) == unicode(text):
					return index
		elif value is not None:
			for index, item in enumerate(items):
				if unicode(item.value) == unicode(value):
					return index
		raise LookupError('item not found in control')
	
	@deprecated
	def get(self, text):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		items = self.__model.get_items()
		return items[self.get_index(text)]
	
	@deprecated
	def set(self, item, text):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		items = self.__model.get_items()
		index = self.index_of(item)
		items[index].text = text
		self.__model.notify(ListDataModel.NOTIFY_CHANGED_ROWS, index, 1)
	
# properties
	
	def get_model(self):
		return self.__model
	
	def set_model(self, model):
		if model is None:
			model = ListDataModel()
		else:
			model = slew.DataModel.ensure(model)
		self._impl.set_model(model)
		self.__model = model
	
	def get_model_column(self):
		return self._impl.get_model_column()
	
	def set_model_column(self, column):
		self._impl.set_model_column(column)
	
	def get_items(self):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		return list(self.__model.get_items())
	
	def set_items(self, items):
		if not isinstance(self.__model, ListDataModel):
			raise RuntimeError("method is unavailable when using a custom model")
		self.set_selection(None)
		self.__model.set_items([ ListDataItem.ensure(x) for x in items ])
		self.__model.notify(ListDataModel.NOTIFY_RESET)
		if len(items) > 0:
			self.set_selection(0)
	
	def get_selection(self):
		pass
	
	def set_selection(self, selection):
		pass
	
	def get_selection_value(self):
		return self.__model.get_items()[self.get_selection()].value
	
	def set_selection_value(self, value):
		for index, item in enumerate(self.__model.get_items()):
			if item.value == value:
				self.set_selection(index)
				break
		else:
			raise ValueError("value not found")




