#
#
#

import sys
import os.path
try:
	import cPickle as pickle
except:
	import pickle

from utils import *
from gdi import *


class version_info(object):
	def __init__(self, major, minor, revision):
		self.version = ( major, minor, revision )
	def __getitem__(self, index):
		return self.version[index]
	def __str__(self):
		return '%d.%d.%d' % self.version

version = version_info(1, 0, 0)
del version_info



#defs{SL_
APPLICATION_QUIT_ON_WINDOWS_CLOSED	= 0x1


EXECUTABLE_NAME						= 0
EXECUTABLE_PATH						= 1
APPLICATION_PATH					= 2
RESOURCE_PATH						= 3
USER_HOME_PATH						= 4
USER_APPLICATION_SUPPORT_PATH		= 5
USER_DOCUMENTS_PATH					= 6
USER_IMAGES_PATH					= 7
USER_PREFERENCES_PATH				= 8
USER_LOG_PATH						= 9
SYSTEM_APPLICATION_SUPPORT_PATH		= 10
SYSTEM_DOCUMENTS_PATH				= 11
SYSTEM_IMAGES_PATH					= 12
SYSTEM_PREFERENCES_PATH				= 13
SYSTEM_FONTS_PATH					= 14
SYSTEM_LOG_PATH						= 15

MOUSE_BUTTON_LEFT					= 0x1
MOUSE_BUTTON_RIGHT					= 0x2
MOUSE_BUTTON_MIDDLE					= 0x4
	
MODIFIER_SHIFT						= 0x1
MODIFIER_ALT						= 0x2
MODIFIER_CONTROL					= 0x4
MODIFIER_META						= 0x8

KEY_SPECIAL							= 256

KEY_SHIFT							= 256
KEY_ALT								= 257
KEY_CONTROL							= 258
KEY_META							= 259
KEY_HOME							= 260
KEY_END								= 261
KEY_INSERT							= 262
KEY_DELETE							= 263
KEY_PAGE_UP							= 264
KEY_PAGE_DOWN						= 265
KEY_LEFT							= 266
KEY_RIGHT							= 267
KEY_UP								= 268
KEY_DOWN							= 269
KEY_F1								= 270
KEY_F2								= 271
KEY_F3								= 272
KEY_F4								= 273
KEY_F5								= 274
KEY_F6								= 275
KEY_F7								= 276
KEY_F8								= 277
KEY_F9								= 278
KEY_F10								= 279
KEY_BACKSPACE						= 280
KEY_RETURN							= 281
KEY_TAB								= 282
KEY_ESCAPE							= 283

SEQUENCE_NONE						= 0
SEQUENCE_UNDO						= 1
SEQUENCE_REDO						= 2
SEQUENCE_CUT						= 3
SEQUENCE_COPY						= 4
SEQUENCE_PASTE						= 5
SEQUENCE_CLOSE						= 6
SEQUENCE_SELECT_ALL					= 7

ICON_NEW							= 0
ICON_DELETE							= 1
ICON_CUT							= 2
ICON_COPY							= 3
ICON_PASTE							= 4
ICON_UNDO							= 5
ICON_REDO							= 6
ICON_FIND							= 7
ICON_FIND_AND_REPLACE				= 8
ICON_QUIT							= 9
ICON_FILE_OPEN						= 10
ICON_FILE_SAVE						= 11
ICON_FILE_SAVE_AS					= 12
ICON_NEW_FOLDER						= 13
ICON_FOLDER							= 14
ICON_FOLDER_OPEN					= 15
ICON_PARENT_FOLDER					= 16
ICON_PRINT							= 17
ICON_HELP							= 18
ICON_TIP							= 19
ICON_EXECUTABLE						= 20
ICON_ARCHIVE						= 21
ICON_CHECK							= 22
ICON_CROSS							= 23
ICON_ERROR							= 24
ICON_QUESTION						= 25
ICON_WARNING						= 26
ICON_INFORMATION					= 27
ICON_MISSING						= 28

