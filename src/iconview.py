import slew

from listview import ListView
from utils import *


@factory
class IconView(ListView):
	
	NAME						= 'iconview'
	
	#defs{SL_ICONVIEW_
	STYLE_WIDE_SEL				= 0x01000000
	#}
	
	PROPERTIES = merge(ListView.PROPERTIES, {
		'style':				BitsProperty(merge(ListView.STYLE_BITS, {
									"widesel":		STYLE_WIDE_SEL
								})),
		'column':				IntProperty()
	})
	
	def create_impl(self, className):
		return ListView.create_impl(self, 'ListView')
	
	def initialize(self):
		self.set_style(0)
		self.set_spacing(2)
		ListView.initialize(self)
	
	def index_to_selection(self, index):
		return -1 if index is None else index.row
	
# properties
	
	def set_style(self, style):
		style |= ListView.STYLE_ICON | ListView.STYLE_FLOW_HORIZONTAL | ListView.STYLE_WRAP
		ListView.set_style(self, style)
	
	def get_selection(self):
		selection = self._impl.get_selection()
		if selection:
			return selection[0].row
		else:
			return -1
	
	def set_selection(self, selection):
		selection = int(selection)
		if selection < 0:
			selection = ()
		else:
			selection = ( self.get_model().index(selection), )
		self._impl.set_selection(selection)
	
	def get_column(self):
		return self.get_model_column()
	
	def set_column(self, column):
		return self.set_model_column(column)
	
	model = DeprecatedDescriptor('model')
	column = DeprecatedDescriptor('column')
	selection = DeprecatedDescriptor('selection')
	iconsize = DeprecatedDescriptor('iconsize')
	gridsize = DeprecatedDescriptor('gridsize')

