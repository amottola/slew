import slew

from utils import *


@factory
class WebView(slew.Window):
	NAME						= 'webview'
	
	#defs{SL_WEBVIEW_
	STYLE_PRIVATE				= 0x00010000
	STYLE_JAVASCRIPT			= 0x00020000
	STYLE_AUTO_LINKS			= 0x00040000
	STYLE_PLUGINS				= 0x00080000
	#}
	
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"private":		STYLE_PRIVATE,
									"javascript":	STYLE_JAVASCRIPT,
									"autolinks":	STYLE_AUTO_LINKS,
									"plugins":		STYLE_PLUGINS,
								})),
		'url':					StringProperty(),
	})
	
	
# methods
	
	def get_title(self):
		return self._impl.get_title()
	
	def get_icon(self):
		return self._impl.get_icon()
	
	def get_selected_text(self):
		return self._impl.get_selected_text()
	
	def get_selected_html(self):
		return self._impl.get_selected_html()
	
	def set_html(self, html, base_url=None):
		self._impl.set_html(html, base_url or '')
	
	def get_history(self):
		return self._impl.get_history_items()
	
	def clear_history(self):
		self._impl.clear_history()
	
	def get_current_history_index(self):
		return self._impl.get_current_history_index()
	
	def set_current_history_index(self, index):
		return self._impl.set_current_history_index(index)
	
	def clear_cache(self):
		self._impl.clear_cache()
	
	def back(self):
		self._impl.back()
	
	def forward(self):
		self._impl.forward()
	
	def stop(self):
		self._impl.stop()
	
	def reload(self):
		self._impl.reload()
	
	def print_document(self, type, name, prompt=True, settings=None, parent=None):
		return self._impl.print_document(type, name, prompt, settings, parent)
	
# properties
	
	def get_url(self):
		return self._impl.get_url()
	
	def set_url(self, url):
		self._impl.set_url(url)
	