CURSOR_NORMAL						= 0
CURSOR_IBEAM						= 1
CURSOR_HAND							= 2
CURSOR_WAIT							= 3
CURSOR_CROSS						= 4
CURSOR_NONE							= 5
CURSOR_MOVE							= 6
CURSOR_RESIZE						= 7
CURSOR_RESIZE_VERTICAL				= 8
CURSOR_RESIZE_HORIZONTAL			= 9
CURSOR_RESIZE_DIAGONAL_F			= 10
CURSOR_RESIZE_DIAGONAL_B			= 11
CURSOR_OPEN_HAND					= 12

BUTTON_OK							= 0x0001
BUTTON_YES							= 0x0002
BUTTON_YES_ALL						= 0x0004
BUTTON_NO							= 0x0008
BUTTON_NO_ALL						= 0x0010
BUTTON_CANCEL						= 0x0020
BUTTON_OPEN							= 0x0040
BUTTON_SAVE							= 0x0080
BUTTON_SAVE_ALL						= 0x0100
BUTTON_CLOSE						= 0x0200
BUTTON_DISCARD						= 0x0400
BUTTON_APPLY						= 0x0800
BUTTON_RESET						= 0x1000
BUTTON_ABORT						= 0x2000
BUTTON_RETRY						= 0x4000
BUTTON_IGNORE						= 0x8000

BUTTON_MAX							= 0x10000

PRINT_PREVIEW						= 0
PRINT_PAPER							= 1
PRINT_PDF							= 2

LEFT								= 0
RIGHT								= 1
TOP									= 2
BOTTOM								= 3

HORIZONTAL							= 0
VERTICAL							= 1

ALIGN_LEFT							= 0x00000001
ALIGN_HCENTER						= 0x00000002
ALIGN_RIGHT							= 0x00000004
ALIGN_JUSTIFY						= 0x00000008
ALIGN_TOP							= 0x00000010
ALIGN_VCENTER						= 0x00000020
ALIGN_BOTTOM						= 0x00000040
ALIGN_HMASK							= 0x0000000F
ALIGN_VMASK							= 0x000000F0
ALIGN_MASK							= 0x000000FF
ALIGN_CENTER						= 0x00000022
	
DATATYPE_UNKNOWN					= 0
DATATYPE_BOOL						= 1
DATATYPE_INTEGER					= 2
DATATYPE_DECIMAL					= 3
DATATYPE_FLOAT						= 4
DATATYPE_DATE						= 5
DATATYPE_TIME						= 6
DATATYPE_TIMESTAMP					= 7
DATATYPE_YEAR						= 8
DATATYPE_STRING						= 9

#}



sApplication = None
sArchiveDict = None
sTranslationsDict = {}



class Error(Exception):
	pass



class Application(object):
	NAME = None
	
	def init(self):
		pass
	def run(self):
		raise NotImplementedError("Application.run()")
	def exit(self):
		exit()
	def open_file(self, file):
		pass



class Completer(object):
	STYLE_UNFILTERED		= 0
	
	def __init__(self, model, column=0, bgcolor=None, hicolor=None, hibgcolor=None, bordercolor=None, style=None):
		self.model = model
		self.column = column
		self.color = bordercolor
		self.bgcolor = bgcolor
		self.hicolor = hicolor
		self.hibgcolor = hibgcolor
	
	@deprecated
	def complete(self):
		raise NotImplementedError("Completer.complete()")
	
	@classmethod
	def ensure(cls, model):
		if not isinstance(model, slew.Completer):
			raise ValueError("expecting 'Completer' object")
		return model



class WidgetDef(object):
	def __init__(self, node):
		self.type = get_factory(node.tag)
		self.name = node.attrib.get('name', '')
		self.node = node
	
	def create(self, globals=None, locals=None):
		widget = self.type()
		widget.load(None, globals, locals, self.node)
		return widget
	
	def get_type(self):
		return self.type
	
	def get_name(self):
		return self.name
	
	def get_node(self):
		return self.node



