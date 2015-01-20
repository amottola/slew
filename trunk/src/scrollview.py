import slew

from utils import *


@factory
class ScrollView(slew.Window):
	NAME						= 'scrollview'
	
	#defs{SL_SCROLLVIEW_
	STYLE_HORIZONTAL			= 0x00010000
	STYLE_VERTICAL				= 0x00020000
	STYLE_BOTH					= 0x00030000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"horizontal":	STYLE_HORIZONTAL,
									"vertical":		STYLE_VERTICAL,
									"both":			STYLE_BOTH
								})),
		'rate':					VectorProperty(),
		'scroll':				VectorProperty(),
		'viewsize':				VectorProperty(),
	})
	
# properties
	
	def get_rate(self):
		return self._impl.get_rate()
	
	def set_rate(self, rate):
		self._impl.set_rate(Vector.ensure(rate, False))
	
	def get_scroll(self):
		return self._impl.get_scroll()
	
	def set_scroll(self, scroll):
		self._impl.set_scroll(Vector.ensure(scroll, False))
	
	def get_viewsize(self):
		return self._impl.get_viewsize()
	
	def set_viewsize(self, viewsize):
		self._impl.set_viewsize(Vector.ensure(viewsize, False))

	def get_viewport_size(self):
		return self._impl.get_viewport_size()
	
	rate = DeprecatedDescriptor('rate')
	scroll = DeprecatedDescriptor('scroll')
	viewsize = DeprecatedDescriptor('viewsize')



