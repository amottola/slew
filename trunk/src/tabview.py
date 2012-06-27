import slew

from utils import *


@factory
class TabView(slew.Window):
	NAME						= 'tabview'
	
	#defs{SL_TABVIEW_
	STYLE_DOCUMENT				= 0x00010000
	STYLE_SCROLL				= 0x00020000
	STYLE_TRIANGULAR			= 0x00040000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"document":		STYLE_DOCUMENT,
									"scroll":		STYLE_SCROLL,
									"triangular":	STYLE_TRIANGULAR,
								})),
		'page':					IntProperty(),
		'align':				BitsProperty({		'top':		slew.ALIGN_TOP,
													'center':	slew.ALIGN_TOP,
													'bottom':	slew.ALIGN_BOTTOM,
													'left':		slew.ALIGN_LEFT,
													'right':	slew.ALIGN_RIGHT
								}),
		'position':				ChoiceProperty({	'top':		slew.ALIGN_TOP,
													'center':	slew.ALIGN_TOP,
													'bottom':	slew.ALIGN_BOTTOM,
													'left':		slew.ALIGN_LEFT,
													'right':	slew.ALIGN_RIGHT
								}),
	})
	
# properties
	
	def get_page(self):
		return self._impl.get_page()
	
	def set_page(self, page):
		if isinstance(page, slew.TabViewPage):
			page = self.get_children().index(page)
		self._impl.set_page(page)
	
	def get_align(self):
		return self._impl.get_position()
	
	def set_align(self, align):
		self._impl.set_position(align)
	
	def get_position(self):
		return self._impl.get_position()
	
	def set_position(self, position):
		self._impl.set_position(position)
	
	page = DeprecatedDescriptor('page')
	align = DeprecatedDescriptor('align')



