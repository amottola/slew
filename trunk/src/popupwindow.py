import slew

from utils import *


@factory
class PopupWindow(slew.Frame):
	NAME						= 'popupwindow'
	
#methods
	
	def popup(self, parent, pos, size=None):
		self._impl.popup(parent, Vector.ensure(pos), Vector.ensure(size))