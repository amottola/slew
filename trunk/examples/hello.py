import slew


class Application(slew.Application):
	def run(self):
		frame = slew.Frame(title="Hello")
		frame.append(slew.VBox())
		frame[0].append(slew.Label(text="Hello world!", align=slew.ALIGN_CENTER))
		frame.show()

slew.run(Application())
