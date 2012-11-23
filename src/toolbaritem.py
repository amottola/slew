import slew

from utils import *


@factory
class ToolBarItem(slew.Widget):
	NAME						= 'toolbaritem'
	
	#defs{SL_TOOLBAR_ITEM_
	TYPE_NORMAL					= 0
	TYPE_SEPARATOR				= 1
	TYPE_CHECK					= 2
	TYPE_RADIO					= 3
	TYPE_URL					= 4
	TYPE_AUTOREPEAT				= 5
	
	MENU_DELAYED				= 0
	MENU_BUTTON					= 1
	MENU_ARROW					= 2
	#}
	
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'type':					ChoiceProperty(["normal", "separator", "check", "radio", "url", "autorepeat"]),
		'text':					TranslatedStringProperty(),
		'description':			TranslatedStringProperty(),
		'checked':				BoolProperty(),
		'enabled':				BoolProperty(),
		'flexible':				BoolProperty(),
		'bitmap':				BitmapProperty(),
		'icon':					IconProperty(),
		'url':					StringProperty(),
	})
	
	def initialize(self):
		slew.Widget.initialize(self)
		self.__icon = None
		self.__menu = None
		self.__url = ''
	
	def onSelect(self, e):
		if (self.get_type() == ToolBarItem.TYPE_URL) and self.__url:
			slew.system_open(self.__url)
	
# methods
	
	def set_autorepeat(self, delay, interval):
		self._impl.set_autorepeat(delay, interval)
	
# properties
	
	def get_type(self):
		return self._impl.get_type()
	
	def set_type(self, type):
		if (type < ToolBarItem.TYPE_NORMAL) or (type > ToolBarItem.TYPE_AUTOREPEAT):
			type = ToolBarItem.TYPE_NORMAL
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
	
	def is_flexible(self):
		return self._impl.is_flexible()
	
	def set_flexible(self, flexible):
		self._impl.set_flexible(flexible)
	
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
	
	def get_url(self):
		return self.__url
	
	def set_url(self, url):
		self.__url = url
	
	def get_menu(self):
		return self.__menu or self._impl.get_menu()
	
	def set_menu(self, menu, mode=MENU_BUTTON):
		self.__menu = slew.Menu.ensure(menu)
		self._impl.set_menu(self.__menu, mode)
	
	type = DeprecatedDescriptor('type')
	text = DeprecatedDescriptor('text')
	description = DeprecatedDescriptor('description')
	checked = DeprecatedBoolDescriptor('checked')
	enabled = DeprecatedBoolDescriptor('enabled')
	flexible = DeprecatedBoolDescriptor('flexible')
	bitmap = DeprecatedDescriptor('bitmap')
	url = DeprecatedDescriptor('url')
	menu = DeprecatedDescriptor('menu')

