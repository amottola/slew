import slew


# print slew.Paper.common_list()
# slew.message_box('Hello world!')
# print slew.get_locale_info()
# print slew.get_computer_info()
# 
# 
# r = slew.get_available_desktop_rect()
# print r[0], r[1]
# 
# # raise ValueError("Hello world!")
# 
# c = slew.run_color_dialog(slew.Color(255,0,0,128), "Prova colore")
# print c
# 
# f = slew.run_font_dialog(None)
# print f



xml = """
<frame title="Prova!" style="caption|resize|close">
	<sizer name="sizer" rowsprop="1,0,1">
		<sizeritem name="content_view" flags="expand">
			<panel>
				<button name="menu_button" pos="100,100" label="D'oh" size="120,-1" />
				<toolbutton name="clone_button" pos="100,200" size="40,40" />
				<button name="move_button" pos="150,150" label="Move statusbar" size="150,-1" />
				<toolbutton name="tool" pos="200,100" size="80,80" style="flat" icon="about_logo.png, selected: about_logo.png" cursor="hand" />
			</panel>
		</sizeritem>
		<foldpanel label="folding...">
			<vbox>
				<hbox margins="0 0 0 0">
					<button prop="1" label="Prop 1" />
					<button prop="2" label="No prop" boxalign="right|vcenter" />
					<hyperlink text="Click me" url="www.easybyte.it" />
					<image bitmap="nuova_scheda.png" />
					<line style="vertical" />
					<slider name="slider" style="horizontal" size="100,-1" onChange="slider_h" />
				</hbox>
				<hbox>
					<groupbox label="Title">
						<sizer>
							<columns>
								<column width="30%" />
								<column />
							</columns>
							<row>
								<vbox>
									<checkbox label="Check 1" />
									<checkbox label="Check 2" />
									<checkbox label="Check 3" />
									<button name="popup" />
								</vbox>
								<vbox>
									<radio label="Option 1" />
									<radio label="Option 2" />
									<radio label="Option A" group="abc" onSelect="radio_h" />
									<radio label="Option B" group="abc" onSelect="radio_h" />
									<radio label="Option C" group="abc" onSelect="radio_h" />
									<textfield size="200,-1" boxalign="right" style="entertabs" />
								</vbox>
							</row>
						</sizer>
					</groupbox>
				</hbox>
			</vbox>
		</foldpanel>
		<sizeritem />
		<foldpanel label="another fold">
			<vbox>
				<hbox margins="0 0 0 0">
					<button prop="1" label="Prop 1" />
					<button prop="2" label="No prop" boxalign="right|vcenter" />
					<hyperlink text="Click me" url="www.easybyte.it" />
					<image bitmap="nuova_scheda.png" />
					<line style="vertical" />
					<slider name="slider" style="horizontal" size="100,-1" onChange="slider_h" />
				</hbox>
				<searchfield name="search" style="cancel" />
				<hbox>
					<combobox name="combo">
						<option>Uno</option>
						<option>Due</option>
						<option>Tre</option>
						<option selected="true">Quattro</option>
						<option>Cinque</option>
						<option>Sei</option>
						<option>Sette</option>
					</combobox>
					<textfield name="textfield" style="entertabs" />
				</hbox>
				<textfield datatype="decimal" format="5.2m" value="12.34567" />
				<hbox margins="30 0 0 0">
					<textview prop="1" length="5" />
					<listbox prop="1">
						<option>Uno</option>
						<option>Due</option>
						<option>Tre</option>
						<option>Quattro</option>
						<option>Cinque</option>
						<option>Sei</option>
						<option>Sette</option>
					</listbox>
					<iconview name="icons" style="" />
				</hbox>
				<grid name="grid" style="rules|header|vheader|nofocus|altrows" />
			</vbox>
		</foldpanel>
	</sizer>
</frame>
"""

menu_xml = """
	<menubar>
		<menu title="File">
			<menuitem text="Open" />
			<menuitem text="Close" />
			<menuitem type="separator" />
			<menuitem text="Quit" />
		</menu>
		<menu title="Edit">
			<menuitem text="Check" type="check" />
			<menu title="Submenu">
				<menuitem text="a" />
				<menuitem text="b" />
				<menuitem text="c" />
			</menu>
			<menuitem type="separator" />
			<menuitem type="radio" text="Radio 1A" />
			<menuitem type="radio" text="Radio 1B" />
			<menuitem type="radio" text="Radio 1C" />
			<menuitem type="separator" />
			<menuitem type="radio" text="Radio 2A" />
			<menuitem type="radio" text="Radio 2B" />
			<menuitem type="radio" text="Radio 2C" />
			<menuitem text="Final check" type="check" />
		</menu>
	</menubar>
"""

