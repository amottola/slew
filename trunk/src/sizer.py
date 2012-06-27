import slew

from utils import *
from window import Window, Layoutable
from sizeritem import SizerItem


ALIGN = {
	'left':			slew.ALIGN_LEFT,
	'center':		slew.ALIGN_HCENTER,
	'right':		slew.ALIGN_RIGHT,
}

VALIGN = {
	'top':			slew.ALIGN_TOP,
	'middle':		slew.ALIGN_VCENTER,
	'bottom':		slew.ALIGN_BOTTOM,
}


@factory
class Sizer(Layoutable):
	NAME						= 'sizer'
	
	PROPERTIES = merge(Layoutable.PROPERTIES, {
		'size':					VectorProperty(),
		'spacing':				VectorProperty(),
		'horizontal_spacing':	IntProperty(),
		'vertical_spacing':		IntProperty(),
		'colsprop':				IntListProperty(),
		'rowsprop':				IntListProperty(),
		'orientation':			ChoiceProperty(['horizontal', 'vertical']),
	})
	
	def create_impl(self, class_name):
		return Layoutable.create_impl(self, 'Sizer')
	
	def clone(self, name=None):
		cloned = get_factory(self.NAME)()
		for pname, prop in self.PROPERTIES.iteritems():
			prop.copy(self, cloned, pname)
		size = self.get_size()
		for column in xrange(0, int(size.x)):
			prop = self.get_column_prop(column)
			if prop == 0:
				cloned.set_column_width(column, self.get_column_width(column))
		for row in xrange(0, int(size.y)):
			prop = self.get_row_prop(row)
			if prop == 0:
				cloned.set_row_height(row, self.get_row_height(row))
		for child in self._Widget__children:
			cloned.append(child.clone())
		if name is not None:
			cloned.set_name(name)
		return cloned
	
	def load(self, xml='', globals=None, locals=None, node=None):
		if node is None:
			node = parse_xml(xml)
		
		if node.tag != self.NAME:
			raise ValueError("tag mismatch, expecting '%s'" % self.NAME)
		
		self._load_properties(node, globals, locals)
		
		row = 0
		rows_prop = 0
		rows_left = []
		col_align = {}
		col_margins = {}
		for child in node.getchildren():
			if child.tag == 'columns':
				index = 0
				cols_prop = 0
				cols_left = []
				for column in child.getchildren():
					if column.tag == 'column':
						size = column.attrib.get('width', None)
						if size is None:
							cols_left.append(index)
						else:
							size = size.strip()
							if size.endswith('%'):
								prop = int(size[:-1])
								cols_prop += prop
								self.set_column_prop(index, prop)
							else:
								self.set_column_width(index, int(size))
						align = column.attrib.get('align', None)
						if align is not None:
							col_align[index] = parse_bit_flags(align, ALIGN)
						col_margins[index] = column.attrib.get('margins', None)
						index += 1
				if cols_left:
					if cols_prop >= 99:
						prop = 1
					else:
						prop = max(1, int((100 - cols_prop) / len(cols_left)))
					for column in cols_left:
						self.set_column_prop(column, prop)
			elif child.tag == 'row':
				column = 0
				size = child.attrib.get('height', None)
				valign = child.attrib.get('valign', None)
				margins = child.attrib.get('margins', None)
				if valign is not None:
					valign = parse_bit_flags(valign, VALIGN)
				if size is None:
					rows_left.append(row)
				else:
					size = size.strip()
					if size.endswith('%'):
						prop = int(size[:-1])
						rows_prop += prop
						self.set_row_prop(row, prop)
					else:
						self.set_row_height(row, int(size))
				for item in child.getchildren():
					if item.tag != 'empty':
						widget = get_factory(item.tag)()
						widget.load(None, globals, locals, item)
						widget.set_cell(Vector(column, row))
						if 'boxalign' not in item.attrib:
							align = col_align.get(column, None)
							if align is None:
								align = valign
							else:
								align |= (valign or 0)
							if align is not None:
								widget.set_boxalign(align)
						if 'margins' not in item.attrib:
							m = margins
							if m is None:
								m = col_margins.get(column, None)
							if m is not None:
								widget.set_margins(m)
						self.append(widget)
					column += 1
				row += 1
			else:
				widget = get_factory(child.tag)()
				widget.load(None, globals, locals, child)
				self.append(widget)
		if rows_left:
			if rows_prop >= 99:
				prop = 1
			else:
				prop = max(1, int((100 - rows_prop) / len(rows_left)))
			for row in rows_left:
				self.set_row_prop(row, prop)
	
	def insert(self, index, child):
		if child._Widget__parent is not None:
			child._Widget__parent.remove(child)
		count = len(self._Widget__children)
		if index < 0:
			index += count + 1
		if index < 0:
			index = 0
		elif index > count:
			index = count
		if isinstance(child, SizerItem):
			self._impl.insert(index, child._impl.setup_child())
		else:
			self._impl.insert(index, child)
		self._Widget__children.insert(index, child)
		child._Widget__parent = self
		return self
	
	def remove(self, child):
		if isinstance(child, int):
			if child < 0:
				child += len(self._Widget__children)
			child = self._Widget__children[child]
		if isinstance(child, SizerItem):
			self._impl.remove(child._impl.child)
		else:
			self._impl.remove(child)
		self._Widget__children.remove(child)
		child._Widget__parent = None
	
