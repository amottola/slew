import slew

from utils import *
from gdi import *


@factory
class Label(slew.Window):
	NAME						= 'label'
	
	#defs{SL_LABEL_
	STYLE_HTML					= 0x00010000
	STYLE_SELECTABLE			= 0x00020000
	STYLE_FOLLOW_LINKS			= 0x00040000
	STYLE_WRAP					= 0x00080000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'html':			STYLE_HTML,
									'selectable':	STYLE_SELECTABLE,
									'links':		STYLE_FOLLOW_LINKS,
									'wrap':			STYLE_WRAP,
								})),
		'text':					TranslatedStringProperty(True),
		'align':				BitsProperty({	'top':		slew.ALIGN_TOP,
												'bottom':	slew.ALIGN_BOTTOM,
												'left':		slew.ALIGN_LEFT,
												'right':	slew.ALIGN_RIGHT,
												'hcenter':	slew.ALIGN_HCENTER,
												'vcenter':	slew.ALIGN_VCENTER,
												'center':	slew.ALIGN_CENTER }),
	})
	
# properties
		
	def get_text(self):
		return self._impl.get_text()
	
	def set_text(self, text):
		self._impl.set_text(text)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	text = DeprecatedDescriptor('text')
	align = DeprecatedDescriptor('align')