menu2_xml = """
	<menu title="Popup">
		<menuitem text="Pippo" />
		<menuitem text="Pluto" />
		<menuitem type="separator" />
		<menuitem text="Paperino" type="check" />
	</menu>
"""

toolbar_xml = """
	<toolbar>
		<toolbaritem text="New" bitmap="nuova_scheda.png" />
		<toolbaritem text="New" bitmap="nuova_scheda.png" />
		<toolbaritem text="New" bitmap="nuova_scheda.png" />
		<toolbaritem type="separator" />
		<toolbaritem text="New" bitmap="nuova_scheda.png" />
		<toolbaritem type="separator" />
		<toolbaritem text="New" bitmap="nuova_scheda.png" type="check" />
		<toolbaritem type="separator" flexible="true" />
		<toolbaritem text="New" bitmap="nuova_scheda.png" />
	</toolbar>
"""

test_xml = """
	<dialog name="custom_dialog" style="caption|close|resize">
		<sizer rowsprop="1,0">
			<sizeritem name="content_view" flags="expand" />
			<sizeritem bordersize="5" flags="expand">
				<sizer size="3,1" colsprop="1,0,0">
					<sizeritem name="bottom_space" align="left" />
					<sizeritem bordersize="5">
						<button size="80,-1" name="cancel" label="Annulla" />
					</sizeritem>
					<sizeritem bordersize="5">
						<button size="80,-1" name="ok" label="Ok" default="True" />
					</sizeritem>
				</sizer>
			</sizeritem>
		</sizer>
	</dialog>
"""

sizer_xml = """
	<sizeritem bordersize="5" flags="expand">
		<sizer size="3,1" colsprop="1,0,0">
			<sizeritem name="bottom_space" align="left" />
			<sizeritem bordersize="5">
				<button size="80,-1" name="cancel" label="Annulla" />
			</sizeritem>
			<sizeritem bordersize="5">
				<button size="80,-1" name="ok" label="Ok" default="True" />
			</sizeritem>
		</sizer>
	</sizeritem>
"""


class Handler(slew.EventHandler):
	def __init__(self):
		self.bitmap = slew.Bitmap(resource="about_logo.png")
	def onResize(self, e):
		def later(s):
			print "resized", s
		slew.call_later(later, e.size)
# 	def onClose(self, e):
# 		if e.widget.message_box("Are you sure?", "Closing", slew.BUTTON_OK|slew.BUTTON_CANCEL, slew.ICON_WARNING) == slew.BUTTON_CANCEL:
# 			return False
	def onClick(self, e):
		print "clicked!"
	def onPaint(self, e):
		if isinstance(e.widget, slew.Frame):
			size = e.dc.get_size()
			e.dc.line((0,0),size)
# 			e.dc.blit(self.bitmap, (0,0))
			e.dc.blit(e.widget.find('tool').get_icon().normal, (0,0))
# 			print size
# 		print slew.get_application().statusbar.get_props()



class CompleterModel(slew.DataModel):
	def __init__(self):
		slew.DataModel.__init__(self)
		self.words = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed consectetur euismod pharetra. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia Curae; Aenean ut enim nisi, ut mattis augue. Nam sem justo, hendrerit in lobortis id, semper sit amet sapien. Fusce sed elit nec enim lobortis posuere ut eu dolor.".split(' ')
	
	def row_count(self, index):
		if index is None:
			return len(self.words)
		return 0
	
	def data(self, index):
		return slew.DataSpecifier(self.words[index.row])




class IconsModel(slew.DataModel):
	ICON = slew.Bitmap(resource="nuova_scheda.png")
	
	def row_count(self, index):
		if index is None:
			return 3
		return 0
	
	def data(self, index):
		d = slew.DataSpecifier()
		d.icon = self.ICON
		d.text = "Icon %d" % index.row
		d.align = slew.ALIGN_CENTER
		d.flags |= d.DRAGGABLE
		if index.row > 0:
			d.flags |= d.DROP_TARGET
		return d



