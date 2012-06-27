import slew

from utils import *
from panel import Panel



class Spacer(Panel):
	def create_impl(self, class_name):
		return Panel.create_impl(self, 'Panel')



class SizerItem_Impl(object):
	def __init__(self, si):
		self.si = si
		self.flags = 0
		self.align = slew.ALIGN_CENTER
		self.border = SizerItem.BORDER_ALL
		self.bordersize = 0
		self.space = Vector(20,20)
		self.cell = Vector(0,0)
		self.span = Vector(1,1)
		self.child = Spacer()
	
	def set_handler(self, handler):
		pass
	
	def insert(self, index, child):
		if index != 0:
			raise ValueError("cannot attach more than one widget to sizeritem!")
		parent = self.si.get_parent()
		if parent is not None:
			if isinstance(self.child, Spacer):
				parent._impl.remove(self.child)
			self.child = child
			parent._impl.insert(index, self.setup_child())
		else:
			self.child = child
	
	def remove(self, child):
		parent = self.si.get_parent()
		if parent is not None:
			parent._impl.remove(child)
		self.child = Spacer()
		if parent is not None:
			parent._impl.insert(0, self.setup_child())
	
	def setup_child(self):
		self.set_border(self.border, self.bordersize)
		self.set_flags_align(self.flags, self.align)
		self.set_space(self.space)
		self.set_cell(self.cell)
		self.set_span(self.span)
		return self.child
	
	def set_flags_align(self, flags, align):
		flags = int(flags)
		align = int(align)
		self.flags = flags
		self.align = align
		if flags & SizerItem.EXPAND:
			self.child.set_boxalign(slew.Window.BOXALIGN_EXPAND)
		else:
			self.child.set_boxalign(align)
	
	def set_border(self, border, bordersize):
		border = int(border)
		bordersize = int(bordersize)
		self.border = border
		self.bordersize = bordersize
		margins = [ 0, 0, 0, 0 ]
		if bordersize > 0:
			if border & SizerItem.BORDER_TOP:
				margins[0] = bordersize
			if border & SizerItem.BORDER_RIGHT:
				margins[1] = bordersize
			if border & SizerItem.BORDER_BOTTOM:
				margins[2] = bordersize
			if border & SizerItem.BORDER_LEFT:
				margins[3] = bordersize
		self.child.set_margins(margins)
	
	def set_space(self, space):
		self.space = space
		if isinstance(self.child, Spacer):
			self.child.set_minsize(space)
			self.child.set_size(space)
	
	def set_cell(self, cell):
		self.cell = cell
		if self.child.get_cell() != cell:
			self.child.set_cell(cell)
	
	def get_cell(self):
		return self.child.get_cell()
	
	def set_span(self, span):
		self.span = span
		if self.child.get_span() != span:
			self.child.set_span(span)
	
	def get_span(self):
		return self.child.get_span()



@factory
class SizerItem(slew.Widget):
	NAME						= 'sizeritem'
	
	#defs{SL_SIZER_ITEM_
	EXPAND						= 0x1
	KEEP_ASPECT_RATIO			= 0x2
	
	BORDER_TOP					= 0x1
	BORDER_BOTTOM				= 0x2
	BORDER_LEFT					= 0x4
	BORDER_RIGHT				= 0x8
	BORDER_ALL					= 0xF
	#}
	
	
	PROPERTIES = merge(slew.Widget.PROPERTIES, {
		'flags':				BitsProperty(['expand', 'keepratio']),
		'align':				BitsProperty({	'top':		slew.ALIGN_TOP,
												'bottom':	slew.ALIGN_BOTTOM,
												'left':		slew.ALIGN_LEFT,
												'right':	slew.ALIGN_RIGHT,
												'hcenter':	slew.ALIGN_HCENTER,
												'vcenter':	slew.ALIGN_VCENTER,
												'center':	slew.ALIGN_CENTER }),
		'border':				BitsProperty(['top','bottom','left','right']),
		'bordersize':			IntProperty(),
		'space':				VectorProperty(),
		'cell':					VectorProperty(),
		'span':					VectorProperty(),
		'pos':					VectorProperty(),
		'size':					VectorProperty(),
	})
	
	def create_impl(self, class_name):
		return SizerItem_Impl(self)
	
# methods
	
	@deprecated
	def map_to_global(self, pos):
		pass
	
	@deprecated
	def map_to_local(self, pos):
		pass
	
# properties
	
	def get_flags(self):
		return self._impl.flags
	
	def set_flags(self, flags):
		self._impl.set_flags_align(flags, self._impl.align)
	
	def get_align(self):
		return self._impl.align
	
	def set_align(self, align):
		self._impl.set_flags_align(self._impl.flags, align)
	
	def get_border(self):
		return self._impl.border
	
	def set_border(self, border):
		self._impl.set_border(border, self._impl.bordersize)
	
	def get_bordersize(self):
		return self._impl.bordersize
	
	def set_bordersize(self, bordersize):
		self._impl.set_border(self._impl.border, bordersize)
	
	def get_space(self):
		return self._impl.space
	
	def set_space(self, space):
		self._impl.set_space(Vector.ensure(space, False))
	
	def get_cell(self):
		return self._impl.get_cell()
	
	def set_cell(self, cell):
		self._impl.set_cell(Vector.ensure(cell, False))
	
	def get_span(self):
		return self._impl.get_span()
	
	def set_span(self, span):
		self._impl.set_span(Vector.ensure(span, False))
	
	def get_pos(self):
		pass
	
	def set_pos(self, pos):
		pass
	
	def get_size(self):
		pass
	
	def set_size(self, size):
		pass
	
	flags = DeprecatedDescriptor('flags')
	align = DeprecatedDescriptor('align')
	border = DeprecatedDescriptor('border')
	bordersize = DeprecatedDescriptor('bordersize')
	space = DeprecatedDescriptor('space')
	cell = DeprecatedDescriptor('cell')
	span = DeprecatedDescriptor('span')
	pos = DeprecatedDescriptor('pos')
	size = DeprecatedDescriptor('size')
	


