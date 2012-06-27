import slew

from utils import *
from gdi import *


@factory
class Image(slew.Window):
	NAME						= 'image'
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'bitmap':				BitmapProperty(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__bitmap = None
	
# properties
		
	def get_bitmap(self):
		return self.__bitmap
	
	def set_bitmap(self, bitmap):
		self.__bitmap = Bitmap.ensure(bitmap)
		self._impl.set_bitmap(self.__bitmap)
	
	bitmap = DeprecatedDescriptor('bitmap')



