import slew

from datacontainer import DataContainer
from utils import *


@factory
class ComboBox(DataContainer):
	
	#defs{SL_COMBOBOX_
	STYLE_ENTERTABS				= 0x00010000
	STYLE_EDITABLE				= 0x00020000
	STYLE_READONLY				= 0x00040000
	#}
	
	NAME						= 'combobox'
	
	PROPERTIES = merge(DataContainer.PROPERTIES, {
		'style':				BitsProperty(merge(DataContainer.STYLE_BITS, {
									"entertabs":	STYLE_ENTERTABS,
									"editable":		STYLE_EDITABLE,
									"readonly":		STYLE_READONLY,
									"system":		0,					# for compatibility!
								})),
		'filter':				StringProperty(),
	})
	
# properties
	
	def get_selection(self):
		return self._impl.get_selection()
	
	def set_selection(self, selection):
		if selection is None:
			selection = -1
		elif isinstance(selection, (tuple, list)):
			selection = selection[0]
		self._impl.set_selection(selection)
	
	def get_value(self):
		return self._impl.get_value()
	
	def set_value(self, value):
		self._impl.set_value(value)
	
	def get_filter(self):
		return self._impl.get_filter()
	
	def set_filter(self, filter):
		self._impl.set_filter(filter)
	
	def open_popup(self):
		self._impl.open_popup()
	
	def close_popup(self):
		self._impl.close_popup()
	
	items = DeprecatedDescriptor('items')
	selection = DeprecatedDescriptor('selection')
	value = DeprecatedDescriptor('value')