class IconsHandler(slew.EventHandler):
	def onDragStart(self, e):
		e.data = "Hello world!"
		print "onDragStart", e
	def onDragEnter(self, e):
		pass
	def onDragMove(self, e):
		pass
	def onContextMenu(self, e):
		if e.index >= 0:
			e.widget.set_highlighted_indexes(e.widget.get_model().index(e.index))
			menu = slew.Menu()
			menu.append(slew.MenuItem(text='Hello world!'))
			menu.popup()
			e.widget.set_highlighted_indexes(None)
			



class GridModel(slew.DataModel):
# 	def __init__(self):
# 		slew.DataModel.__init__(self)
# 		self.data = []
	
	def column_count(self):
		return 3
	
	def row_count(self, index):
		if index is None:
			return 10
		return 0
	
	def data(self, index):
		d = slew.DataSpecifier()
		d.text = "%d,%d" % (index.row, index.column)
		d.flags = d.SELECTABLE | d.ENABLED
		return d
	
	def header(self, pos):
		d = slew.DataSpecifier()
		if pos.x == 1:
			d.width = 250
# 		d.height = 20
		d.text = "Column %d" % pos.x
		return d




class MyModel(slew.DataModel):
	
	def __init__(self):
		slew.DataModel.__init__(self)
		self.rows = 5
		r = range(0, self.row_count())
		self._data = [[chr(65+i)*5, 0, 0, chr(65 + i)+str(i), 'Hello <b>world</b>!'] for i in r]
# 		self._icons = IconModel()
		
	def row_count(self, index=None):
		if index:
			return 0
		return self.rows
	
	def column_count(self):
		return 5

	def header(self, pos):
		if pos.y < 0:
			return slew.DataSpecifier(text = "Column %c" % chr(65 + int(pos.x)), width = 100 + (int(pos.x) * 50))
		else:
			return slew.DataSpecifier(text = "%d" % int(pos.y))
	
	def data(self, index):
		d = slew.DataSpecifier()
		if index.column == 0:
			d.text = self._data[index.row][0]
			d.icon = slew.Bitmap(size=(16,16))
			d.icon.bgcolor = (0,255,0)
			d.icon.rect((0,0),(15,15))
			d.flags |= slew.DataSpecifier.CLICKABLE_ICON
			d.completer = slew.Completer(model=CompleterModel())
#			d.flags = slew.DataSpecifier.READONLY
# 			d.completer = slew.Completer(model=CompletionModel(10))#self._icons)
# 		elif index.column == 1:
# 			d.flags = slew.DataSpecifier.CHECKBOX
# 			d.selection = self._data[index.row][1]
# 		elif index.column == 2:
# 			d.flags = slew.DataSpecifier.COMBOBOX
# 			d.choices = ('Pippo', 'Pluto', 'Paperino')
# 			d.selection = self._data[index.row][2]
		elif index.column == 3:
			d.text = self._data[index.row][3]
		elif index.column == 4:
			d.text = self._data[index.row][4]
			d.icon = slew.Bitmap(size=(16,16))
			d.icon.bgcolor = (255,0,0)
			d.icon.rect((0,0),(15,15))
			d.datatype = slew.DATATYPE_TIME
			if (index.row % 2 == 0):
				d.format = "HMr"
			else:
				d.format = "HM"
			d.flags |= slew.DataSpecifier.HTML
# 			print "data: ", d
		d.flags |= slew.DataSpecifier.DEFAULT | slew.DataSpecifier.DRAGGABLE | slew.DataSpecifier.DROP_TARGET
		if (index.column != 3):
			d.flags &= ~slew.DataSpecifier.READONLY
		return d
		
	def set_data(self, index, d):
# 		print "set_data", index.row, index.column, d.text, d.selection
		if index.column in (1, 2):
			self._data[index.row][index.column] = d.selection
		elif index.column == 4:
			self._data[index.row][4] = d.text
# 			if d.text == '':
# # 				slew.message_box("Invalid empty field!")
# 				return False
		else:
			self._data[index.row][index.column] = d.text

	def addRows(self):
		print "adding row at position 2..."
		self.rows += 1
		self._data.insert(2, ['another test', 1, 2, 'test', '12:59:00'])
		self.notify(slew.DataModel.NOTIFY_ADDED_ROWS, 2, 1)

	def removeRows(self):
		print "removing 2 rows at position 3..."
		self.rows -= 2
		del self._data[3:5]
		self.notify(slew.DataModel.NOTIFY_REMOVED_ROWS, 3, 2)



