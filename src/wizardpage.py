import slew

from utils import *
from gdi import *


@factory
class WizardPage(slew.Window):
	NAME						= 'wizardpage'
	
	#defs{SL_WIZARDPAGE_
	STYLE_COMMIT				= 0x00010000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"commit":		STYLE_COMMIT,
								})),
		'title':				TranslatedStringProperty(),
		'subtitle':				TranslatedStringProperty(),
		'next_page':			IntProperty(),
		'watermark':			BitmapProperty(),
		'logo':					BitmapProperty(),
		'banner':				BitmapProperty(),
		'background':			BitmapProperty(),
	})
	
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__complete = True
		self.__watermark = None
		self.__logo = None
		self.__banner = None
		self.__background = None
	
# methods
	
	def is_complete(self):
		return self.__complete
	
	def set_complete(self, complete):
		self.__complete = bool(complete)
		self._impl.set_complete(self.__complete)
	
# properties
	
	def get_next_page(self, page):
		return self._impl.get_next_page()
	
	def set_next_page(self, page):
		if isinstance(page, slew.WizardPage):
			page = self.get_parent().get_children().index(page)
		self._impl.set_next_page(page or -1)
	
	def get_title(self):
		return self._impl.get_title()
	
	def set_title(self, title):
		self._impl.set_title(title)

	def get_subtitle(self):
		return self._impl.get_subtitle()
	
	def set_subtitle(self, title):
		self._impl.set_subtitle(title)

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

