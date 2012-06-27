import slew

from utils import *
from ranged import Ranged


@factory
class Slider(Ranged):
	NAME						= 'slider'
	
	#defs{SL_SLIDER_
	STYLE_INVERSE				= 0x00020000
	
	TICKS_NONE					= 0
	TICKS_TOP					= 1
	TICKS_BOTTOM				= 2
	TICKS_LEFT					= 3
	TICKS_RIGHT					= 4
	#}
	
	PROPERTIES = merge(Ranged.PROPERTIES, {
		'style':				BitsProperty(merge(Ranged.STYLE_BITS, {
									'inverse':	STYLE_INVERSE
								})),
		'ticks':				ChoiceProperty(["none", "top", "bottom", "left", "right"]),
	})
	
# properties
	
	def get_ticks(self):
		return self._impl.get_ticks()
	
	def set_ticks(self, ticks):
		self._impl.set_ticks(ticks)
	
	ticks = DeprecatedDescriptor('ticks')



