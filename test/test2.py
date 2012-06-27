# -*- coding: utf-8 -*-


import sys, os
sys.path += [ '../lib']

import slew


xml = """
<frame size="180,180">
	<sizer rowsprop="1,1,1,1">
		<sizeritem flags="expand" bordersize="10">
			<button name="setup" label="Page setup" pos="30,20" size="120,20" />
		</sizeritem>
		<sizeritem flags="expand" bordersize="10">
			<button name="preview" label="Preview" pos="30,80" size="120,20" />
		</sizeritem>
		<sizeritem flags="expand" bordersize="10">
			<button name="print" label="Print" pos="30,110" size="120,20" />
		</sizeritem>
		<sizeritem flags="expand" bordersize="10">
			<button name="pdf" label="Save as PDF" pos="30,140" size="120,20" />
		</sizeritem>
		
		<sceneview name="view" style="opengl|nofocus" size="400,400" />
	</sizer>
</frame>
"""


kong = slew.Bitmap(resource="about_logo.png").resized((1000,1000))


class Handler(slew.EventHandler):
	def onClick(self, e):
		name = e.widget.name
		if name == 'setup':
			slew.get_application().PageSetup()
		elif name == 'preview':
			slew.print_document(slew.PRINT_PREVIEW, 'Test', slew.get_application().Print_Callback, 4, settings = slew.get_application().print_settings)
		elif name == 'print':
			slew.print_document(slew.PRINT_PAPER, 'Test', slew.get_application().Print_Callback, 4, settings = slew.get_application().print_settings)
		elif name == 'pdf':
			open('test.pdf', 'wb').write(slew.print_document(slew.PRINT_PDF, 'Test', slew.get_application().Print_Callback, 4, settings = slew.get_application().print_settings))


class ViewHandler(slew.EventHandler):
	def onPaint(self, e):
		if e.background:
			e.dc.blit(kong, (0,0))
		
	def onResize(self, e):
		pass
# 		e.widget.set_anchor(slew.SceneView.ANCHOR_CENTER)
# 		e.widget.set_scene_rect(*e.widget.items_bounding_rect())
# 		e.widget.fit_in_view((-100,0), (2200,1))


class SceneItem(slew.SceneItem):
	def __init__(self, **kwargs):
		slew.SceneItem.__init__(self, **kwargs)
		self.set_effect(slew.SceneItem.EFFECT_SHADOW)
	
	def onPaint(self, e):
		tl = slew.Vector(0,0)
		br = e.dc.size - (1,1)
		e.dc.rect(tl, br)
		e.dc.line(tl, br)
		e.dc.line(slew.Vector(br.x, 0), slew.Vector(0, br.y))


class Application(slew.Application):

	def __init__(self):
		self.print_settings = slew.PrinterSettings()
	
	
	def PageSetup(self):
		print "Before: ", self.print_settings
		slew.page_setup(self.print_settings)
		print "After: ", self.print_settings
	
	
	@staticmethod
	def Print_Callback(dc, page, userdata):
		if (page == 2):
			dc.size = (3000,3000)
		elif (page == 3):
			dc.dpi = (100,100)
		elif (page == 4):
			dc.set_size((2100, 2970))
		
		print 'Printer DPI: ', dc.get_printer_dpi()
		print 'size: ', dc.size
		print 'dpi: ', dc.dpi
		pageRect = dc.get_page_rect()
		print 'page: ', pageRect
# 		paperRect = dc.get_paper_rect()
# 		print 'paper: ', paperRect
		
		s = dc.size.w - 1
		
		dc.color = (0,255,0)
		print dc.color, dc.bgcolor
		dc.rect((0,0), (s,s))
		dc.line((0,0), (s,s))
		dc.line((s,0), (0,s))
		dc.rect((3,10), (5,12))
		
		dc.color = (255,0,0)
		dc.bgcolor = None
		
		text = "La pagina è: %d" % page
		
		dc.font = slew.Font(size=40)
		dc.text(text, (100, 50))
		dc.rect((100, 50), slew.Vector(100, 50) + dc.text_extent(text))
		
		families = {
			slew.Font.FAMILY_ROMAN:		'Roman',
			slew.Font.FAMILY_SCRIPT:		'Script',
			slew.Font.FAMILY_SANS_SERIF:	'Sans-serif',
			slew.Font.FAMILY_FIXED_PITCH:	'Fixed pitch',
			slew.Font.FAMILY_TELETYPE:	'Teletype',
		}
		
# 		pos = slew.Vector(100, 200)
# 		dc.color = (0,0,0)
# 		for (key, value) in families.iteritems():
# 			dc.font = slew.Font(size=12, family=key)
# 			dc.text(value, pos)
# 			pos += (0, dc.text_extent(value).h)
# 		print str(a)
		
		dc.rect(pageRect[1] - (2,2), pageRect[1])
		dc.color = (0,255,0)
		dc.point(pageRect[1])
		
		dc.color = (0,0,0)
		left = (float(pageRect[0].x) * 2.54) / dc.dpi.x
		top = (float(pageRect[0].y) * 2.54) / dc.dpi.y
		right = (float(pageRect[1].x) * 2.54) / dc.dpi.x
		bottom = (float(pageRect[1].y) * 2.54) / dc.dpi.y
		print left, top, right, bottom
		
		def cm(cm):
			return (cm / 2.54) * dc.dpi.x
		
		dc.color = (0,0,255)
		dc.line((cm(5) - pageRect[0].x, cm(10)), (cm(5) - pageRect[0].x, cm(20)))
		dc.line((cm(5) - pageRect[0].x, cm(20)), (cm(15) - pageRect[0].x, cm(20)))
		
		dc.font = slew.Font(size=int(cm(1)))
		dc.text("10x10 cm", (cm(6), cm(18)))
		
		bmp = slew.Bitmap(resource=os.path.join(os.getenv('REPOSITORY'), "trunk", "data", 'tango', 'battery.png'))
		dc.blit(bmp, (cm(5), cm(21)))
		
		bmp.color = (255,0,0)
		bmp.bgcolor = None
		bmp.rect((0,0), (31,31))
		bmp.color = (0,255,0)
		bmp.line((0,0), (31,31))
		bmp.line((31,0), (0,31))
		dc.blit(bmp, (cm(7), cm(20) + 1))
		
		dc.color = (0,255,0)
		text = "%s dpi, %s size" % (str(dc.dpi), str(dc.size))
		s = dc.text_extent(text)
		print "Text extent for '%s': %s" % (text, str(s))
		dc.text(text, ((pageRect[1].x - pageRect[0].x - s.w + 1) / 2, cm(22)))
	
	def run(self):
		self.f = slew.Frame(xml)
		for n in [ 'setup', 'preview', 'print', 'pdf' ]:
			self.f.find(n).handler = Handler()
		
		scene = self.f.find('view')
		scene.handler = ViewHandler()
		
# 		scene.set_align(slew.ALIGN_TOP | slew.ALIGN_LEFT)
		scene.append(SceneItem(size=(2100, 2970)))
		
		print slew.get_screen_dpi()
		scale = slew.get_screen_dpi().x / 254			# 100%
		scene.set_scale(scale)
		
		slew.chain_debug_handler(self.f, True, 'onDrag* onMouseWheel onDblClick')
		self.f.show()


slew.run(Application())

