import math, time

from xml.etree import ElementTree as ET
from datetime import datetime, date


import slew


def _patch_etree():
	OriginalXMLTreeBuilder = ET.XMLTreeBuilder
	OriginalElementTree = ET.ElementTree
	Original_serialize_xml = ET._serialize_xml
	
	class PatchedXMLTreeBuilder(ET.XMLTreeBuilder):
		def __init__(self, html=0, target=None, parse_comments=False):
			OriginalXMLTreeBuilder.__init__(self, html, target)
			self._parser.StartCdataSectionHandler = self._start_cdata
			self._parser.EndCdataSectionHandler = self._end_cdata
			if parse_comments:
				self._parser.CommentHandler = self._handle_comment
			self._parser.ProcessingInstructionHandler = self._handle_pi
			self._cdataSection = False
			self._cdataBuffer = None
		
		def _start(self, tag, attrib_in):
			elem = OriginalXMLTreeBuilder._start(self, tag, attrib_in)
			elem.line = self._parser.CurrentLineNumber
			elem.column = self._parser.CurrentColumnNumber
			return elem
		
		def _start_list(self, tag, attrib_in):
			elem = OriginalXMLTreeBuilder._start_list(self, tag, attrib_in)
			elem.line = self._parser.CurrentLineNumber
			elem.column = self._parser.CurrentColumnNumber
			return elem
			
		def _start_cdata(self):
			"""
			A CDATA section beginning has been recognized - start collecting
			character data.
			"""
			self._cdataSection = True
			self._cdataBuffer = []
			
		def _end_cdata(self):
			"""
			The CDATA section has ended - join the character data we collected
			and add a CDATA element to the target tree.
			"""
			self._cdataSection = False
			text = self._fixtext("".join(self._cdataBuffer))
			elem = self._target.start(None, {})
			elem._is_cdata = True
			self._target.data(text)
			self._target.end(None)
			elem.text = text
	# 		print "created cdata with content:\n---\n%s\n---" % text
			
		def _handle_comment(self, data):
			self._target.start(ET.Comment, {})
			self._target.data(data)
			self._target.end(ET.Comment)
		
		def _handle_pi(self, target, data):
			self._target.start(ET.PI, {})
			self._target.data(target + " " + data)
			self._target.end(ET.PI)
		
		def _data(self, text):
			"""
			If we are in the middle of a CDATA section, collect the data into a
			special buffer, otherwise treat it as before.
			"""
			if self._cdataSection:
				self._cdataBuffer.append(text)
			else:
				OriginalXMLTreeBuilder._data(self, text)
	
	def Patched_serialize_xml(write, elem, encoding, qnames, namespaces):
		if getattr(elem, '_is_cdata', False):
			if elem.text:
				text = elem.text.encode(encoding)
				write("<![CDATA[%s]]>" % text)
		else:
			Original_serialize_xml(write, elem, encoding, qnames, namespaces)
	
	def CDATA(text=None):
		element = ET.Element(None)
		element.text = text
		element._is_cdata = True
		return element
	
	ET.XMLTreeBuilder = ET.XMLParser = PatchedXMLTreeBuilder
	ET._serialize_xml = Patched_serialize_xml
	ET.CDATA = CDATA

try:
	from xml.etree.ElementTree import CDATA
except:
	_patch_etree()



def deprecated(func):
	import inspect
	
	def wrapper(*args, **kwargs):
# 		frame = inspect.stack()[1]
# 		print "call to deprecated function '%s' in %s, line %d" % (func.__name__, frame[1], frame[2]) 
		return func(*args, **kwargs)
	wrapper.__doc__ = func.__doc__
	wrapper.__name__ = func.__name__
	return wrapper



class DeprecatedDescriptor(object):
	def __init__(self, name):
		self.name = name
	def __get__(self, instance, owner):
# 		import inspect
# 		frame = inspect.stack()[1]
# 		print "call to deprecated descriptor '%s' in %s, line %d" % (self.name, frame[1], frame[2])
		return getattr(instance, 'get_' + self.name)()
	def __set__(self, instance, value):
# 		import inspect
# 		frame = inspect.stack()[1]
# 		print "call to deprecated descriptor '%s' in %s, line %d" % (self.name, frame[1], frame[2])
		getattr(instance, 'set_' + self.name)(value)



class DeprecatedBoolDescriptor(DeprecatedDescriptor):
	def __get__(self, instance, owner):
# 		import inspect
# 		frame = inspect.stack()[1]
# 		print "call to deprecated descriptor '%s' in %s, line %d" % (self.name, frame[1], frame[2])
		return getattr(instance, 'is_' + self.name)()



sWidgetFactory = {}



def merge(a, b):
	c = a.copy()
	c.update(b)
	return c



def parse_xml(xml):
	document = ET.ElementTree()
	if isinstance(xml, unicode):
		xml = xml.encode('utf-8')
	if isinstance(xml, (basestring, buffer)):
		from io import BytesIO
		xml = BytesIO(bytes(xml))
	document.parse(xml, ET.XMLTreeBuilder())
	return document.getroot()



