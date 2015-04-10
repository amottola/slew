import slew

from utils import *
from window import Window, Layoutable


@factory
class FlowBox(Layoutable):
	NAME						= 'flowbox'
	
	PROPERTIES = merge(Layoutable.PROPERTIES, {
		'spacing':				VectorProperty(),
		'horizontal_spacing':	IntProperty(),
		'vertical_spacing':		IntProperty(),
		'orientation':			ChoiceProperty(['horizontal', 'vertical']),
	})
	
	def create_impl(self, class_name):
		return Layoutable.create_impl(self, 'FlowBox')
	

# methods
	
	def fit(self):
		parent = self.get_parent()
		while (parent is not None) and (not isinstance(parent, Window)):
			parent = parent.get_parent()
		if parent is not None:
			return parent.fit()
	
	def get_minsize(self):
		return self._impl.get_minsize()
	
# properties
	
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
	
	spacing = DeprecatedDescriptor('spacing')
	


@factory
class HFlowBox(FlowBox):
	NAME						= 'hflowbox'
	
	def initialize(self):
		FlowBox.initialize(self)
		self.set_orientation(slew.HORIZONTAL)



@factory
class VFlowBox(FlowBox):
	NAME						= 'vflowbox'
	
	def initialize(self):
		FlowBox.initialize(self)
		self.set_orientation(slew.VERTICAL)

