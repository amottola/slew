import slew

logo = slew.Bitmap(resource='python-logo.png')
size = logo.get_size()

class Logo(slew.SceneItem):
	def onPaint(self, e):
		e.dc.blit(logo, (0,0))

class Item(slew.SceneItem):
	def __init__(self, i):
		subitem = Logo()
		subitem.set_size(size)
		subitem.set_pos((20,20))
		self.append(subitem)
		self.set_size(size + (40,40))
		self.set_pos(((i % 10) * (size.w + 100), (i / 10) * (size.h + 100)))
	def onPaint(self, e):
		e.dc.bgcolor = (255,255,255)
		e.dc.rect((0,0), self.get_size())

class Application(slew.Application):
	def run(self):
		self.frame = slew.Frame()
		scene = slew.SceneView(bgcolor=(64,64,64))
		for i in xrange(0, 100):
			scene.append(Item(i))
		self.frame.append(slew.VBox().append(scene))
		self.frame.show()

slew.run(Application())
