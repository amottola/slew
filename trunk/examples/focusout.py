import slew

def denyFocusOut(e):
	e.widget.popup_message("Can't focus out!")
	return False

xml = """
	<frame title="Focus out denial">
		<vbox margins="10">
			<textfield onFocusOut="denyFocusOut" format="r5" datatype="integer" margins="0 0 10 0" />
			<checkbox label="Try focus me!" margins="0 0 10 0" />
			<button label="Or me..." />
		</vbox>
	</frame>
"""

class Application(slew.Application):
	def run(self):
		slew.Frame(xml, globals=globals()).show()

slew.run(Application())
