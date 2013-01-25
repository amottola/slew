import slew
import math

from utils import *
from numbers import Number



class Vector(object):
	def __init__(self, x=0.0, y=0.0):
		self.x = float(x)
		self.y = float(y)
	
	class XDescriptor(object):
		def __get__(self, instance, owner):
			return instance.x
		def __set__(self, instance, value):
			instance.x = value
	
	class YDescriptor(object):
		def __get__(self, instance, owner):
			return instance.y
		def __set__(self, instance, value):
			instance.y = value
	
	w = XDescriptor()
	h = YDescriptor()
	
	column = XDescriptor()
	row = YDescriptor()
	
	def format(self, precision=2):
		format = '(%%.%df, %%.%df)' % (precision, precision)
		return format % (self.x, self.y)
	
	def __str__(self):
		return '(%f,%f)' % (self.x, self.y)
	
	def __repr__(self):
		return 'Vector%s' % str(self)
	
	def __getitem__(self, index):
		if index == 0:
			return self.x
		elif index == 1:
			return self.y
		else:
			raise IndexError("invalid index to Vector")
	
	def __setitem__(self, index, value):
		if index == 0:
			self.x = value
		elif index == 1:
			self.y = value
		else:
			raise IndexError("invalid index to Vector")
	
	def __nonzero__(self):
		return (self.x != 0.0) or (self.y != 0.0)
	
	def __len__(self):
		if (self.x == 0.0) and (self.y == 0.0):
			return 0
		else:
			return 2
	
	def __cmp__(self, other):
		if isinstance(other, Vector):
			if (self.x < other.x) and (self.y < other.y):
				return -1
			elif (self.x == other.x) and (self.y == other.y):
				return 0
		else:
			if (self.x < other[0]) and (self.y < other[1]):
				return -1
			elif (self.x == other[0]) and (self.y == other[1]):
				return 0
		return 1
	
	def __add__(self, other):
		if isinstance(other, Vector):
			return Vector(self.x + other.x, self.y + other.y)
		else:
			return Vector(self.x + other[0], self.y + other[1])
	
	def __iadd__(self, other):
		if isinstance(other, Vector):
			self.x += other.x
			self.y += other.y
		else:
			self.x += other[0]
			self.y += other[1]
		return self
	
	def __sub__(self, other):
		if isinstance(other, Vector):
			return Vector(self.x - other.x, self.y - other.y)
		else:
			return Vector(self.x - other[0], self.y - other[1])
	
	def __isub__(self, other):
		if isinstance(other, Vector):
			self.x -= other.x
			self.y -= other.y
		else:
			self.x -= other[0]
			self.y -= other[1]
		return self
	
	def __mul__(self, other):
		if isinstance(other, Vector):
			return (self.x * other.x) + (self.y * other.y)
		elif isinstance(other, Number):
			return Vector(self.x * other, self.y * other)
		else:
			raise NotImplementedError("Vector __mul__(%s)" % str(other))
	
	def __div__(self, other):
		if isinstance(other, Number):
			return Vector(self.x / other, self.y / other)
		else:
			raise NotImplementedError("Vector __div__(%s)" % str(other))
	
	def __neg__(self):
		return Vector(-self.x, -self.y)
	
	def __xor__(self):
		return Vector(self.x * other.y, -self.y * other.x)
	
	def copy(self):
		return Vector(self.x, self.y)
	
	def transposed(self):
		return Vector(self.y, self.x)
	
	def magnitude(self):
		return math.sqrt((self.x * self.x) + (self.y * self.y))
	
	def as_tuple(self):
		return (self.x, self.y)
	
	@classmethod
	def ensure(cls, value, allowNone=True):
		if (value is None) and allowNone:
			return value
		if isinstance(value, basestring):
			if value.startswith('('):
				value = value[1:]
			if value.endswith(')'):
				value = value[:-1]
			value = value.split(',')
		if isinstance(value, Vector):
			return value
		elif not isinstance(value, (tuple, list)):
			raise ValueError("expected 'Vector' object")
		return Vector(value[0], value[1])



