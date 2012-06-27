import slew

from utils import *
from gdi import *


@factory
class Radio(slew.Window):
	NAME						= 'radio'
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'group':				StringProperty(),
		'value':				StringProperty(),
		'label':				TranslatedStringProperty(),
		'selected':				BoolProperty(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__value = ''
	
# properties
	
	def get_group(self):
		return self._impl.get_group()
	
	def set_group(self, group):
		self._impl.set_group(group)
	
	def get_value(self):
		return self.__value
	
	def set_value(self, value):
		self.__value = value
	
	def get_label(self):
		return self._impl.get_label()
	
	def set_label(self, label):
		self._impl.set_label(label)
	
	def is_selected(self):
		return self._impl.is_selected()
	
	def set_selected(self, selected):
		self._impl.set_selected(selected)
	
	group = DeprecatedDescriptor('group')
	value = DeprecatedDescriptor('value')
	label = DeprecatedDescriptor('label')
	selected = DeprecatedBoolDescriptor('selected')



