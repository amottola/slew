import slew


class Application(slew.Application):
	def run(self):
		xml = """
			<frame title="Hello">
				<vbox>
					<label align="center">Hello world!</label>
				</vbox>
			</frame>
		"""
		frame = slew.Frame(xml)
		frame.show()

slew.run(Application())