class Interface(object):
	def __init__(self, defs):
		self.__defs_list = defs
		self.__defs_dict = {}
		for d in defs:
			self.__defs_dict[d.name] = d
	
	def __len__(self):
		return len(self.__defs_list)
	
	def __getitem__(self, key):
		if isinstance(key, basestring):
			return self.__defs_dict[key]
		return self.__defs_list[key]
	
	def __iter__(self):
		return self.__defs_list.__iter__()
	
	def iteritems(self):
		return self.__defs_dict.iteritems()
	
	def itervalues(self):
		return self.__defs_dict.itervalues()
	
	def iterkeys(self):
		return self.__defs_dict.iterkeys()



class DataIndex(object):
	def __init__(self, row, column=0, parent=None):
		self.row = row
		self.column = column
		self.parent = parent
	
	def __str__(self):
		return 'DataIndex(%d, %d, %s)' % (self.row, self.column, str(self.parent))
	
	def __repr__(self):
		return str(self)
	
	def __eq__(self, other):
		return isinstance(other, DataIndex) and (self.row == other.row) and (self.column == other.column) and (self.parent == other.parent)
	
	@classmethod
	def ensure(cls, value, allowNone=True):
		if (value is None) and allowNone:
			return value
		if not isinstance(value, DataIndex):
			raise ValueError("expecting 'DataIndex' object")
		return value



class DataSpecifier(object):
	#defs{SL_DATA_SPECIFIER_
	TEXT				= 0x00000000
	CHECKBOX			= 0x00000001
	COMBOBOX			= 0x00000002
	BROWSER				= 0x00000003
	TYPE_MASK			= 0x000000FF
	CAPS				= 0x00000100
	READONLY			= 0x00000200
	SELECT_ON_FOCUS		= 0x00000400
	SELECTABLE			= 0x00000800
	ENABLED				= 0x00001000
	DRAGGABLE			= 0x00002000
	DROP_TARGET			= 0x00004000
	UNDEFINED			= 0x00008000
	CLICKABLE_ICON		= 0x00010000
	HTML				= 0x00020000
	ELIDE_LEFT			= 0x00040000
	ELIDE_MIDDLE		= 0x00080000
	ELIDE_RIGHT			= 0x00100000
	AUTO_WIDTH			= 0x00200000
	FIXED_WIDTH			= 0x00400000
	SEPARATOR			= 0x00800000
	
	INVALID				= -1
	
	DEFAULT				= 0x00001A00		# ENABLED | SELECTABLE | READONLY
	#}

	def __init__(self, text='', format='', align=ALIGN_LEFT|ALIGN_VCENTER, icon_align=ALIGN_CENTER, flags=DEFAULT, icon=None, color=None, bgcolor=None, font=None, selection=0, width=0, height=0, completer=None, tip='', browsed_data=None):
		self.text = text
		self.datatype = DATATYPE_STRING
		self.format = format
		self.format_vars = None
		self.align = align
		self.icon_align = icon_align
		self.length = 0
		self.filter = ''
		self.flags = flags
		self.icon = icon
		self.color = color
		self.bgcolor = bgcolor
		self.font = font
		self.selection = selection
		self.choices = ()
		self.width = width
		self.height = height
		self.completer = completer
		self.browsed_data = browsed_data
		self.tip = tip
		self.widget = None
		self.model = None
	
	def __eq__(self, other):
		return (self.text == other.text) and (self.datatype == other.datatype) and (self.format == other.format) and (self.format_vars == other.format_vars) and \
			   (self.align == other.align) and (self.length == other.length) and (self.filter == other.filter) and (self.flags == other.flags) and \
			   (self.selection == other.selection) and (self.choices == other.choices)
	
	def __hash__(self):
		return hash(self.text) ^ (self.datatype << 16) ^ (hash(self.format) << 32) ^ (hash(self.format_vars) << 48) ^ (hash(self.align) << 64) ^ \
			   (hash(self.length) << 80) ^ (hash(self.filter) << 96) ^ (hash(self.flags) << 112) ^ (hash(self.selection) << 128)



