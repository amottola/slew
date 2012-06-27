import slew

from utils import *


@factory
class SplitView(slew.Window):
	NAME						= 'splitview'
	
	#defs{SL_SPLITVIEW_
	STYLE_HORIZONTAL			= 0x00010000
	STYLE_VERTICAL				= 0x00020000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"horizontal":	STYLE_HORIZONTAL,
									"vertical":		STYLE_VERTICAL
								})),
		'sizes':				IntListProperty(),
	})
	
# properties
	
	def get_sizes(self):
		return self._impl.get_sizes()
	
	def set_sizes(self, sizes):
		self._impl.set_sizes(sizes)
	
	sizes = DeprecatedDescriptor('sizes')



