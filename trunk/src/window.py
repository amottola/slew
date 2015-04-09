import slew

from utils import *


class Layoutable(slew.Widget):
	#defs{SL_LAYOUTABLE_
	BOXALIGN_EXPAND				= 0xFFFFFFFF
	#}
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'margins':				StringProperty(),
		'boxalign':				BitsProperty({	'left':		slew.ALIGN_LEFT,
												'hcenter':	slew.ALIGN_HCENTER,
												'right':	slew.ALIGN_RIGHT,
												'top':		slew.ALIGN_TOP,
												'vcenter':	slew.ALIGN_VCENTER,
												'bottom':	slew.ALIGN_BOTTOM,
												'center':	slew.ALIGN_CENTER,
												'expand':	BOXALIGN_EXPAND }),
		'cell':					VectorProperty(),
		'span':					VectorProperty(),
		'prop':					IntProperty(),
	})

	def replace(self, index, child):
		count = len(self._Widget__children)
		if isinstance(index, basestring):
			for i, widget in enumerate(self._Widget__children):
				if widget._Widget__name == index:
					child._Widget__name = index
					index = i
					break
			else:
				index = -1
		old_child = None
		if index < 0:
			index += count + 1
		if index < 0:
			index = 0
		elif index > count:
			index = count
		elif index < count:
			old_child = self._Widget__children[index]
			self.remove(index)
		self.insert(index, child)
		if (old_child is not None) and isinstance(old_child, Layoutable) and isinstance(child, Layoutable):
			cell = old_child.get_cell()
			if cell != child.get_cell():
				child.set_cell(cell)
			span = old_child.get_span()
			if span != child.get_span():
				child.set_span(span)
			child.set_margins(old_child.get_margins())
			child.set_boxalign(old_child.get_boxalign())
			child.set_prop(old_child.get_prop())
	
	def get_margins(self):
		return self._impl.get_margins()
	
	def set_margins(self, margins):
		if margins is None:
			margins = ()
		elif not isinstance(margins, (list, tuple)):
			margins = (str(margins) or '').replace('px', '').split(' ')
		self._impl.set_margins(margins)
	
	def get_boxalign(self):
		return self._impl.get_boxalign()
	
	def set_boxalign(self, align):
		self._impl.set_boxalign(int(align))
	
	def get_cell(self):
		return self._impl.get_cell()
	
	def set_cell(self, cell):
		self._impl.set_cell(Vector.ensure(cell, False))
	
	def get_span(self):
		return self._impl.get_span()
	
	def set_span(self, span):
		self._impl.set_span(Vector.ensure(span, False))
	
	def get_prop(self):
		return self._impl.get_prop()
	
	def set_prop(self, prop):
		self._impl.set_prop(int(prop))



