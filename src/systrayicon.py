import slew

from utils import *
from gdi import *


@factory
class SystrayIcon(slew.Widget):
	NAME						= 'systrayicon'
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'tip':					StringProperty(),
		'icon':					IconProperty(),
	})
	
	
	def initialize(self):
		slew.Widget.initialize(self)
		self.__menu = None
		self.__icon = None
	
# methods
	
	def show(self):
		self._impl.set_visible(True)
	
	def hide(self):
		self._impl.set_visible(False)
	
	def popup_message(self, message, title="", icon=slew.ICON_INFORMATION):
		self._impl.popup_message(message, title, icon)
	
# properties
	
	def get_tip(self):
		return self._impl.get_tip()
	
	def set_tip(self, tip):
		self._impl.set_tip(tip)
	
	def get_menu(self):
		return self.__menu
	
	def set_menu(self, menu):
		self.__menu = slew.Menu.ensure(menu)
		self._impl.set_menu(self.__menu)
	
	def get_icon(self):
		return self.__icon
	
	def set_icon(self, icon):
		self.__icon = Icon.ensure(icon)
		self._impl.set_icon(self.__icon)
		

