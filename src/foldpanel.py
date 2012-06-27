import slew

from utils import *


@factory
class FoldPanel(slew.Window):
	NAME						= 'foldpanel'
	
	#defs{SL_FOLDPANEL_
	STYLE_EXPANDABLE			= 0x00010000
	STYLE_ANIMATED				= 0x00020000
	STYLE_FLAT					= 0x00040000
	STYLE_H_RESIZE				= 0x00080000
	STYLE_V_RESIZE				= 0x00100000
	STYLE_RESIZEABLE			= 0x00180000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"expandable":	STYLE_EXPANDABLE,
									"animated":		STYLE_ANIMATED,
									"flat":			STYLE_FLAT,
									"hresize":		STYLE_H_RESIZE,
									"vresize":		STYLE_V_RESIZE,
									"resize":		STYLE_RESIZEABLE,
								})),
		'label':				TranslatedStringProperty(),
		'folded':				BoolProperty(),
		'duration':				IntProperty(),
		'toggle_shortcut':		StringProperty(),
	})
	
# methods
	
	def fold(self, folded):
		self._impl.set_expanded(True, not folded)
	
	def expand(self, animate):
		self._impl.set_expanded(animate, True)
	
	def collapse(self, animate):
		self._impl.set_expanded(animate, False)
	
	def set_expanded(self, expanded=True, animate=False):
		self._impl.set_expanded(animate, expanded)
	
	def is_expanded(self):
		return self._impl.is_expanded()
	
# properties
	
	def get_label(self):
		return self._impl.get_label()
	
	def set_label(self, label):
		self._impl.set_label(label)
	
	def is_folded(self):
		return not self.is_expanded()
	
	def set_folded(self, folded):
		self._impl.set_expanded(False, not folded)
	
	def get_duration(self):
		return self._impl.get_duration()
	
	def set_duration(self, duration):
		self._impl.set_duration(duration)
	
	def get_toggle_shortcut(self):
		return self._impl.get_toggle_shortcut()
	
	def set_toggle_shortcut(self, shortcut):
		self._impl.set_toggle_shortcut(shortcut)
	
	label = DeprecatedDescriptor('label')
	folded = DeprecatedBoolDescriptor('folded')
	duration = DeprecatedDescriptor('duration')



