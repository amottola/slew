import slew

class Model(slew.DataModel):
	def row_count(self, index):
		if index is None:
			return 100
		return 0
	def column_count(self):
		return 5
	def header(self, pos):
		return slew.DataSpecifier(text="Column %d" % pos.x, width=(50 + pos.x * 20))
	def data(self, index):
		d = slew.DataSpecifier()
		d.text = '%c-%d' % (65+index.column, index.row)
		if index.column == 0:
			d.flags &= ~slew.DataSpecifier.READONLY
		return d

class Application(slew.Application, slew.EventHandler):
	def run(self):
		self.frame = slew.Frame()
		grid = slew.Grid()
		grid.set_model(Model())
		self.frame.append(slew.VBox().append(grid))
		self.frame.show()

slew.run(Application())