class Color(object):
# 	class RGBDescriptor(object):
# 		def __get__(self, instance, owner):
# 			return instance.r | (instance.g << 8) | (instance.b << 16) | (instance.a << 24)
# 		def __set__(self, instance, value):
# 			instance.r = value & 0xFF
# 			instance.g = (value >> 8) & 0xFF
# 			instance.b = (value >> 16) & 0xFF
# 			instance.a = (value >> 24) & 0xFF
# 	rgb = RGBDescriptor()
	
	def __init__(self, r=0, g=0, b=0, a=255, value=None):
		if value is not None:
			if isinstance(value, (tuple, list)):
				if len(value) == 4:
					r, g, b, a = value
				else:
					r, g, b = value
					a = 255
			elif isinstance(value, basestring):
				if value[0] == '#':
					value = value[1:]
				value = value.split(',')
				r = g = b = 0
				a = 255
				if len(value) == 1:
					value = value[0]
					if len(value) == 3:
						r = int(value[0], 16)
						r |= (r << 4)
						g = int(value[1], 16)
						g |= (g << 4)
						b = int(value[2], 16)
						b |= (b << 4)
					elif len(value) == 4:
						r = int(value[0], 16)
						r |= (r << 4)
						g = int(value[1], 16)
						g |= (g << 4)
						b = int(value[2], 16)
						b |= (b << 4)
						a = int(value[2], 16)
						a |= (a << 4)
					elif len(value) == 6:
						r = int(value[0:2], 16)
						g = int(value[2:4], 16)
						b = int(value[4:6], 16)
					elif len(value) == 8:
						r = int(value[0:2], 16)
						g = int(value[2:4], 16)
						b = int(value[4:6], 16)
						a = int(value[6:8], 16)
					else:
						raise ValueError('invalid color representation')
				else:
					if len(value) == 3:
						r, g, b = [ int(x) for x in value ]
					elif len(value) == 4:
						r, g, b, a = [ int(x) for x in value ]
					else:
						raise ValueError('invalid color representation')
			else:
				r = value & 0xFF
				g = (value >> 8) & 0xFF
				b = (value >> 16) & 0xFF
				a = (value >> 24) & 0xFF
		self.r = r
		self.g = g
		self.b = b
		self.a = a
	
	def gray(self, intensity=255):
		hue = (((self.r * 11) + (self.g * 16) + (self.b * 5)) * intensity) / 8160
		return Color(hue, hue, hue, self.a)
	
	def bright(self, factor=1.0):
		return Color(min(max(0, int(self.r * factor)), 255), min(max(0, int(self.g * factor)), 255), min(max(0, int(self.b * factor)), 255), self.a)
	
	def __str__(self):
		return "#%02X%02X%02X" % (self.r, self.g, self.b)
	
	def __repr__(self):
		return "(%d,%d,%d,%d)" % (self.r, self.g, self.b, self.a)
	
	def __hash__(self):
		return hash(self.r | (self.g << 8) | (self.b << 16) | (self.a << 24))
	
	def __cmp__(self, other):
		if isinstance(other, Color) and (other.r == self.r) and (other.g == self.g) and (other.b == self.b) and (other.a == self.a):
			return 0
		return -1
	
	def copy(self):
		return Color(self.r, self.g, self.b, self.a)
	
	@classmethod
	def ensure(cls, value, allowNone=True):
		if value is None:
			if allowNone:
				return value
			raise ValueError('expected Color object')
		if isinstance(value, Color):
			return value
		return Color(value=value)



class Font(object):
	#defs{SL_FONT_
	FAMILY_DEFAULT			= 0
	FAMILY_ROMAN			= 1
	FAMILY_SCRIPT			= 2
	FAMILY_SANS_SERIF		= 3
	FAMILY_FIXED_PITCH		= 4
	FAMILY_TELETYPE			= 5
	
	SIZE_DEFAULT = -1
	
	STYLE_NORMAL			= 0x00000000
	STYLE_BOLD				= 0x00000001
	STYLE_ITALIC			= 0x00000002
	STYLE_UNDERLINED		= 0x00000004
	STYLE_NO_KERNING		= 0x00000008
	#}
	

	FAMILIES = [
		"default",
		"roman",
		"script",
		"sanserif",
		"fixed",
		"teletype"
	]
	
	STYLES = [ "bold", "italic", "underlined", "nokerning" ]
	
	TEMP_DC = None
	
	def __init__(self, family=0, face="", size=SIZE_DEFAULT, style=0, spacing=0, string=None):
		self.family = family
		self.face = face
		self.size = size
		self.style = style
		self.spacing = spacing
		if string is not None:
			parts = string.split(',')
			for p in parts:
				key, value = map(lambda x: x.strip(), p.split(':'))
				if key == 'family':
					self.family = Font.FAMILIES.index(value)
				elif key == 'face':
					self.face = value
				elif key == 'size':
					self.size = int(value)
				elif key == 'style':
					self.style = parse_bit_flags(value, Font.STYLES)
				elif key == 'spacing':
					self.spacing = int(value)
	
	def __str__(self):
		s = "family:" + Font.FAMILIES[self.family]
		if self.face:
			s += ",face:" + self.face
		s += ",size:%d" % self.size
		if self.style:
			s += ",style:%s" % build_bit_flags(self.style, Font.STYLES)
		if self.spacing:
			s += ",spacing:%d" % self.spacing
		return s
	
	def __hash__(self):
		return hash((self.family, self.face, self.size, self.style, self.spacing))
	
	def __cmp__(self, other):
		if isinstance(other, Font) and (other.family == self.family) and (other.face == self.face) and (other.size == self.size) and (other.style == self.style) and (other.spacing == self.spacing):
			return 0
		return -1
	
	def copy(self):
		return Font(self.family, self.face, self.size, self.style, self.spacing)
	
	def text_extent(self, text, max_width=None):
		if Font.TEMP_DC is None:
			Font.TEMP_DC = slew.Bitmap(size=(1,1))
		Font.TEMP_DC.set_font(self)
		return Font.TEMP_DC.text_extent(text, max_width)
	
	@classmethod
	def ensure(cls, value, **kwargs):
		if isinstance(value, (type(None), Font)):
			return value
		return Font(string=value, **kwargs)



