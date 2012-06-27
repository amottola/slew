import slew

from utils import *


@factory
class StackView(slew.Window):
	NAME						= 'stackview'
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'page':					IntProperty(),
	})
	
# properties
	
	def get_page(self):
		return self._impl.get_page()
	
	def set_page(self, page):
		if isinstance(page, slew.TabViewPage):
			page = self.get_children().index(page)
		self._impl.set_page(page)
	
	page = DeprecatedDescriptor('page')



