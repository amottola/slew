import slew

from utils import *
from gdi import *


@factory
class CheckBox(slew.Window):
	NAME						= 'checkbox'
	
	#defs{SL_CHECKBOX_
	STYLE_READONLY				= 0x00010000
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"readonly":		STYLE_READONLY,
								})),
		'label':				TranslatedStringProperty(),
		'checked':				BoolProperty(),
	})
	
# properties
	
	def get_label(self):
		return self._impl.get_label()
	
	def set_label(self, label):
		self._impl.set_label(label)
	
	def is_checked(self):
		return self._impl.is_checked()
	
	def set_checked(self, checked):
		self._impl.set_checked(checked)
	
	label = DeprecatedDescriptor('label')
	checked = DeprecatedBoolDescriptor('checked')