# methods
	
	def get_column_width(self, column):
		return self._impl.get_column_width(column)
	
	def set_column_width(self, column, width):
		self._impl.set_column_width(column, width)
	
	def get_column_prop(self, column):
		return self._impl.get_column_prop(column)
	
	def set_column_prop(self, column, prop):
		self._impl.set_column_prop(column, prop)
	
	def get_row_height(self, row):
		return self._impl.get_row_height(row)
	
	def set_row_height(self, row, height):
		self._impl.set_row_height(row, height)
	
	def get_row_prop(self, row):
		return self._impl.get_row_prop(row)
	
	def set_row_prop(self, row, prop):
		self._impl.set_row_prop(row, prop)
	
	def fit(self):
		parent = self.get_parent()
		while (parent is not None) and (not isinstance(parent, Window)):
			parent = parent.get_parent()
		if parent is not None:
			return parent.fit()
	
	def get_minsize(self):
		return self._impl.get_minsize()
	
# properties
	
	def get_size(self):
		return self._impl.get_size()
	
	def set_size(self, size):
		self._impl.set_size(Vector.ensure(size, False))
	
	def get_spacing(self):
		return Vector(self.get_horizontal_spacing(), self.get_vertical_spacing())
	
	def set_spacing(self, spacing):
		spacing = Vector.ensure(spacing, False)
		self.set_horizontal_spacing(int(spacing.x))
		self.set_vertical_spacing(int(spacing.x))
	
	def get_horizontal_spacing(self):
		return self._impl.get_horizontal_spacing()
	
	def set_horizontal_spacing(self, spacing):
		self._impl.set_horizontal_spacing(spacing)
	
	def get_vertical_spacing(self):
		return self._impl.get_vertical_spacing()
	
	def set_vertical_spacing(self, spacing):
		self._impl.set_vertical_spacing(spacing)
	
	def get_orientation(self):
		return self._impl.get_orientation()
	
	def set_orientation(self, orientation):
		self._impl.set_orientation(orientation)
	
	def get_colsprop(self):
		props = []
		for column in xrange(0, int(self.get_size().x)):
			props.append(self.get_column_prop(column))
		return props
	
	def set_colsprop(self, props):
		count = len(props)
		size = self.get_size()
		if size.x < count:
			self.set_size(Vector(count, size.y))
		for column in xrange(0, count):
			self.set_column_prop(column, int(props[column]))
	
	def get_rowsprop(self):
		props = []
		for row in xrange(0, int(self.get_size().y)):
			props.append(self.get_row_prop(row))
		return props
	
	def set_rowsprop(self, props):
		count = len(props)
		size = self.get_size()
		if size.y < count:
			self.set_size(Vector(size.x, count))
		for row in xrange(0, count):
			self.set_row_prop(row, int(props[row]))
	
	size = DeprecatedDescriptor('size')
	spacing = DeprecatedDescriptor('spacing')
	rowsprop = DeprecatedDescriptor('rowsprop')
	colsprop = DeprecatedDescriptor('colsprop')
	


@factory
class HBox(Sizer):
	NAME						= 'hbox'
	
	def initialize(self):
		Sizer.initialize(self)
		self.set_orientation(slew.HORIZONTAL)



@factory
class VBox(HBox):
	NAME						= 'vbox'
	
	def initialize(self):
		Sizer.initialize(self)
		self.set_orientation(slew.VERTICAL)

