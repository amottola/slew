import slew

from utils import *


@factory
class StatusBar(slew.Window):
	NAME						= 'statusbar'
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'text':					TranslatedStringProperty(),
		'props':				IntListProperty(),
	})
	
# methods
	
	def get_prop(self, index):
		return self._impl.get_prop(index)
	
	def set_prop(self, index, prop):
		self._impl.set_prop(index, prop)
	
# properties
	
	def get_text(self):
		return self._impl.get_text()
	
	def set_text(self, text):
		self._impl.set_text(text)
	
	def get_props(self):
		return self._impl.get_props()
	
	def set_props(self, props):
		if props is None:
			props = ()
		elif not isinstance(props, (list, tuple)):
			props = (str(props) or '').split(' ')
		self._impl.set_props(props)
	
	text = DeprecatedDescriptor('text')
	props = DeprecatedDescriptor('props')



