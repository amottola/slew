import slew

from utils import *
from ranged import Ranged


@factory
class Progress(Ranged):
	NAME						= 'progress'
	
	#defs{SL_PROGRESS_
	STYLE_INDETERMINATE			= 0x00020000
	#}
	
	PROPERTIES = merge(Ranged.PROPERTIES, {
		'style':				BitsProperty(merge(Ranged.STYLE_BITS, {
									'indeterminate':	STYLE_INDETERMINATE
								})),
	})
	
# methods
	
	@deprecated
	def pulse(self):
		self.set_style(Progress.STYLE_INDETERMINATE)

