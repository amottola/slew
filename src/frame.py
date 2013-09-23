import slew

from utils import *


@factory
class Frame(slew.Window):
	NAME						= 'frame'
	
	#defs{SL_FRAME_
	STYLE_MINI					= 0x00010000
	STYLE_CAPTION				= 0x00020000
	STYLE_MINIMIZE_BOX			= 0x00040000
	STYLE_MAXIMIZE_BOX			= 0x00080000
	STYLE_CLOSE_BOX				= 0x00100000
	STYLE_STAY_ON_TOP			= 0x00200000
	STYLE_RESIZEABLE			= 0x00400000
	STYLE_MODIFIED				= 0x00800000
	STYLE_SHEET					= 0x01000000
	STYLE_MAXIMIZED				= 0x02000000
	STYLE_MINIMIZED				= 0x04000000
	STYLE_MODAL					= 0x08000000

	STYLE_DEFAULT				= 0x005E0000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"mini":			STYLE_MINI,
									"caption":		STYLE_CAPTION,
									"maximize":		STYLE_MAXIMIZE_BOX,
									"minimize":		STYLE_MINIMIZE_BOX,
									"close":		STYLE_CLOSE_BOX,
									"stayontop":	STYLE_STAY_ON_TOP,
									"resize":		STYLE_RESIZEABLE,
									"modified":		STYLE_MODIFIED,
									"sheet":		STYLE_SHEET,
									"modal":		STYLE_MODAL,
									"default":		STYLE_DEFAULT
								})),
		'title':				TranslatedStringProperty(),
		'framesize':			VectorProperty(),
	})
	
# methods
	
	def close(self):
		self._impl.close()
	
	def center(self):
		self._impl.center()
	
	def maximize(self):
		self._impl.maximize()
	
	def is_maximized(self):
		return self._impl.is_maximized()
	
	def minimize(self):
		self._impl.minimize()
	
	def is_minimized(self):
		return self._impl.is_minimized()
	
	def is_active(self):
		return self._impl.is_active()
		
	def save_geometry(self):
		return self._impl.save_geometry()
	
	def restore_geometry(self, state):
		self._impl.restore_geometry(state)
	
	def alert(self, timeout=0):
		self._impl.alert(timeout)
	
	@deprecated
	def destroy(self):
		self.detach()
		self.close()
	
# properties
	
	def get_title(self):
		return self._impl.get_title()
	
	def set_title(self, title):
		self._impl.set_title(title)

	def get_framesize(self):
		return self._impl.get_framesize()
	
	def set_framesize(self, size):
		self._impl.set_framesize(Vector.ensure(size))
	
	title = DeprecatedDescriptor('title')

