import slew

from utils import *
from gdi import *
from view import View


@factory
class Grid(View):
	
	NAME						= 'grid'
	
	#defs{SL_GRID_
	STYLE_ALT_ROWS				= 0x00010000
	STYLE_HEADER				= 0x00020000
	STYLE_SORTABLE				= 0x00040000
	STYLE_SELECT_ROWS			= 0x00080000
	STYLE_RULES					= 0x00100000
	STYLE_READONLY				= 0x00200000
	STYLE_VERTICAL_HEADER		= 0x00400000
	STYLE_AUTO_ROWS				= 0x00800000
	STYLE_SINGLE				= 0x00000000
	STYLE_MULTI					= 0x01000000
	STYLE_NO_SELECTION			= 0x02000000
	STYLE_DELAYED_EDIT			= 0x04000000
	STYLE_AUTO_SCROLL			= 0x08000000
	#}
	
	PROPERTIES = merge(View.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'altrows':				STYLE_ALT_ROWS,
									'header':				STYLE_HEADER,
									'sortable':				STYLE_SORTABLE,
									'selectrows':			STYLE_SELECT_ROWS,
									'rules':				STYLE_RULES,
									'readonly':				STYLE_READONLY,
									'vheader':				STYLE_VERTICAL_HEADER,
									'autorows':				STYLE_AUTO_ROWS,
									'single':				STYLE_SINGLE,
									'multi':				STYLE_MULTI,
									'noselection':			STYLE_NO_SELECTION,
									'delayedit':			STYLE_DELAYED_EDIT,
									'autoscroll':			STYLE_AUTO_SCROLL,
								})),
		'row':					IntProperty(),
		'column':				IntProperty(),
		'ascending':			BoolProperty(),
		'sortby':				IntProperty(),
	})
	
# methods
	
	def edit(self, index, insertion_point=-1):
		self._impl.edit(slew.DataIndex.ensure(index), insertion_point)
	
	def set_cell_span(self, row, column, rowspan=1, colspan=1):
		self._impl.set_cell_span(row, column, rowspan, colspan)
	
	def get_row_span(self, row, column):
		return self._impl.get_row_span(row, column)
	
	def get_column_span(self, row, column):
		return self._impl.get_column_span(row, column)
	
	def get_column_width(self, column):
		return self._impl.get_column_width(column)
	
	def set_column_width(self, column, width):
		return self._impl.set_column_width(column, width)
	
	def get_column_pos(self, column):
		return self._impl.get_column_pos(column)
	
	def set_column_pos(self, column, pos):
		self._impl.set_column_pos(column, pos)
	
	def set_sorting(self, column, ascending=None):
		self._impl.set_sorting(-1 if column is None else column, ascending)
	
	def complete(self):
		self._impl.complete()
	
	def get_insertion_point(self):
		return self._impl.get_insertion_point()
	
	def get_edit_text(self):
		return self._impl.get_edit_text()
	
	def select_all(self):
		self._impl.select_all()
	
	def move_cursor(self, dx, dy):
		self._impl.move_cursor(dx, dy)
	
	def set_default_row_height(self, height):
		self._impl.set_default_row_height(height)
	
	@deprecated
	def set_row_height(self, height):
		self.set_default_row_height(height)
	
	def set_column_hidden(self, column, hidden):
		self._impl.set_column_hidden(column, hidden)
	
	def is_column_hidden(self, column):
		return self._impl.is_column_hidden(column)

# properties
	
	def get_row(self):
		return self._impl.get_row()
	
	def set_row(self, row):
		self._impl.set_row(row)
	
	def get_column(self):
		return self._impl.get_column()
	
	def set_column(self, column):
		self._impl.set_column(column)
	
	def is_ascending(self):
		return self._impl.is_ascending()
	
	def set_ascending(self, ascending):
		self._impl.set_ascending(ascending)
	
	def get_sortby(self):
		return self._impl.get_sortby()
	
	def set_sortby(self, column):
		self._impl.set_sortby(column)
	
	row = DeprecatedDescriptor('row')
	column = DeprecatedDescriptor('column')
	ascending = DeprecatedBoolDescriptor('ascending')
	sortby = DeprecatedDescriptor('sortby')

