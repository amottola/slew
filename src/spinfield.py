import slew

from utils import *
from ranged import Ranged


@factory
class SpinField(Ranged):
	NAME						= 'spinfield'
	
	#defs{SL_SPINFIELD_
	STYLE_WRAP					= 0x00020000
	#}
	
	PROPERTIES = merge(Ranged.PROPERTIES, {
		'style':				BitsProperty(merge(Ranged.STYLE_BITS, {
									'wrap':		STYLE_WRAP
								})),
	})
	
