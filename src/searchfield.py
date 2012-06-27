import slew

from utils import *
from gdi import *
from textfield import TextField


@factory
class SearchField(TextField):
	NAME						= 'searchfield'
	
	#defs{SL_SEARCHFIELD_
	STYLE_CANCEL				= 0x00010000
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'cancel':		STYLE_CANCEL,
								})),
	})
	
	
	DEFAULT_ICON				= None
	DEFAULT_EMPTY_TEXT			= None
	
	def initialize(self):
		TextField.initialize(self)
		self.__menu = None
		if self.DEFAULT_ICON is not None:
			self.set_icon(self.DEFAULT_ICON)
		if self.DEFAULT_EMPTY_TEXT is not None:
			self.set_empty_text(self.DEFAULT_EMPTY_TEXT)
	
# methods
	
	def get_menu(self):
		return self.__menu
	
	def set_menu(self, menu):
		self.__menu = slew.Menu.ensure(menu)
		self._impl.set_menu(self.__menu)
	
	def get_empty_text(self):
		return self._impl.get_empty_text()
	
	def set_empty_text(self, text):
		self._impl.set_empty_text(text)
	
	@classmethod
	def set_default_icon(cls, icon):
		cls.DEFAULT_ICON = Icon.ensure(icon)
		
	@classmethod
	def set_default_empty_text(cls, text):
		cls.DEFAULT_EMPTY_TEXT = text
		
	
# properties
	

