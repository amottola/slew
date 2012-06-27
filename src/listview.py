import slew

from utils import *
from gdi import *
from view import View


@factory
class ListView(View):
	
	NAME						= 'listview'
	
	#defs{SL_LISTVIEW_
	STYLE_LIST					= 0x00000000
	STYLE_ICON					= 0x00010000
	STYLE_FLOW_VERTICAL			= 0x00000000
	STYLE_FLOW_HORIZONTAL		= 0x00020000
	STYLE_FIXED_RESIZE			= 0x00040000
	STYLE_WRAP					= 0x00080000
	STYLE_SINGLE				= 0x00000000
	STYLE_MULTI					= 0x00100000
	#}
	
	STYLE_BITS = merge(slew.Window.STYLE_BITS, {
		'list':					STYLE_LIST,
		'icon':					STYLE_ICON,
		'vflow':				STYLE_FLOW_VERTICAL,
		'hflow':				STYLE_FLOW_HORIZONTAL,
		'fixed':				STYLE_FIXED_RESIZE,
		'wrap':					STYLE_WRAP,
		'single':				STYLE_SINGLE,
		'multi':				STYLE_MULTI,
	})
	
	PROPERTIES = merge(View.PROPERTIES, {
		'style':				BitsProperty(STYLE_BITS),
		'model_column':			IntProperty(),
		'gridsize':				VectorProperty(),
		'iconsize':				VectorProperty(),
		'spacing':				IntProperty(),
	})
	
	def index_to_selection(self, index):
		return index
	
# properties
	
	def get_model_column(self):
		return self._impl.get_model_column()
	
	def set_model_column(self, column):
		self._impl.set_model_column(column)
	
	def get_selection(self):
		return self._impl.get_selection() or None
	
	def get_grid_size(self):
		return self._impl.get_grid_size()
	
	def set_grid_size(self, size):
		self._impl.set_grid_size(Vector.ensure(size, False))
	
	def get_icon_size(self):
		return self._impl.get_icon_size()
	
	def set_icon_size(self, size):
		self._impl.set_icon_size(Vector.ensure(size, False))
	
	def get_spacing(self):
		return self._impl.get_spacing()
	
	def set_spacing(self, spacing):
		self._impl.set_spacing(spacing)
	
	get_iconsize = get_icon_size
	set_iconsize = set_icon_size
	get_gridsize = get_grid_size
	set_gridsize = set_grid_size
	
	