class GridHandler(slew.EventHandler):
	def onChange(self, e):
		if e.index.column == 0:
			if e.value and e.completion < 0:
				e.widget.complete()
	def onModify(self, e):
		print "Grid onModify for index:", e.index
# 		return False
	def onDragEnter(self, e):
		pass
	def onDragMove(self, e):
		pass



class Application(slew.Application):
	def run(self):
		def slider_h(e):
			print "slider changed:", e.value, e.widget.get_min(), e.widget.get_max()
		
		def radio_h(e):
			frame = e.widget.get_parent_frame()
			print "selected radio:", frame.get_radio('abc').get_name()
		
		class popup_h(slew.EventHandler):
			def __init__(self, f):
				self.f = f
			def onClick(self, e):
				self.p = slew.PopupWindow()
				self.p.append(slew.Sizer().append(slew.Calendar()))
				self.p.popup(e.widget, (0,e.widget.size.h))
				def clicked(e):
					print "selected date:", e.date
				self.p[0][0].onClick = clicked
				print "popped up"
		
		self.clones = []
		self.f = slew.Frame(xml, globals(), locals())
# 		self.f = slew.Frame(xml, globals(), locals(), title="Prova!", size=(500,500), minsize=(100,100))
		self.f.handler = Handler()
		button = self.f.find('menu_button')
		button.handler = Handler()
		button.set_menu(slew.Menu(menu2_xml))
# 		button.set_pos((0,0))
		button.set_margins(20)
		print button.get_margins()
		button = self.f.find('clone_button')
		def clone(e):
			self.clones.append(self.f.clone())
			self.clones[-1].show()
		button.onClick = clone
		
# 		self.statusbar = slew.MenuBar(menu_xml)
# 		self.statusbar = slew.ToolBar(toolbar_xml)
		self.statusbar = slew.SizerItem(sizer_xml)

# 		self.statusbar.append(slew.ToolButton(size=(20,20), label="+"))
# 		self.statusbar.append(slew.ToolButton(size=(20,20), label="-"))
		self.current = None
		def move(e):
			if self.current is None:
				self.current = 0
			else:
				self.current += 1
			if self.current >= len(self.clones):
				self.current = 0
			print "moving to", self.current
			sizer = self.clones[self.current].find('sizer')
			sizer.append(self.statusbar)
				
		button = self.f.find('move_button')
		button.onClick = move
		
# 		slew.set_shortcut("esc", lambda: slew.exit())
		
# 		self.f.find('slider').onChange = slider_h
		
		self.f.find('icons').model = IconsModel()
		self.f.find('icons').handler = IconsHandler()
		
		self.f.find('grid').model = MyModel()#GridModel()
# 		self.f.find('grid').handler = GridHandler()
		
		self.f.find('popup').handler = popup_h(self.f)


# 		slew.chain_debug_handler(self.f, True, 'onModify onFocus* onSelect onDblClick onContextMenu onChange onClick onCancel onEnter onCell*')
# 		slew.chain_debug_handler(self.f, True, 'onModify onFocus* onCell* onDrag*')
# 		slew.chain_debug_handler(self.f, True, 'onCell*')# onFocus* onChange*')
		
		self.f.center()
		self.f.show()
		
		tf = self.f.find('textfield')
		tf.set_completer(slew.Completer(CompleterModel()))
		def complete(e):
			print 'textfield onChange', e
			if (e.completion < 0) and len(e.value):
				tf.complete()
		def focus_out(e):
			print "focus out!", e.widget
			if isinstance(e.widget, slew.Grid):
				return e.widget.popup_message("Hello world! ", align=slew.RIGHT, buttons=slew.BUTTON_OK|slew.BUTTON_CANCEL, index=e.index) == slew.BUTTON_OK
			else:
				return e.widget.popup_message("Hello world! ", align=slew.RIGHT, buttons=slew.BUTTON_OK|slew.BUTTON_CANCEL) == slew.BUTTON_OK
		tf.onChange = complete
		tf.onFocusOut = focus_out
		self.f.find('grid').onCellEditEnd = focus_out
		
		print self.f.find('combo').items
		
# 		self.f2 = slew.Dialog(test_xml)
# 		self.f2.show_modal()


slew.run(Application())
