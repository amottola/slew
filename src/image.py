import slew

from utils import *
from gdi import *


@factory
class Image(slew.Window):
	NAME						= 'image'
	
	#defs{SL_IMAGE_
	STYLE_FIT					= 0x00010000
	STYLE_SCALE					= 0x00020000
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'bitmap':				BitmapProperty(),
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'fit':			STYLE_FIT,
									'scale':		STYLE_SCALE,
								})),
		'align':				BitsProperty({	'top':		slew.ALIGN_TOP,
												'bottom':	slew.ALIGN_BOTTOM,
												'left':		slew.ALIGN_LEFT,
												'right':	slew.ALIGN_RIGHT,
												'hcenter':	slew.ALIGN_HCENTER,
												'vcenter':	slew.ALIGN_VCENTER,
												'center':	slew.ALIGN_CENTER }),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__bitmap = None
	
# properties
		
	def get_bitmap(self):
		return self.__bitmap
	
	def set_bitmap(self, bitmap):
		self.__bitmap = Bitmap.ensure(bitmap)
		self._impl.set_bitmap(self.__bitmap)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	bitmap = DeprecatedDescriptor('bitmap')
	align = DeprecatedDescriptor('align')



