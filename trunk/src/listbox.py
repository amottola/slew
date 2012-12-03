import slew

from datacontainer import DataContainer
from listview import ListView
from utils import *


@factory
class ListBox(DataContainer, ListView):
	
	STYLE_SINGLE				= ListView.STYLE_SINGLE
	STYLE_MULTI					= ListView.STYLE_MULTI
	
	NAME						= 'listbox'
	
	PROPERTIES = merge(DataContainer.PROPERTIES, {
		'style':				BitsProperty(merge(DataContainer.STYLE_BITS, {
									"single":		STYLE_SINGLE,
									"multi":		STYLE_MULTI,
								})),
		'selection':			Property(),
	})
	
	def create_impl(self, className):
		return ListView.create_impl(self, 'ListView')
	
	def initialize(self):
		DataContainer.initialize(self)
		self.__style = self.get_style()
		self.__style &= ~ListView.STYLE_WRAP
		self.set_style(self.__style)
	
	def index_to_selection(self, index):
		return None if index is None else index.row
	
# properties
	
	def set_style(self, style):
		self.__style = style
		ListView.set_style(self, style)
	
	def get_selection(self):
		selection = self._impl.get_selection()
		if self.__style & ListBox.STYLE_MULTI:
			return [ x.row for x in selection ]
		if selection:
			return selection[0].row
	
	def set_selection(self, selection):
		if selection is None:
			selection = ()
		elif not isinstance(selection, (tuple, list)):
			selection = ( selection, )
		model = self.get_model()
		self._impl.set_selection([ model.index(x) for x in selection ])
	
	items = DeprecatedDescriptor('items')
	selection = DeprecatedDescriptor('selection')