def parse_bit_flags(string, names):
	bits = 0
	parts = string.split('|')
	for part in parts:
		spart = part.strip()
		if spart:
			if isinstance(names, dict):
				bits |= names[spart]
			else:
				bits |= (1 << int(names.index(spart)))
	return bits



def build_bit_flags(bits, names):
	result = ""
	i = 0
	while bits:
		if bits & 1:
			if len(result) > 0:
				result += '|'
			result += names[i]
		i += 1
		bits >>= 1
	return result



def profile(command, globals, locals):
	import cProfile, pstats, tempfile, os
	
	filename = tempfile.mktemp()
	result = cProfile.runctx(command, globals, locals, filename)
	stats = pstats.Stats(filename)
	stats.strip_dirs()
	stats.sort_stats('time', 'calls')
	stats.print_stats(20)
	stats.print_callers(20)
	os.unlink(filename)



def backtrace():
	import inspect
	
	print "backtrace:"
	for frame in inspect.stack()[1:]:
		print "\tFile '%s', line %d, function '%s'" % (frame[1], frame[2], frame[3])



def factory(cls):
	sWidgetFactory[cls.NAME] = cls
	setattr(slew, cls.__name__, cls)
	cls.__slew_class_name__ = cls.__name__
	return cls
	


def get_factory(name):
	if name in sWidgetFactory:
		return sWidgetFactory[name]
	raise RuntimeError("unknown widget type '%s'" % name)


def get_factory_relaxed(name):
	return sWidgetFactory.get(name, None)



from gdi import *



class Property(object):
	def set(self, instance, name, value):
		getattr(instance, 'set_' + name)(value)
	def load(self, instance, name, node, globals, locals):
		pass
	def copy(self, source, dest, name):
		getattr(dest, 'set_' + name)(getattr(source, 'get_' + name)())
		


class EventProperty(Property):
	def set(self, instance, name, value):
		setattr(instance, name, value)
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			var = node.attrib[name]
			if not var:
				return
			if (locals is not None) and (var in locals):
				setattr(instance, name, locals[var])
			elif (globals is not None) and (var in globals):
				setattr(instance, name, globals[var])
			else:
				raise NameError("name '%s' is not defined" % var)
	def copy(self, source, dest, name):
		setattr(dest, name, getattr(source, name))



class HandlerProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			var = node.attrib[name]
			if (locals is not None) and (var in locals):
				instance.set_handler(locals[var])
			elif (globals is not None) and (var in globals):
				instance.set_handler(globals[var])
			else:
				raise NameError("name '%s' is not defined" % var)



class StringProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(node.attrib[name])



class TranslatedStringProperty(Property):
	def __init__(self, node_text=False):
		self.node_text = node_text
	def load(self, instance, name, node, globals, locals):
		if self.node_text:
			text = node.text
		else:
			text = node.attrib.get(name, None)
		if text is not None:
			getattr(instance, 'set_' + name)(slew.translate(text))



class ChoiceProperty(Property):
	def __init__(self, list):
		self.list = list
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			if isinstance(self.list, dict):
				value = self.list[node.attrib[name]]
			else:
				value = self.list.index(node.attrib[name])
			getattr(instance, 'set_' + name)(value)

	

class BitsProperty(Property):
	def __init__(self, list):
		self.list = list
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(parse_bit_flags(node.attrib[name], self.list))



class VectorProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			x, y = node.attrib[name].split(',')
			getattr(instance, 'set_' + name)(Vector(x, y))



class BoolProperty(Property):
	TRUE = {
		't':	True,
		'true':	True,
		'1':	True,
		'on':	True,
	}
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(self.is_true(node.attrib[name]))
	def copy(self, source, dest, name):
		getattr(dest, 'set_' + name)(getattr(source, 'is_' + name)())
	
	@classmethod
	def is_true(cls, value):
		return value.lower() in cls.TRUE



class ColorProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(Color(value=node.attrib[name]))



class FontProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(Font(string=node.attrib[name]))



class IntProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(int(node.attrib[name]))



class IntListProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(node.attrib[name].split(','))



class FloatProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(float(node.attrib[name]))



class BitmapProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(Bitmap(data=slew.load_resource(node.attrib[name])))



class IconProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			pixmap = {}
			for part in node.attrib[name].split(','):
				desc = part.split(':')
				if len(desc) == 1:
					pixmap['normal'] = Bitmap(data=slew.load_resource(desc[0].strip()))
				else:
					type, res = desc
					pixmap[type.strip()] = Bitmap(data=slew.load_resource(res.strip()))
			getattr(instance, 'set_' + name)(Icon(pixmap.get('normal'), pixmap.get('disabled'), pixmap.get('active'), pixmap.get('selected')))



class DateProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			ts = time.strptime(node.attrib[name], '%Y-%m-%d')
			getattr(instance, 'set_' + name)(date(*(ts[0:3])))



class DateTimeProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			ts = time.strptime(node.attrib[name], '%Y-%m-%d %H:%M:%S')
			getattr(instance, 'set_' + name)(datetime(*(ts[0:6])))



class ExtraProperty(Property):
	def load(self, instance, name, node, globals, locals):
		if name in node.attrib:
			getattr(instance, 'set_' + name)(eval(node.attrib[name]))


