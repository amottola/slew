import slew

from utils import *
from gdi import *


@factory
class TextField(slew.Window):
	NAME						= 'textfield'
	
	#defs{SL_TEXTFIELD_
	STYLE_ENTERTABS				= 0x00010000
	STYLE_READONLY				= 0x00020000
	STYLE_PASSWORD				= 0x00040000
	STYLE_SELECT				= 0x00080000
	STYLE_CAPS					= 0x00100000
	STYLE_NOFRAME				= 0x00200000
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'entertabs':		STYLE_ENTERTABS,
									'readonly':			STYLE_READONLY,
									'password':			STYLE_PASSWORD,
									'select':			STYLE_SELECT,
									'caps':				STYLE_CAPS,
								})),
		'value':				StringProperty(),
		'length':				IntProperty(),
		'align':				BitsProperty({
									'left':				slew.ALIGN_LEFT,
									'center':			slew.ALIGN_HCENTER,
									'right':			slew.ALIGN_RIGHT,
								}),
		'filter':				StringProperty(),
		'format':				StringProperty(),
		'icon':					IconProperty(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__format = ''
		self.__format_vars = {}
		self.__icon = None
		self.__completer = None
	
# methods
	
	def cut(self):
		self._impl.cut()
	
	def copy(self):
		self._impl.copy()
	
	def paste(self):
		self._impl.paste()
	
	def delete(self):
		self._impl.delete()
	
	def append(self, text):
		self._impl.insert(-1, text)
	
	def insert(self, position, text):
		self._impl.insert(position, text)
	
	def is_modified(self):
		return self._impl.is_modified()
	
	def is_valid(self):
		return self._impl.is_valid()
	
	def undo(self):
		self._impl.undo()
	
	def redo(self):
		self._impl.redo()
	
	def is_undo_available(self):
		return self._impl.is_undo_available()
	
	def is_redo_available(self):
		return self._impl.is_redo_available()
	
	def select_all(self):
		self.set_selection()
	
	def complete(self):
		self._impl.complete()
	
# properties
	
	def get_selection(self):
		start, end = self._impl.get_selection()
		if (start < 0) or (end < 0):
			return None
		return (start, end)
	
	def set_selection(self, start=None, end=None):
		if start is None:
			start = -1
		if end is None:
			end = -1
		self._impl.set_selection(start, end)
	
	def get_insertion_point(self):
		return self._impl.get_insertion_point()
	
	def set_insertion_point(self, insertion_point):
		self._impl.set_insertion_point(insertion_point)
	
	def get_value(self):
		return self._impl.get_value()
	
	def set_value(self, value):
		self._impl.set_value(value)
	
	def get_length(self):
		return self._impl.get_length()
	
	def set_length(self, length):
		self._impl.set_length(length)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	def get_filter(self):
		return self._impl.get_filter()
	
	def set_filter(self, filter):
		self._impl.set_filter(filter)
	
	def set_datatype(self, datatype):
		self._impl.set_datatype(datatype)
		self.set_format(self.__format)
	
	def get_format(self):
		return self.__format
	
	def set_format(self, format):
		self.__format = format
		self._impl.set_format(slew.normalize_format(format, self.__format_vars))
	
	def get_format_vars(self):
		return self.__format_vars
	
	def set_format_vars(self, format_vars):
		self.__format_vars = format_vars or {}
		self.set_format(self.__format)
	
	def get_completer(self):
		return self.__completer
	
	def set_completer(self, completer):
		c = slew.Completer.ensure(completer)
		self.__completer = c
		if c is None:
			self._impl.set_completer(None, 0, None, None, None, None)
		else:
			self._impl.set_completer(slew.DataModel.ensure(c.model), c.column, Color.ensure(c.color), Color.ensure(c.bgcolor), Color.ensure(c.hicolor), Color.ensure(c.hibgcolor))
	
	def get_icon(self):
		return self.__icon
	
	def set_icon(self, icon):
		self.__icon = Icon.ensure(icon)
		self._impl.set_icon(self.__icon)
	
	value = DeprecatedDescriptor('value')
	length = DeprecatedDescriptor('length')
	align = DeprecatedDescriptor('align')
	filter = DeprecatedDescriptor('filter')
	format = DeprecatedDescriptor('format')
	valid = DeprecatedBoolDescriptor('valid')
	completer = DeprecatedDescriptor('completer')
	icon = DeprecatedDescriptor('icon')


