import slew

from utils import *


@factory
class Menu(slew.Widget):
	NAME						= 'menu'
	
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'title':				TranslatedStringProperty(),
		'enabled':				BoolProperty(),
	})
	
# methods
	
	def popup(self, pos=None):
		return self._impl.popup(Vector.ensure(pos))
	
# properties
	
	def get_title(self):
		return self._impl.get_title()
	
	def set_title(self, title):
		self._impl.set_title(title)
	
	def is_enabled(self):
		return self._impl.is_enabled()
	
	def set_enabled(self, enabled):
		self._impl.set_enabled(enabled)
	
	title = DeprecatedDescriptor('title')
	enabled = DeprecatedBoolDescriptor('enabled')
	
	@classmethod
	def ensure(cls, menu):
		if (menu is None) or isinstance(menu, Menu):
			return menu
		raise TypeError("expected 'Menu' or None")



