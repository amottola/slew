import slew

from utils import *


@factory
class TabViewPage(slew.Window):
	NAME						= 'tabviewpage'
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'title':				TranslatedStringProperty(),
		'icon':					IconProperty(),
	})
	
# properties
	
	def get_title(self):
		return self._impl.get_title()
	
	def set_title(self, title):
		self._impl.set_title(title)
	
	def get_icon(self):
		return self._impl.get_icon()
	
	def set_icon(self, icon):
		self._impl.set_position(Icon.ensure(icon))
	
	title = DeprecatedDescriptor('title')
	icon = DeprecatedDescriptor('icon')