class DataModel(object):
	#defs{SL_DATA_MODEL_
	NOTIFY_RESET			= 0
	NOTIFY_ADDED_COLUMNS	= 1
	NOTIFY_ADDED_ROWS		= 2
	NOTIFY_CHANGED_COLUMNS	= 3
	NOTIFY_CHANGED_ROWS		= 4
	NOTIFY_REMOVED_COLUMNS	= 5
	NOTIFY_REMOVED_ROWS		= 6
	NOTIFY_CHANGED_CELL		= 7
	#}
	
	def __new__(cls, *args, **kwargs):
		self = object.__new__(cls)
		self._impl = _slew.DataModel(self)
		return self
	
	def index(self, row, column=0, parent=None):
		return DataIndex(row, column, parent)
	
	def data(self, index):
		return DataSpecifier()
	
	def header(self, column):
		if column.x < 0:
			return DataSpecifier(str(column.y))
		return DataSpecifier('Column %d' % column.x)
	
	def row_count(self, index=None):
		return 0
	
	def column_count(self):
		return 1
	
	def has_children(self, index=None):
		pass
	
	def set_data(self, index, value):
		pass
	
	def notify(self, what, index=0, count=1, parent=None):
		self._impl.notify(what, index, count, parent)
	
	def refresh_data_cache(self):
		self._impl.refresh_data_cache()
	
	@classmethod
	def ensure(cls, model):
		if (model is not None) and (not isinstance(model, slew.DataModel)):
			raise ValueError("expecting 'DataModel' object")
		return model



class FontDataModel(DataModel):
	FONTS = []
	
	def __init__(self):
		if not self.FONTS:
			self.FONTS = _slew.get_fonts_list()
	
	def row_count(self, index=None):
		if index is None:
			return len(FontDataModel.FONTS)
		return 0
	
	def data(self, index):
		font = FontDataModel.FONTS[index.row]
		return DataSpecifier(unicode(font.face), font=font)
	
	def get_font(self, index):
		return FontDataModel.FONTS[index]
	
	def reload_fonts(self):
		FontDataModel.FONTS = get_fonts_list()
	


