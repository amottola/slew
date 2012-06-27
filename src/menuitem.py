import slew

from utils import *


@factory
class MenuItem(slew.Widget):
	NAME						= 'menuitem'
	
	#defs{SL_MENU_ITEM_
	TYPE_NORMAL					= 0
	TYPE_SEPARATOR				= 1
	TYPE_CHECK					= 2
	TYPE_RADIO					= 3
	TYPE_QUIT					= 4
	TYPE_ABOUT					= 5
	TYPE_PREFERENCES			= 6
	TYPE_APPLICATION			= 7
	#}
	
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'type':					ChoiceProperty(["normal", "separator", "check", "radio", "quit", "about", "preferences", "application"]),
		'text':					TranslatedStringProperty(),
		'description':			TranslatedStringProperty(),
		'checked':				BoolProperty(),
		'enabled':				BoolProperty(),
		'bitmap':				BitmapProperty(),
		'icon':					IconProperty(),
	})
	
	def initialize(self):
		slew.Widget.initialize(self)
		self.__icon = None
		
# methods
	
	def popup(self, pos=None):
		return self._impl.popup(Vector.ensure(pos))
	
# properties
	
	def get_type(self):
		return self._impl.get_type()
	
	def set_type(self, type):
		if (type < MenuItem.TYPE_NORMAL) or (type > MenuItem.TYPE_APPLICATION):
			type = MenuItem.TYPE_NORMAL
		self._impl.set_type(type)
	
	def get_text(self):
		return self._impl.get_text()
	
	def set_text(self, text):
		self._impl.set_text(text)
		
	def get_description(self):
		return self._impl.get_description()
	
	def set_description(self, description):
		self._impl.set_description(description)
	
	def is_checked(self):
		return self._impl.is_checked()
	
	def set_checked(self, checked):
		self._impl.set_checked(checked)
	
	def is_enabled(self):
		return self._impl.is_enabled()
	
	def set_enabled(self, enabled):
		self._impl.set_enabled(enabled)
	
	def get_icon(self):
		return self.__icon or self._impl.get_icon()
	
	def set_icon(self, icon):
		self.__icon = Icon.ensure(icon)
		self._impl.set_icon(self.__icon)
	
	def get_bitmap(self):
		icon = self.get_icon()
		if icon is None:
			return None
		return icon.normal
	
	def set_bitmap(self, bitmap):
		self.set_icon(bitmap)
	
	type = DeprecatedDescriptor('type')
	text = DeprecatedDescriptor('text')
	description = DeprecatedDescriptor('description')
	checked = DeprecatedBoolDescriptor('checked')
	enabled = DeprecatedBoolDescriptor('enabled')
	bitmap = DeprecatedDescriptor('bitmap')



