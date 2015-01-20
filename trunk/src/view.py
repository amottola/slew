import slew

from utils import *


class View(slew.Window):
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'model':				Property(),
		'selection':			Property(),
	})
	
	EMPTY_MODEL = None
	
	def initialize(self):
		slew.Window.initialize(self)
		if View.EMPTY_MODEL is None:
			View.EMPTY_MODEL = slew.DataModel()
		self.set_model(None)
	
	def get_model(self):
		return self.__model
	
	def set_model(self, model):
		model = slew.DataModel.ensure(model)
		self._impl.set_model(model or View.EMPTY_MODEL)
		self.__model = model
	
	def show_index(self, index):
		self._impl.show_index(slew.DataIndex.ensure(index))
	
	def get_scroll_pos(self):
		return self._impl.get_scroll_pos()
	
	def set_scroll_pos(self, pos):
		self._impl.set_scroll_pos(Vector.ensure(pos, False))
	
	def get_scroll_rate(self):
		return self._impl.get_scroll_rate()
	
	def set_scroll_rate(self, pos):
		self._impl.set_scroll_rate(Vector.ensure(pos, False))

	def get_viewport_size(self):
		return self._impl.get_viewport_size()
	
	@deprecated
	def show_item(self, index):
		self.show_index(index)
	
	def get_current_index(self):
		return self._impl.get_current_index()
	
	def set_current_index(self, index):
		return self._impl.set_current_index(slew.DataIndex.ensure(index))
	
	def get_selection(self):
		return self._impl.get_selection()
	
	def set_selection(self, selection):
		self._impl.set_selection(selection or ())
	
	def get_index_at(self, pos):
		return self._impl.get_index_at(Vector.ensure(pos, False))
	
	def get_index_rect(self, index):
		return self._impl.get_index_rect(slew.DataIndex.ensure(index, False))
	
	def set_highlighted_indexes(self, indexes):
		if indexes is None:
			indexes = ()
		if not isinstance(indexes, (list, tuple)):
			indexes = ( indexes, )
		self._impl.set_highlighted_indexes(indexes)
	
	def popup_message(self, text, align=slew.BOTTOM, buttons=0, index=None):
		return self._impl.popup_message(text, align, buttons, slew.DataIndex.ensure(index))

	model = DeprecatedDescriptor('model')
	selection = DeprecatedDescriptor('selection')