class Paper(object):
	#defs{SL_PRINTER_PAPER_
	CUSTOM		= -1
	
	A0			= 0
	A1			= 1
	A2			= 2
	A3			= 3
	A4			= 4
	A5			= 5
	A6			= 6
	A7			= 7
	A8			= 8
	A9			= 9
	B1			= 10
	B2			= 11
	B3			= 12
	B4			= 13
	B5			= 14
	B6			= 15
	B7			= 16
	B8			= 17
	B9			= 18
	B10			= 19
	C5E			= 20
	COMM10E		= 21
	DLE			= 22
	EXECUTIVE	= 23
	FOLIO		= 24
	LEDGER		= 25
	LEGAL		= 26
	LETTER		= 27
	TABLOID		= 28
	#}
	
	INFO = (
		(	'A0',			"A0 (841 x 1189 mm)",								8410,	11890	),
		(	'A1',			"A1 (594 x 841 mm)",								5940,	8410	),
		(	'A2',			"A2 (420 x 594 mm)",								4200,	5940	),
		(	'A3',			"A3 (297 x 420 mm)",								2970,	4200	),
		(	'A4',			"A4 (210 x 297 mm)",								2100,	2970	),
		(	'A5',			"A5 (148 x 210 mm)",								1480,	2100	),
		(	'A6',			"A6 (105 x 148 mm)",								1050,	1480	),
		(	'A7',			"A7 (74 x 105 mm)",									740,	1050	),
		(	'A8',			"A8 (52 x 74 mm)",									520,	740		),
		(	'A9',			"A9 (37 x 52 mm)",									370,	520		),
		(	'B1',			"B1 (707 x 1000 mm)",								7070,	10000	),
		(	'B2',			"B2 (500 x 707 mm)",								5000,	7070	),
		(	'B3',			"B3 (353 x 500 mm)",								3530,	5000	),
		(	'B4',			"B4 (250 x 353 mm)",								2500,	3530	),
		(	'B5',			"B5 (176 x 250 mm)",								1760,	2500	),
		(	'B6',			"B6 (125 x 176 mm)",								1250,	1760	),
		(	'B7',			"B7 (88 x 125 mm)",									880,	1250	),
		(	'B8',			"B8 (62 x 88 mm)",									620,	880		),
		(	'B9',			"B9 (33 x 62 mm)",									330,	620		),
		(	'B10',			"B10 (31 x 44 mm)",									310,	440		),
		(	'C5E',			"C5E (163 x 229 mm)",								1630,	2290	),
		(	'COMM10E',		"U.S. Common 10 Envelope (105 x 241 mm)",			1050,	2410	),
		(	'DLE',			"DL Envelope (110 x 220 mm)",						1100,	2200	),
		(	'EXECUTIVE',	"Executive (7,5 x 10 in)",							1905,	2540	),
		(	'FOLIO',		"Folio (210 x 330 mm)",								2100,	3300	),
		(	'LEDGER',		"Ledger (17 x 11 in)",								4318,	2794	),
		(	'LEGAL',		"Legal (8,5 x 14 in)",								2159,	3556	),
		(	'LETTER',		"Letter (8,5 x 11 in)",								2159,	2794	),
		(	'TABLOID',		"Tabloid (11 x 17 in)",								2794,	4318	),
		(	'CUSTOM',		"Custom paper",										2100,	2970	),
	)

	def __init__(self, description=None, size=None, type=A4):
		size = Vector.ensure(size)
		if (description is None) and (size is None):
			if type is None:
				raise ValueError('unspecified paper type')
			self.name, self.description, w, h = Paper.INFO[type]
			self.size = Vector(w, h)
			self.type = type
		elif size is not None:
			for type, info in enumerate(Paper.INFO[:-1]):
				name, desc, w, h = info
				if size == Vector(w, h):
					self.name = name
					self.description = desc
					self.size = size
					self.type = type
					return
			self.name, desc, w, h = Paper.INFO[-1]
			if description is None:
				self.description = desc
			else:
				self.description = description
			self.size = size
			self.type = Paper.CUSTOM
		else:
			raise 'insufficient specifiers to build Paper object'
	
	def __str__(self):
		return '%.2fx%.2f cm "%s"' % (self.size.w / 100.0, self.size.h / 100.0, self.description)
	
	def __copy__(self):
		return Paper(self.description, self.size, self.type)
	
	def copy(self):
		return self.__copy__()
	
	@classmethod
	def common_list(cls):
		list = []
		for type in xrange(0, len(cls.INFO)):
			list.append(Paper(type=type))
		return list



class PrinterSettings(object):
	#defs{SL_PRINTER_
	BIN_DEFAULT				= 0
	BIN_ONLY_ONE			= 1
	BIN_LOWER				= 2
	BIN_MIDDLE				= 3
	BIN_MANUAL				= 4
	BIN_ENVELOPE			= 5
	BIN_ENVELOPE_MANUAL		= 6
	BIN_TRACTOR				= 7
	BIN_SMALL_FORMAT		= 8
	BIN_LARGE_FORMAT		= 9
	BIN_LARGE_CAPACITY		= 10
	BIN_CASSETTE			= 11
	BIN_FORM_SOURCE			= 12
	
	DUPLEX_DEFAULT			= 0
	DUPLEX_SIMPLEX			= 1
	DUPLEX_HORIZONTAL		= 2
	DUPLEX_VERTICAL			= 3
	
	ORIENTATION_HORIZONTAL	= 0
	ORIENTATION_VERTICAL	= 1
	
	QUALITY_DRAFT			= 0
	QUALITY_LOW				= 1
	QUALITY_MEDIUM			= 2
	QUALITY_HIGH			= 3
	#}
	
	def __init__(self, bin=BIN_DEFAULT, color=True, duplex=DUPLEX_SIMPLEX, orientation=ORIENTATION_VERTICAL, paper=None, name=None, quality=QUALITY_HIGH, collate=True, margin_top=None, margin_right=None, margin_bottom=None, margin_left=None, creator=None):
		self.bin = bin
		self.color = color
		self.duplex = duplex
		self.orientation = orientation
		self.paper = paper or Paper()
		self.name = name or ''
		self.quality = quality
		self.collate = collate
		self.margin_top = margin_top
		self.margin_right = margin_right
		self.margin_bottom = margin_bottom
		self.margin_left = margin_left
		self.creator = creator or ('Slew %s' % version)
	
	def __copy__(self):
		s = PrinterSettings(self.bin, self.color, self.duplex, self.orientation, None, self.name, self.quality, self.collate, self.margin_top, self.margin_right, self.margin_bottom, self.margin_left, self.creator)
		s.paper = self.paper.copy()
		return s
	
	def copy(self):
		return self.__copy__()



