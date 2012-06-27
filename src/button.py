import slew

from utils import *
from gdi import *


@factory
class Button(slew.Window):
	NAME						= 'button'
	
	#defs{SL_BUTTON_
	STYLE_TOGGLE				= 0x00010000
	STYLE_BROWSE				= 0x00020000
	STYLE_FLAT					= 0x00040000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"toggle":		STYLE_TOGGLE,
									"browse":		STYLE_BROWSE,
									"flat":			STYLE_FLAT
								})),
		'label':				TranslatedStringProperty(),
		'toggled':				BoolProperty(),
		'default':				BoolProperty(),
		'bitmap':				BitmapProperty(),
		'icon':					IconProperty(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__icon = None
		self.__menu = None
		
# properties
	
	def get_label(self):
		return self._impl.get_label()
	
	def set_label(self, label):
		self._impl.set_label(label)
	
	def is_toggled(self):
		return self._impl.is_toggled()
	
	def set_toggled(self, toggled):
		self._impl.set_toggled(toggled)
	
	def is_default(self):
		return self._impl.is_default()
	
	def set_default(self, default):
		self._impl.set_default(default)
	
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
	
	def get_menu(self):
		return self.__menu or self._impl.get_menu()
	
	def set_menu(self, menu):
		self.__menu = slew.Menu.ensure(menu)
		self._impl.set_menu(self.__menu)
	
	label = DeprecatedDescriptor('label')
	toggled = DeprecatedBoolDescriptor('toggled')
	default = DeprecatedBoolDescriptor('default')
	bitmap = DeprecatedDescriptor('bitmap')



