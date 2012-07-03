import slew

class App(slew.Application):
	def run(self):
		interface = slew.load_interface("prompt.xml")
		self.dialog = interface['test'].create()
		self.dialog.find('ok').onClick = self.clicked
		self.dialog.show_modal()
	
	def clicked(self, e):
		slew.message_box("Hi %s!" % self.dialog.find('my_name').get_value())
		self.dialog.end_modal()

slew.run(App())