def _serialize_data(object):
	return pickle.dumps(object, -1)



def _unserialize_data(object):
	return pickle.loads(object)



def get_application():
	return sApplication



def set_application_name(name):
	_slew.set_application_name(name)



def set_application_flags(flags):
	_slew.set_application_flags(flags)



def run(application):
	if not isinstance(application, Application):
		raise TypeError('expected Application object')
	global sApplication
	sApplication = application
	if application.NAME is not None:
		_slew.set_application_name(application.NAME)
	if application.init() is not False:
		if application.run() is not False:
			_slew.run(application)
		application.exit()



def exit():
	_slew.exit()



def load_resource(resource):
	global sArchiveDict
	if sArchiveDict is None:
		sArchiveDict = {}
		try:
			node = parse_xml(_slew.load_resource('index.xml'))
			if node.tag != 'index':
				raise ValueError('bad archive index')
			for child in node.getchildren():
				if child.tag == 'resource':
					sArchiveDict[child.attrib['name']] = child.text.strip()
		except IOError:
			pass
	if resource.startswith("resource:"):
		resource = resource[9:]
	resource = resource.strip()
	if resource in sArchiveDict:
		resource = sArchiveDict[resource]
	try:
# 		print "attempting to load resource:", resource
		return _slew.load_resource(resource)
	except IOError, e:
		try:
			with open(os.path.join(get_path(RESOURCE_PATH), os.path.normpath(resource)), 'rb') as file:
				data = file.read()
		except:
			raise IOError(u'%s (%s)' % (unicode(e), resource))
		return data



def load_interface(resource):
	node = parse_xml(load_resource(resource))
	if node.tag != 'interface':
		raise ValueError('invalid interface file')
	defs = [ WidgetDef(x) for x in node.getchildren() ]
	return Interface(defs)



def message_box(message, title='', buttons=BUTTON_OK, icon=ICON_INFORMATION, callback=None, userdata=None):
	return _slew.message_box(message, title, buttons, icon, callback, userdata)



def set_shortcut(sequence, action):
	_slew.set_shortcut(sequence, action)



def page_setup(settings, parent=None):
	return _slew.page_setup(settings, parent)



def print_setup(settings, parent=None):
	return _slew.print_setup(settings, parent)



def print_document(type, name, callback, prompt=True, settings=None, parent=None):
	return _slew.print_document(type, name, callback, prompt, settings, parent)



def set_locale(lang):
	_slew.set_locale(lang)



def get_locale_info(lang='it'):
	return _slew.get_locale_info(lang)



def get_computer_info():
	return _slew.get_computer_info()



def get_path(type):
	return _slew.get_path(type)



def get_available_desktop_rect():
	return _slew.get_available_desktop_rect()



def get_fonts_list():
	return _slew.get_fonts_list()



def run_color_dialog(color, title='Select color'):
	return _slew.run_color_dialog(Color.ensure(color), title)



def run_font_dialog(font, title='Select font'):
	return _slew.run_font_dialog(Font.ensure(font), title)



def normalize_format(format, vars):
	return _slew.normalize_format(format, vars)



def format_datatype(datatype, format, value):
	return _slew.format_datatype(datatype, format, value)



def open_uri(uri):
	return _slew.open_uri(uri)



def get_clipboard_data(mimetype=''):
	return _slew.get_clipboard_data(mimetype)