class Window(Layoutable):
	NAME						= 'window'
	
	#defs{SL_WINDOW_
	STYLE_NORMAL				= 0x00000000
	STYLE_THIN					= 0x00000001
	STYLE_THICK					= 0x00000002
	STYLE_SUNKEN				= 0x00000004
	STYLE_RAISED				= 0x00000008
	STYLE_FRAMELESS				= 0x00000010
	STYLE_BGIMAGE_TILED			= 0x00000020
	STYLE_BGIMAGE_STRETCHED		= 0x00000040
	STYLE_SMALL					= 0x00000080
	STYLE_NOFOCUS				= 0x00000100
	STYLE_TRANSLUCENT			= 0x00000200
	STYLE_NOMOUSE				= 0x00000400
	STYLE_DATA					= 0x00008000
	
	BOXALIGN_EXPAND				= 0xFFFFFFFF
	#}
	BOXALIGN_LEFT				= slew.ALIGN_LEFT
	BOXALIGN_HCENTER			= slew.ALIGN_HCENTER
	BOXALIGN_RIGHT				= slew.ALIGN_RIGHT
	BOXALIGN_TOP				= slew.ALIGN_TOP
	BOXALIGN_VCENTER			= slew.ALIGN_VCENTER
	BOXALIGN_BOTTOM				= slew.ALIGN_BOTTOM
	
	BOXALIGN_HMASK				= slew.ALIGN_HMASK
	BOXALIGN_VMASK				= slew.ALIGN_VMASK
	BOXALIGN_CENTER				= slew.ALIGN_CENTER
	
	STYLE_BITS = {
		"normal":				STYLE_NORMAL,
		"thin":					STYLE_THIN,
		"thick":				STYLE_THICK,
		"sunken":				STYLE_SUNKEN,
		"raised":				STYLE_RAISED,
		"frameless":			STYLE_FRAMELESS,
		"bgimage_tiled":		STYLE_BGIMAGE_TILED,
		"bgimage_stretched":	STYLE_BGIMAGE_STRETCHED,
		"small":				STYLE_SMALL,
		"nofocus":				STYLE_NOFOCUS,
		"translucent":			STYLE_TRANSLUCENT,
		"nomouse":				STYLE_NOMOUSE,
		"data":					STYLE_DATA,
	}
	
	PROPERTIES = merge(Layoutable.PROPERTIES, {
		'datatype':				ChoiceProperty(("unknown", "bool", "integer", "decimal", "float", "date", "time", "timestamp", "year", "string")),
		'style':				BitsProperty(STYLE_BITS),
		'pos':					VectorProperty(),
		'size':					VectorProperty(),
		'minsize':				VectorProperty(),
		'maxsize':				VectorProperty(),
		'fixedsize':			VectorProperty(),
		'visible':				BoolProperty(),
		'tip':					TranslatedStringProperty(),
		'enabled':				BoolProperty(),
		'cursor':				ChoiceProperty(("normal", "ibeam", "hand", "wait", "cross", "none", "move", "resize")),
		'color':				ColorProperty(),
		'bgcolor':				ColorProperty(),
		'hicolor':				ColorProperty(),
		'hibgcolor':			ColorProperty(),
		'font':					FontProperty(),
	})
	
# methods
	
	def set_updates_enabled(self, enabled):
		self._impl.set_updates_enabled(enabled)
	
	def show(self):
		self._impl.set_visible(True)
	
	def hide(self):
		self._impl.set_visible(False)
	
	def focus(self):
		self._impl.set_focus(True)
	
	def unfocus(self):
		self._impl.set_focus(False)
	
	def has_focus(self):
		return self._impl.has_focus()
	
	def render(self):
		return self._impl.render()
	
	def repaint(self, topleft=None, bottomright=None):
		self._impl.repaint(Vector.ensure(topleft), Vector.ensure(bottomright))
	
	def message_box(self, message, title="", buttons=slew.BUTTON_OK, icon=slew.ICON_INFORMATION, modality=0, callback=None, userdata=None):
		return self._impl.message_box(message, title, buttons, icon, modality, callback, userdata)
	
	def map_to_local(self, coord):
		return self._impl.map_to_local(Vector.ensure(coord))
	
	def map_to_global(self, coord):
		return self._impl.map_to_global(Vector.ensure(coord))
	
	def grab_mouse(self):
		self._impl.grab_mouse(True)
	
	def ungrab_mouse(self):
		self._impl.grab_mouse(False)
	
	@deprecated
	def post_notification(self, data):
		handler = self.get_handler()
		if handler is None:
			handler = self
		slew.call_later(handler.onNotify, slew.Event(widget=self, time=datetime.now(), data=data))
	
	def set_opacity(self, opacity):
		self._impl.set_opacity(float(opacity))
	
	def popup_message(self, text, align=slew.BOTTOM, buttons=0):
		return self._impl.popup_message(text, align, buttons)
	
	def find_focus(self):
		return self._impl.find_focus()
	
	def set_shortcut(self, key, action):
		self._impl.set_shortcut(key, action)
	
	def set_timeout(self, msecs, *args):
		self._impl.set_timeout(msecs, *args)
	
	def fit(self):
		self._impl.fit()
		return self.get_size()
	
	def has_style(self, style):
		return bool(self._impl.get_style() & style)
	
	def add_style(self, style):
		self._impl.set_style(self._impl.get_style() | style)
	
	def remove_style(self, style):
		self._impl.set_style(self._impl.get_style() & ~style)
	
	def toggle_style(self, style):
		self._impl.set_style(self._impl.get_style() ^ style)
	
	def alter_style(self, add, remove):
		self._impl.set_style((self._impl.get_style() | add) & ~remove)
	
	def enable_style(self, style, enable):
		if enable:
			self._impl.set_style(self._impl.get_style() | style)
		else:
			self._impl.set_style(self._impl.get_style() & ~style)
	