class DC(object):
	#defs{SL_DC_
	TEXT_ELIDE_LEFT				= 0x100
	TEXT_ELIDE_CENTER			= 0x200
	TEXT_ELIDE_RIGHT			= 0x400
	
	PEN_STYLE_SOLID				= 0
	PEN_STYLE_DASH				= 1
	PEN_STYLE_DOT				= 2
	PEN_STYLE_DASH_DOT			= 3
	PEN_STYLE_DASH_DOT_DOT		= 4
	
	BRUSH_STYLE_SOLID			= 0
	BRUSH_STYLE_DENSE_90		= 1
	BRUSH_STYLE_DENSE_80		= 2
	BRUSH_STYLE_DENSE_70		= 3
	BRUSH_STYLE_DENSE_60		= 4
	BRUSH_STYLE_DENSE_50		= 5
	BRUSH_STYLE_DENSE_40		= 6
	BRUSH_STYLE_DENSE_30		= 7
	BRUSH_STYLE_DENSE_20		= 8
	BRUSH_STYLE_DENSE_10		= 9
	BRUSH_STYLE_H_LINES			= 10
	BRUSH_STYLE_V_LINES			= 11
	BRUSH_STYLE_CROSS_LINES		= 12
	BRUSH_STYLE_BD_LINES		= 13
	BRUSH_STYLE_FD_LINES		= 14
	BRUSH_STYLE_D_CROSS_LINES	= 15
	#}

	def get_color(self):
		return self._impl.get_color()
	
	def set_color(self, color):
		self._impl.set_color(Color.ensure(color))
	
	def get_bgcolor(self):
		return self._impl.get_bgcolor()
	
	def set_bgcolor(self, color):
		self._impl.set_bgcolor(Color.ensure(color))
	
	def get_font(self):
		return self._impl.get_font()
	
	def set_font(self, font):
		self._impl.set_font(Font.ensure(font))
	
	def get_size(self):
		return self._impl.get_size()
	
	def set_size(self, size):
		self._impl.set_size(Vector.ensure(size, False))
	
	def get_pensize(self):
		return self._impl.get_pensize()
	
	def set_pensize(self, size):
		self._impl.set_pensize(size)
	
	def get_penstyle(self):
		return self._impl.get_penstyle()
	
	def set_penstyle(self, style):
		self._impl.set_penstyle(style)
	
	def get_brushstyle(self):
		return self._impl.get_brushstyle()
	
	def set_brushstyle(self, style):
		self._impl.set_brushstyle(style)
	
	def clear(self):
		self._impl.clear()
	
	def point(self, pos):
		self._impl.point(Vector.ensure(pos, False))
	
	def get_point(self, pos):
		return self._impl.get_point(Vector.ensure(pos, False))
	
	def line(self, start, end):
		self._impl.line(Vector.ensure(start, False), Vector.ensure(end, False))
	
	def rect(self, tl, br):
		self._impl.rect(Vector.ensure(tl, False), Vector.ensure(br, False))
	
	def rounded_rect(self, tl, br, xr=0, yr=0):
		self._impl.rounded_rect(Vector.ensure(tl, False), Vector.ensure(br, False), xr, yr)
	
	def circle(self, pos, r):
		self._impl.ellipse(Vector.ensure(pos, False), r, r)
	
	def ellipse(self, pos, a, b):
		self._impl.ellipse(Vector.ensure(pos, False), a, b)
	
	def text(self, text, tl, br=None, flags=0):
		if br is None:
			flags = -1
		self._impl.text(text, Vector.ensure(tl, False), Vector.ensure(br), flags)
		
	def text_extent(self, text, max_width=None):
		return self._impl.text_extent(text, int(max_width or -1))

	def blit(self, dc, pos, size=None, source_pos=None, source_size=None, repeat=False):
		dc._impl.blit(self, Vector.ensure(pos, False), Vector.ensure(size), Vector.ensure(source_pos), Vector.ensure(source_size), repeat)
	
	def set_clipping(self, tl, br=None):
		self._impl.set_clipping(Vector.ensure(tl), Vector.ensure(br))
	
	def set_antialias(self, enabled):
		self._impl.set_antialias(enabled)
	
	def get_transform(self):
		return self._impl.get_transform()
	
	def set_transform(self, matrix, replace=True):
		if not replace:
			m = self._impl.get_transform()
			def calc(y, x):
				return m[y][0] * matrix[0][x] + m[y][1] * matrix[1][x] + m[y][2] * matrix[2][x]
			matrix = (
				( calc(0,0), calc(0,1), calc(0,2) ),
				( calc(1,0), calc(1,1), calc(1,2) ),
				( calc(2,0), calc(2,1), calc(2,2) )
			)
		self._impl.set_transform(matrix)
	
	def translate(self, dx, dy):
		matrix = (
			( 1.0, 0.0, 0.0 ),
			( 0.0, 1.0, 0.0 ),
			( dx, dy, 1.0 )
		)
		self.set_transform(matrix, False)
	
	def scale(self, dx, dy=None):
		if dy is None:
			dy = dx
		matrix = (
			( dx, 0.0, 0.0 ),
			( 0.0, dy, 0.0 ),
			( 0.0, 0.0, 1.0 )
		)
		self.set_transform(matrix, False)
	
	def rotate(self, angle):
		angle = (3.14159236 * angle) / 180.0
		cos = math.cos(angle)
		sin = math.sin(angle)
		matrix = (
			( cos, sin, 0.0 ),
			( -sin, cos, 0.0 ),
			( 0.0, 0.0, 1.0 )
		)
		self.set_transform(matrix, False)
	
	color = DeprecatedDescriptor('color')
	bgcolor = DeprecatedDescriptor('bgcolor')
	font = DeprecatedDescriptor('font')
	size = DeprecatedDescriptor('size')
	pensize = DeprecatedDescriptor('pensize')
	penstyle = DeprecatedDescriptor('penstyle')
	brushstyle = DeprecatedDescriptor('brushstyle')



