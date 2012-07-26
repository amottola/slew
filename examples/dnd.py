import slew

class Handler(slew.EventHandler):
	def onDragStart(self, e):
		e.data = e.widget.get_bitmap()
		return e.data is not None
	def onDragMove(self, e):
		return isinstance(e.data, slew.Bitmap)
	def onDrop(self, e):
		e.widget.set_bitmap(e.data)
	def onPaint(self, e):
		size = e.widget.get_size()
		e.dc.rect((0,0), size)
		if e.widget.get_bitmap() is None:
			e.dc.text('Drop images here', (0,0), size, slew.ALIGN_CENTER)

class Application(slew.Application):
	def run(self):
		self.frame = slew.Frame()
		pict = slew.Image(size=(300,300))
		pict.set_handler(Handler())
		self.frame.append(slew.VBox(margins=20).append(pict))
		self.frame.show()

slew.run(Application())