# properties
	
	def get_datatype(self):
		return self._impl.get_datatype()
	
	def set_datatype(self, datatype):
		self._impl.set_datatype(datatype)
	
	def get_style(self):
		return self._impl.get_style()
	
	def set_style(self, style):
		self._impl.set_style(style)
	
	def get_pos(self):
		return self._impl.get_pos()
	
	def set_pos(self, pos):
		self._impl.set_pos(Vector.ensure(pos))
	
	def get_size(self):
		return self._impl.get_size()
	
	def set_size(self, size):
		self._impl.set_size(Vector.ensure(size))
	
	def get_minsize(self):
		return self._impl.get_minsize()
	
	def set_minsize(self, minsize):
		self._impl.set_minsize(Vector.ensure(minsize))
	
	def get_maxsize(self):
		return self._impl.get_maxsize()
	
	def set_maxsize(self, maxsize):
		self._impl.set_maxsize(Vector.ensure(maxsize))
	
	def get_fixedsize(self):
		return self._impl.get_minsize()
	
	def set_fixedsize(self, size):
		size = Vector.ensure(size)
		self._impl.set_minsize(size)
		self._impl.set_maxsize(size)
	
	def set_visible(self, visible):
		self._impl.set_visible(visible)
	
	def is_visible(self):
		return self._impl.is_visible()
	
	def get_tip(self):
		return self._impl.get_tip()
	
	def set_tip(self, tip):
		self._impl.set_tip(tip)
	
	def is_enabled(self):
		return self._impl.is_enabled()
	
	def set_enabled(self, enabled):
		self._impl.set_enabled(enabled)
	
	def get_cursor(self):
		return self._impl.get_cursor()
	
	def set_cursor(self, cursor):
		self._impl.set_cursor(cursor)
	
	def get_color(self):
		return self._impl.get_color()
	
	def get_system_color(self):
		return self._impl.get_system_color()

	def set_color(self, color):
		self._impl.set_color(Color.ensure(color))
	
	def get_bgcolor(self):
		return self._impl.get_bgcolor()
	
	def get_system_bgcolor(self):
		return self._impl.get_system_bgcolor()
	
	def set_bgcolor(self, color):
		self._impl.set_bgcolor(Color.ensure(color))
	
	def get_hicolor(self):
		return self._impl.get_hicolor()
	
	def get_system_hicolor(self):
		return self._impl.get_system_hicolor()
	
	def set_hicolor(self, color):
		self._impl.set_hicolor(Color.ensure(color))
	
	def get_hibgcolor(self):
		return self._impl.get_hibgcolor()
	
	def get_system_hibgcolor(self):
		return self._impl.get_system_hibgcolor()
	
	def set_hibgcolor(self, color):
		self._impl.set_hibgcolor(Color.ensure(color))
	
	def set_font(self, font):
		self._impl.set_font(Font.ensure(font))

	def get_font(self):
		return self._impl.get_font()
	
	def get_system_font(self):
		return self._impl.get_system_font()
	
	datatype = DeprecatedDescriptor('datatype')
	style = DeprecatedDescriptor('style')
	pos = DeprecatedDescriptor('pos')
	size = DeprecatedDescriptor('size')
	minsize = DeprecatedDescriptor('minsize')
	maxsize = DeprecatedDescriptor('maxsize')
	visible = DeprecatedBoolDescriptor('visible')
	tip = DeprecatedDescriptor('tip')
	enabled = DeprecatedBoolDescriptor('enabled')
	cursor = DeprecatedDescriptor('cursor')
	color = DeprecatedDescriptor('color')
	bgcolor = DeprecatedDescriptor('bgcolor')
	hicolor = DeprecatedDescriptor('hicolor')
	hibgcolor = DeprecatedDescriptor('hibgcolor')
	font = DeprecatedDescriptor('font')

