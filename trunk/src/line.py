import slew

from utils import *


@factory
class Line(slew.Window):
	NAME						= 'line'
	
	#defs{SL_LINE_
	STYLE_HORIZONTAL			= 0x00000000
	STYLE_VERTICAL				= 0x00010000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"horizontal":	STYLE_HORIZONTAL,
									"vertical":		STYLE_VERTICAL,
								})),
	})