class PrintDC(DC):
	def get_dpi(self):
		return self._impl.get_dpi()
	
	def set_dpi(self, dpi):
		self._impl.set_dpi(Vector.ensure(dpi, False))
	
	def get_page_rect(self):
		return self._impl.get_page_rect()
	
	def get_printer_dpi(self):
		return self._impl.get_printer_dpi()
	
	dpi = DeprecatedDescriptor('dpi')



class Bitmap(DC):
	def __init__(self, data=None, resource=None, size=None, bits=None):
		self._impl = slew._slew.Bitmap(Vector.ensure(size) or Vector(32,32))
		if resource is not None:
			data = slew.load_resource(resource)
		if data is not None:
			self.load(data)
		elif bits is not None:
			self.set_bits(bits)
	
	def get_bits(self):
		return self._impl.get_bits()
	
	def set_bits(self, bits):
		self._impl.set_bits(bits)
	
	def resized(self, size, smooth=True):
		return self._impl.resized(Vector.ensure(size, False), smooth)
	
	def colorized(self, color):
		return self._impl.colorized(Color.ensure(color, False))
	
	def load(self, data):
		self._impl.load(data)
	
	def save(self):
		return self._impl.save()
	
	def copy(self):
		return self._impl.copy()
	
	@deprecated
	def resize(self, size):
		return self.resized(size)
		
	@deprecated
	def bits(self):
		return self.get_bits()
	
	@deprecated
	def set_mask(self, color):
		pass
	
	@classmethod
	def ensure(cls, bitmap, allowNone=True):
		if (bitmap is None) and allowNone:
			return bitmap
		if not isinstance(bitmap, Bitmap):
			raise TypeError("expected Bitmap object")
		return bitmap



class Icon(object):
	def __init__(self, normal, disabled=None, active=None, selected=None):
		self.normal = Bitmap.ensure(normal, False)
		self.disabled = Bitmap.ensure(disabled)
		self.active = Bitmap.ensure(active)
		self.selected = Bitmap.ensure(selected)
	
	def copy(self):
		normal = self.normal.copy()
		disabled = None if self.disabled is None else self.disabled.copy()
		active = None if self.active is None else self.active.copy()
		selected = None if self.selected is None else self.selected.copy()
		return Icon(normal, disabled, active, selected)
	
	@classmethod
	def ensure(cls, icon, allowNone=True):
		if (icon is None) and allowNone:
			return icon
		elif isinstance(icon, Bitmap):
			icon = Icon(icon)
		if not isinstance(icon, Icon):
			raise TypeError("expected Icon object")
		return icon

