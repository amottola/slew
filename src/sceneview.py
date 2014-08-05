import slew

from utils import *



class SceneItem(slew.EventHandler):
	ID							= 0
	
	#defs{SL_SCENE_ITEM_
	FLAG_DONT_SCALE				= 0x00000001
	FLAG_SELECTABLE				= 0x00000002
	FLAG_CLICKABLE				= 0x00000004
	FLAG_EDITABLE				= 0x00000008
	FLAG_FOCUSABLE				= 0x00000010
	
	EFFECT_NONE					= 0
	EFFECT_BLUR					= 1
	EFFECT_COLORIZE				= 2
	EFFECT_SHADOW				= 3
	EFFECT_OPACITY				= 4
	#}
	
	def __new__(cls, *args, **kwargs):
		self = slew.EventHandler.__new__(cls)
		self._impl = slew.get_backend().SceneItem(self)
		self.__parent = None
		self.__children = []
		if 'name' not in kwargs:
			self.__name = 'SceneItem_%d' % SceneItem.ID
			SceneItem.ID += 1
		return self
	
	def __init__(self, **kwargs):
		for name in kwargs:
			getattr(self, 'set_' + name)(kwargs[name])
	
	def __len__(self):
		return len(self.__children)
	
	def __contains__(self, item):
		return item in self.__children
	
	def __iter__(self):
		return self.__children.__iter__()
	
	def __getitem__(self, index):
		return self.__children[index]
	
	def __setitem__(self, index, widget):
		self.replace(index, widget)
	
	def __delitem__(self, index):
		self.remove(index)
	
	def get_name(self):
		return self.__name
	
	def set_name(self, name):
		self.__name = str(name)
	
	def get_parent(self):
		return self.__parent
	
	def _set_parent(self, parent):
		self.__parent = parent
	
	def get_children(self):
		return list(self.__children)
	
	def append(self, item):
		self.insert(-1, item)
	
	def insert(self, index, item):
		if item.__parent is self:
			return
		elif item.__parent is not None:
			item.__parent.remove(item)
		count = len(self.__children)
		if index < 0:
			index += count + 1
		if index < 0:
			index = 0
		elif index > count:
			index = count
		self._impl.insert(index, item)
		self.__children.insert(index, item)
		item.__parent = self
		return self
	
	def remove(self, item):
		if isinstance(item, int):
			count = len(self.__children)
			if item < 0:
				item += count + 1
			if (item < 0) or (item >= count):
				return
			elem = self.__children[item]
			self._impl.remove(elem)
			if elem in self.__children:
				self.__children.pop(item)
			elem.__parent = None
		else:
			self._impl.remove(item)
			if item in self.__children:
				self.__children.remove(item)
			item.__parent = None
	
	def replace(self, index, item):
		self.insert(index, item)
	
	def detach(self):
		if self.__parent is not None:
			self.__parent.remove(self)
	
	def clear(self):
		for child in self.get_children():
			self.remove(child)
	
	def get_pos(self):
		return self._impl.get_pos()
	
	def set_pos(self, pos):
		self._impl.set_pos(Vector.ensure(pos, False))
	
	def get_size(self):
		return self._impl.get_size()
	
	def set_size(self, size):
		self._impl.set_size(Vector.ensure(size, False))
	
	def get_origin(self):
		return self._impl.get_origin()
	
	def set_origin(self, pos):
		self._impl.set_origin(Vector.ensure(pos, False))
	
	def is_visible(self):
		return self._impl.is_visible()
	
	def set_visible(self, visible):
		self._impl.set_visible(visible)
	
	def show(self):
		self._impl.set_visible(True)
	
	def hide(self):
		self._impl.set_visible(False)
	
	def focus(self):
		self._impl.set_focus(True)
	
	def unfocus(self):
		self._impl.set_focus(False)
	
	def ensure_visible(self):
		self._impl.ensure_visible()
	
	def grab_mouse(self):
		self._impl.set_grab(True)
	
	def ungrab_mouse(self):
		self._impl.set_grab(False)
	
	def get_zorder(self):
		return self._impl.get_zorder()
	
	def set_zorder(self, z):
		self._impl.set_zorder(z)
	
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
	
	def repaint(self, topleft=None, bottomright=None):
		self._impl.repaint(Vector.ensure(topleft), Vector.ensure(bottomright))
	
	def get_flags(self):
		return self._impl.get_flags()
	
	def set_flags(self, flags):
		self._impl.set_flags(flags)

	def map_to_scene(self, pos):
		return self._impl.map_to_scene(Vector.ensure(pos, False))
	
	def map_from_scene(self, pos):
		return self._impl.map_from_scene(Vector.ensure(pos, False))
	
	def get_scene_rect(self):
		return self._impl.get_scene_rect()
	
	def set_effect(self, effect, **kwargs):
		self._impl.set_effect(int(effect or SceneItem.EFFECT_NONE), kwargs)
	
	def is_selected(self):
		return self._impl.is_selected()
	
	def set_selected(self, selected):
		self._impl.set_selected(selected)
	
	def get_font(self):
		return self._impl.get_font()
	
	def set_font(self, font):
		self._impl.set_font(Font.ensure(font))
	
	def get_text(self):
		return self._impl.get_text()
	
	def set_text(self, text):
		self._impl.set_text(text)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	def get_color(self):
		return self._impl.get_color()
	
	def set_color(self, color):
		self._impl.set_color(Color.ensure(color))

	def get_padding(self):
		return self._impl.get_padding()
	
	def set_padding(self, padding):
		if padding is None:
			padding = ()
		elif not isinstance(padding, (list, tuple)):
			padding = (str(padding) or '').replace('px', '').split(' ')
		self._impl.set_padding(padding)
	
	def get_text_transform(self):
		return Transform(self._impl.get_text_transform())
	
	def set_text_transform(self, matrix):
		if isinstance(matrix, Transform):
			matrix = matrix.as_tuple()
		self._impl.set_text_transform(matrix)
	
	def translate_text(self, dx, dy):
		t = self.get_text_transform()
		t.translate(dx, dy)
		self.set_text_transform(t)
	
	def scale_text(self, dx, dy=None):
		if dy is None:
			dy = dx
		t = self.get_text_transform()
		t.scale(dx, dy)
		self.set_text_transform(t)
		
	def rotate_text(self, angle):
		t = self.get_text_transform()
		t.rotate(angle)
		self.set_text_transform(t)



