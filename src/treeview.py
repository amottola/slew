import slew

from utils import *
from gdi import *
from view import View


@factory
class TreeView(View):
	
	NAME						= 'treeview'
	
	#defs{SL_TREEVIEW_
	
	STYLE_SINGLE				= 0x00000000
	STYLE_MULTI					= 0x00010000
	STYLE_NO_SELECTION			= 0x00020000
	STYLE_EXPANDERS				= 0x00040000
	STYLE_SELECT_ROWS			= 0x00080000
	STYLE_HEADER				= 0x00100000
	STYLE_ANIMATED				= 0x00200000
	STYLE_RULES					= 0x00400000
	STYLE_DELAYED_EDIT			= 0x00800000
	STYLE_ALT_ROWS				= 0x01000000
	STYLE_READONLY				= 0x02000000
	STYLE_FIT_COLS				= 0x04000000
	#}
	
	PROPERTIES = merge(View.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'single':				STYLE_SINGLE,
									'multi':				STYLE_MULTI,
									'noselection':			STYLE_NO_SELECTION,
									'expanders':			STYLE_EXPANDERS,
									'selectrows':			STYLE_SELECT_ROWS,
									'header':				STYLE_HEADER,
									'animated':				STYLE_ANIMATED,
									'rules':				STYLE_RULES,
									'delayedit':			STYLE_DELAYED_EDIT,
									'altrows':				STYLE_ALT_ROWS,
									'readonly':				STYLE_READONLY,
									'fitcols':				STYLE_FIT_COLS,
								})),
	})
	
# methods
	
	def edit(self, index, insertion_point=-1):
		self._impl.edit(slew.DataIndex.ensure(index), insertion_point)
	
	def set_expanded(self, index, expanded, recursive=False):
		index = slew.DataIndex.ensure(index)
		model = self.get_model()
		self._impl.set_expanded(index, expanded)
		if recursive and model:
			for row in xrange(0, model.row_count(index)):
				self.set_expanded(model.index(row, 0, index), expanded, True)
	
	def is_expanded(self, index):
		return self._impl.is_expanded(slew.DataIndex.ensure(index, False))
	
	def expand(self, index=None, children=False, expand=True):
		self.set_expanded(index, expand, children)
	
	def collapse(self, index=None, children=False):
		self.set_expanded(index, False, children)
	
	def expand_all(self, index=None):
		self.set_expanded(index, True, True)
	
	def collapse_all(self, index=None):
		self.set_expanded(index, False, True)
	
	def set_span_first_column(self, row, enabled, parent=None):
		self._impl.set_span_first_column(row, enabled, slew.DataIndex.ensure(parent))
	
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
	
# properties
	
	def get_indentation(self):
		return self._impl.get_indentation()
	
	def set_indentation(self, indentation):
		self._impl.set_indentation(indentation)
	

