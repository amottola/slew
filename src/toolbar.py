import slew

from utils import *


@factory
class ToolBar(slew.Window):
	NAME						= 'toolbar'
	
	#defs{SL_TOOLBAR_
	STYLE_TEXT					= 0x1
	STYLE_ICON					= 0x2
	STYLE_HORIZONTAL			= 0x0
	STYLE_VERTICAL				= 0x4
	STYLE_DEFAULT				= STYLE_TEXT | STYLE_ICON
	#}
	
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'style':				BitsProperty({
									'text':			STYLE_TEXT,
									'icon':			STYLE_ICON,
									'horizontal':	STYLE_HORIZONTAL,
									'vertical':		STYLE_VERTICAL,
								}),
		'align':				BitsProperty({
									'top':			slew.ALIGN_TOP,
									'bottom':		slew.ALIGN_BOTTOM,
									'left':			slew.ALIGN_LEFT,
									'right':		slew.ALIGN_RIGHT,
								}),											# should really be ChoiceProperty, not used here for compatibility
	})
	
# properties
	
	def get_style(self):
		return self._impl.get_style()
	
	def set_style(self, style):
		self._impl.set_style(style)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	style = DeprecatedDescriptor('style')
	align = DeprecatedDescriptor('align')


