import slew

from utils import *


class Ranged(slew.Window):
	
	#defs{SL_RANGED_
	STYLE_HORIZONTAL			= 0x00000000
	STYLE_VERTICAL				= 0x00010000
	#}
	
	STYLE_BITS = merge(slew.Window.STYLE_BITS, {
		'horizontal':			STYLE_HORIZONTAL,
		'vertical':				STYLE_VERTICAL
	})
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(STYLE_BITS),
		'min':					IntProperty(),
		'max':					IntProperty(),
		'value':				IntProperty(),
	})
	
# properties
	
	def get_min(self):
		return self._impl.get_min()
	
	def set_min(self, min):
		self._impl.set_min(min)
	
	def get_max(self):
		return self._impl.get_max()
	
	def set_max(self, max):
		self._impl.set_max(max)
	
	def get_value(self):
		return self._impl.get_value()
	
	def set_value(self, value):
		self._impl.set_value(value)
	
	min = DeprecatedDescriptor('min')
	max = DeprecatedDescriptor('max')
	value = DeprecatedDescriptor('value')



