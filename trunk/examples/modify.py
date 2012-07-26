import slew


xml = """
	<frame title="Focus out denial">
		<vbox margins="10">
			<textfield name="textfield" datatype="timestamp" margins="0 0 10 0" />
			<checkbox name="checkbox" label="I'm a checkbox" margins="0 0 10 0" />
			<button name="button" label="Click me" />
		</vbox>
	</frame>
"""

class Application(slew.Application, slew.EventHandler):
	def onModify(self, e):
		self.frame.add_style(slew.Frame.STYLE_MODIFIED)
	def run(self):
		self.frame = slew.Frame(xml)
		self.frame.find("textfield").set_handler(self)
		self.frame.find("checkbox").set_handler(self)
		self.frame.find("button").set_handler(self)
		self.frame.show()

slew.run(Application())