@factory
class SceneView(slew.Window):
	NAME						= 'sceneview'
	
	#defs{SL_SCENEVIEW_
	STYLE_OPENGL				= 0x00010000
	STYLE_SCROLLBARS			= 0x00020000
	STYLE_DRAG_SCROLL			= 0x00040000
	STYLE_DRAG_SELECT			= 0x00080000
	STYLE_NO_ANTIALIAS			= 0x00100000
	
	ANCHOR_NONE					= 0
	ANCHOR_CENTER				= 1
	ANCHOR_MOUSE				= 2
	#}
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									'opengl':		STYLE_OPENGL,
									'scrollbars':	STYLE_SCROLLBARS,
									'dragscroll':	STYLE_DRAG_SCROLL,
									'dragselect':	STYLE_DRAG_SELECT,
									'noaa':			STYLE_NO_ANTIALIAS,
								})),
		'anchor':				IntProperty(),
		'align':				BitsProperty({	'left':		slew.ALIGN_LEFT,
												'hcenter':	slew.ALIGN_HCENTER,
												'right':	slew.ALIGN_RIGHT,
												'top':		slew.ALIGN_TOP,
												'vcenter':	slew.ALIGN_VCENTER,
												'bottom':	slew.ALIGN_BOTTOM,
												'center':	slew.ALIGN_CENTER,
								}),
		'scale':				FloatProperty(),
	})
	
	
	def __str__(self):
		def visit(s, item, level):
			s.append('%s%s' % ('  ' * level, str(type(item))))
			for child in item:
				visit(s, child, level + 1)
		output = []
		visit(output, self, 0)
		return u'\n'.join(output)
	
# methods
	
	def insert(self, index, item):
		if item.get_parent() is self:
			return
		return slew.Window.insert(self, index, item)
	
	def clear(self):
		children = self.get_children()
		while len(children) > 0:
			self.remove(children.pop())
	
	def map_to_scene(self, pos):
		return self._impl.map_to_scene(Vector.ensure(pos, False))
	
	def map_from_scene(self, pos):
		return self._impl.map_from_scene(Vector.ensure(pos, False))
	
	def ensure_visible(self, tl, br=None, margin=None):
		if br is None:
			br = tl
		if isinstance(tl, SceneItem):
			tl, br = tl.get_scene_rect()
		if margin is None:
			margin = Vector(50, 50)
		self._impl.ensure_visible(Vector.ensure(tl, False), Vector.ensure(br, False), Vector.ensure(margin, False))
	
	def center_on(self, pos):
		self._impl.center_on(Vector.ensure(pos, False))
	
	def fit_in_view(self, tl, br):
		self._impl.fit_in_view(Vector.ensure(tl, False), Vector.ensure(br, False))
	
	def get_items_bounding_rect(self):
		return self._impl.get_items_bounding_rect()
	
	def get_scene_rect(self):
		return self._impl.get_scene_rect()
	
	def set_scene_rect(self, tl, br=None):
		self._impl.set_scene_rect(Vector.ensure(tl, True), Vector.ensure(br, True))
	
	def get_item_at(self, pos):
		items = self.get_items_at(pos)
		if len(items) > 0:
			return items[0]
		return None
	
	def get_items_at(self, pos):
		return self._impl.get_items_at(Vector.ensure(pos, False))
	
	def get_viewport_size(self):
		return self._impl.get_viewport_size()
	
	def get_selection(self):
		return self._impl.get_selection()
	
	def set_selection_rect(self, tl, br=None):
		self._impl.set_selection_rect(Vector.ensure(tl, True), Vector.ensure(br, True))
	
	def clear_selection(self):
		self._impl.clear_selection()
	
	def get_scroll_pos(self):
		return self._impl.get_scroll_pos()
	
	def set_scroll_pos(self, pos):
		self._impl.set_scroll_pos(Vector.ensure(pos, False))
	
	def get_scroll_rate(self):
		return self._impl.get_scroll_rate()
	
	def set_scroll_rate(self, pos):
		self._impl.set_scroll_rate(Vector.ensure(pos, False))
	
# properties
	
	def get_anchor(self):
		return self._impl.get_anchor()
	
	def set_anchor(self, anchor):
		self._impl.set_anchor(anchor)
	
	def get_align(self):
		return self._impl.get_align()
	
	def set_align(self, align):
		self._impl.set_align(align)
	
	def get_scale(self):
		return self._impl.get_scale()
	
	def set_scale(self, scale):
		self._impl.set_scale(scale)
	

