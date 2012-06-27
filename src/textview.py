import slew

from utils import *
from gdi import *


@factory
class TextView(slew.Window):
	NAME						= 'textview'
	
	#defs{SL_TEXTVIEW_
	STYLE_READONLY				= 0x00010000
	STYLE_NOWRAP				= 0x00020000
	STYLE_LINENUMS				= 0x00040000
	STYLE_HTML					= 0x00080000
	STYLE_TABFOCUS				= 0x00100000
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'readonly':		STYLE_READONLY,
									'nowrap':		STYLE_NOWRAP,
									'linenums':		STYLE_LINENUMS,
									'html':			STYLE_HTML,
									'tabfocus':		STYLE_TABFOCUS,
								})),
		'value':				StringProperty(),
		'length':				IntProperty(),
		'align':				BitsProperty({
									'left':				slew.ALIGN_LEFT,
									'center':			slew.ALIGN_HCENTER,
									'right':			slew.ALIGN_RIGHT,
								}),
		'filter':				StringProperty(),
		'line':					IntProperty(),
		'column':				IntProperty(),
		'current_line_color':	ColorProperty(),
		'tab_width':			IntProperty(),
		'cursor_width':			IntProperty(),
	})
	
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
	
	def set_modified(self, modified):
		self._impl.set_modified(modified)
	
	def undo(self):
		self._impl.undo()
	
	def redo(self):
		self._impl.redo()
	
	def is_undo_available(self):
		return self._impl.is_undo_available()
	
	def is_redo_available(self):
		return self._impl.is_redo_available()
	
	def set_highlighted_lines(self, lines):
		color = Color(255, 220, 220)
		if isinstance(lines, (tuple, list)):
			data = {}
			for line in lines:
				data[line] = color
			lines = data
		self._impl.set_highlighted_lines(lines or {})
		
	def set_syntax(self, syntax):
		self._impl.set_syntax(syntax or '')
	
	def select_all(self):
		self.set_selection()
	
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
	
	def get_position(self, line, column):
		return self._impl.get_position(line, column)
	
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
	
	def get_line(self, pos=None):
		return self._impl.get_line(pos)
	
	def set_line(self, line):
		self._impl.set_line(int(line))
	
	def get_column(self, pos=None):
		return self._impl.get_column(pos)
	
	def set_column(self, column):
		self._impl.set_column(int(column))
	
	def get_current_line_color(self):
		return self._impl.get_current_line_color()
	
	def set_current_line_color(self, color):
		self._impl.set_current_line_color(color)
	
	def get_tab_width(self):
		return self._impl.get_tab_width()
	
	def set_tab_width(self, width):
		self._impl.set_tab_width(int(width))
	
	def get_cursor_width(self):
		return self._impl.get_cursor_width()
	
	def set_cursor_width(self, width):
		self._impl.set_cursor_width(int(width))
	
	value = DeprecatedDescriptor('value')
	length = DeprecatedDescriptor('length')
	align = DeprecatedDescriptor('align')
	filter = DeprecatedDescriptor('filter')


