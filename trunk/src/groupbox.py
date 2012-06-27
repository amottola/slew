import slew

from utils import *


@factory
class GroupBox(slew.Window):
	NAME						= 'groupbox'
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'label':				TranslatedStringProperty(),
	})
	
# properties
	
	def get_label(self):
		return self._impl.get_label()
	
	def set_label(self, label):
		self._impl.set_label(label)
	
	label = DeprecatedDescriptor('label')



