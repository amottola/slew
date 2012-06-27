import slew

from utils import *


@factory
class Dialog(slew.Frame):
	NAME						= 'dialog'
	
	#defs{SL_DIALOG_
	MODALITY_AUTO				= 0
	MODALITY_WINDOW				= 1
	MODALITY_APPLICATION		= 2
	#}
	
# methods
	
	def show_modal(self, blocking=True, modality=MODALITY_AUTO):
		return self._impl.show_modal(blocking, modality)
	
	def end_modal(self, value=None):
		self._impl.end_modal(value)
	

