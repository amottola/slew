import slew

from utils import *
from gdi import *
from label import Label


@factory
class Hyperlink(Label):
	NAME						= 'hyperlink'
	
	PROPERTIES = merge(Label.PROPERTIES, {
		'url':					StringProperty(),
		'label':				TranslatedStringProperty(True),
	})
	
	def create_impl(self, class_name):
		return Label.create_impl(self, 'Label')
	
	def initialize(self):
		Label.initialize(self)
		self.__text = ''
		self.__url = ''
		self.set_style(0)
	
	def onClick(self, e):
		slew.system_open(e.url)
	
	def _update(self):
		self._impl.set_tip(self.__url)
		self._impl.set_text('<a href="%s" style="text-decoration: none">%s</a>' % (self.__url, self.__text))
		
# properties
		
	def set_style(self, style):
		Label.set_style(self, style | self.STYLE_HTML)
	
	def get_visited(self):
		pass
	
	def set_visited(self, visited):
		pass
	
	def get_hover(self):
		pass
	
	def set_hover(self, hover):
		pass
	
	def get_text(self):
		return self.__text
	
	def set_text(self, text):
		self.__text = text
		self._update()
	
	def get_url(self):
		return self.__url
	
	def set_url(self, url):
		self.__url = url
		self._update()
	
	set_label = set_text
	get_label = get_text
	
	url = DeprecatedDescriptor('url')
	visited = DeprecatedDescriptor('visited')
	hover = DeprecatedDescriptor('hover')