def has_clipboard_data(mimetype=''):
	return _slew.has_clipboard_data(mimetype)



def set_clipboard_data(data, mimetype=''):
	return _slew.set_clipboard_data(data, mimetype)



def add_clipboard_data(data, mimetype=''):
	return _slew.add_clipboard_data(data, mimetype)



def call_later(func, *args):
	call_later_timeout(0, func, *args)



def call_later_timeout(timeout, func, *args):
	_slew.call_later_timeout(timeout, func, *args)



def get_mouse_buttons():
	return _slew.get_mouse_buttons()



def get_mouse_pos():
	return _slew.get_mouse_pos()



def get_keyboard_modifiers():
	return _slew.get_keyboard_modifiers()



def get_keyboard_repeat_rate():
	return _slew.get_keyboard_repeat_rate()



def set_keyboard_repeat_rate(rate):
	_slew.set_keyboard_repeat_rate(rate)



def process_events():
	_slew.process_events()



def flush_events():
	_slew.flush_events()



def get_standard_bitmap(bitmap, size):
	return _slew.get_standard_bitmap(bitmap, Vector.ensure(size))



def get_screen_dpi():
	return _slew.get_screen_dpi()



def get_screen_bitmap():
	return _slew.get_screen_bitmap()



def find_focus():
	return _slew.find_focus()



def beep():
	_slew.beep()



def open_file(message=None, specs='*.*', path='', multi=False):
	if message is None:
		message = translate('Open file')
	return _slew.open_file(message, specs, path, multi)



def save_file(message=None, spec=None, path=''):
	if message is None:
		message = translate('Save file')
	if spec is None:
		spec = ('*', translate('All files'))
	return _slew.save_file(message, spec, path)



def choose_directory(message=None, path=''):
	if message is None:
		message = translate('Choose directory')
	return _slew.choose_directory(message, path)



def set_translations_dict(d):
	global sTranslationsDict
	sTranslationsDict = d.copy()



def translate(text, lang='it'):
	return sTranslationsDict.get(text, text)



@deprecated
def get_default_archive():
	pass



@deprecated
def set_default_archive(archive):
	pass



@deprecated
def load_translations(resource):
	pass



@deprecated
def set_default_translation(dict, lang=None):
	pass



def get_backend_info():
	return _slew.get_backend_info()



def install_exception_handler():
	def excepthook(exc_type, exc_value, exc_traceback):
		import traceback
		message = '\n'.join(traceback.format_exception(exc_type, exc_value, exc_traceback, 20))
		sys.__excepthook__(exc_type, exc_value, exc_traceback)
		_slew.report_exception(message)
	sys.excepthook = excepthook



from widget import Widget, Event, EventHandler, chain_debug_handler
from window import Window
from datacontainer import DataItem, ListDataItem, ListDataModel

from frame import Frame
from dialog import Dialog
from popupwindow import PopupWindow
from menubar import MenuBar
from menu import Menu
from menuitem import MenuItem
from statusbar import StatusBar
from toolbar import ToolBar
from toolbaritem import ToolBarItem
from sizer import Sizer, HBox, VBox
from sizeritem import SizerItem
from panel import Panel
from foldpanel import FoldPanel
from scrollview import ScrollView
from splitview import SplitView
from tabview import TabView
from tabviewpage import TabViewPage
from stackview import StackView
from label import Label
from hyperlink import Hyperlink
from button import Button
from toolbutton import ToolButton
from groupbox import GroupBox
from checkbox import CheckBox
from radio import Radio
from line import Line
from image import Image
from slider import Slider
from spinfield import SpinField
from progress import Progress
from textfield import TextField
from textview import TextView
from searchfield import SearchField
from calendar import Calendar
from combobox import ComboBox
from listbox import ListBox
from listview import ListView
from iconview import IconView
from grid import Grid
from treeview import TreeView
from sceneview import SceneView, SceneItem
from systrayicon import SystrayIcon
from wizard import Wizard
from wizardpage import WizardPage
from webview import WebView


import _slew
