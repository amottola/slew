import slew

from utils import *
from gdi import *


@factory
class Wizard(slew.Dialog):
	NAME						= 'wizard'
	
	#defs{SL_WIZARD_
	STYLE_CANCEL				= 0x00010000
	#}
	
	
	PROPERTIES = merge(slew.Dialog.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"cancel":		STYLE_CANCEL,
								})),
		'next_enabled':			BoolProperty(),
		'watermark':			BitmapProperty(),
		'logo':					BitmapProperty(),
		'banner':				BitmapProperty(),
		'background':			BitmapProperty(),
	})
	
	
	def initialize(self):
		slew.Dialog.initialize(self)
		self.__watermark = None
		self.__logo = None
		self.__banner = None
		self.__background = None
	
# methods
	
	def get_page(self):
		return self._impl.get_page()
	
	def next(self):
		self._impl.next()
	
	def back(self):
		self._impl.back()
	
	def restart(self):
		self._impl.restart()
	
# properties
	
	def get_start_page(self):
		return self._impl.get_start_page()
	
	def set_start_page(self, page):
		if isinstance(page, slew.WizardPage):
			page = self.get_children().index(page)
		self._impl.set_start_page(page)
	
	def is_back_enabled(self):
		return self._impl.is_back_enabled()
	
	def set_back_enabled(self, enabled):
		self._impl.set_back_enabled(enabled)
	
	def is_next_enabled(self):
		return self._impl.is_next_enabled()
	
	def set_next_enabled(self, enabled):
		self._impl.set_next_enabled(enabled)
	
	def get_watermark(self):
		return self.__watermark
	
	def set_watermark(self, bitmap):
		self.__watermark = Bitmap.ensure(bitmap)
		self._impl.set_watermark(self.__watermark)
	
	def get_logo(self):
		return self.__logo
	
	def set_logo(self, bitmap):
		self.__logo = Bitmap.ensure(bitmap)
		self._impl.set_logo(self.__logo)
	
	def get_banner(self):
		return self.__banner
	
	def set_banner(self, bitmap):
		self.__banner = Bitmap.ensure(bitmap)
		self._impl.set_banner(self.__banner)
	
	def get_background(self):
		return self.__background
	
	def set_background(self, bitmap):
		self.__background = Bitmap.ensure(bitmap)
		self._impl.set_background(self.__background)

