import inspect
import slew


from utils import *



class Event(object):
	#defs{SL_EVENT_
	DRAG_ACTION_NONE				= 0x0
	DRAG_ACTION_MOVE				= 0x1
	DRAG_ACTION_COPY				= 0x2
	DRAG_ACTION_CUSTOMIZABLE		= 0x4
	DRAG_ON_ITEM					= 0
	DRAG_ABOVE_ITEM					= 1
	DRAG_BELOW_ITEM					= 2
	DRAG_ON_VIEWPORT				= 3
	#}
	
	def __init__(self, **kwargs):
		for key, value in kwargs.iteritems():
			setattr(self, key, value)
	
	def __repr__(self):
		return repr(self.__dict__)
	
	def __str__(self):
		return str(self.__dict__)



class EventHandler(object):
	PROPERTIES = {
		'handler':	HandlerProperty(),
	}
	
	def __new__(cls, *args, **kwargs):
		self = object.__new__(cls)
		self.__handler = None
		return self
	
	def get_handler(self):
		return self.__handler
	
	def set_handler(self, handler):
		self.__handler = handler
	
	def onTimer(self, e):				pass
	def onPaint(self, e):				pass
	def onFocusIn(self, e):				pass
	def onFocusOut(self, e):			pass
	def onChar(self, e):				pass
	def onPaint(self, e):				pass
	def onPaintView(self, e):			pass
	def onKeyDown(self, e):				pass
	def onKeyUp(self, e):				pass
	def onMouseDown(self, e):			pass
	def onMouseUp(self, e):				pass
	def onDblClick(self, e):			pass
	def onMouseMove(self, e):			pass
	def onMouseEnter(self, e):			pass
	def onMouseLeave(self, e):			pass
	def onMouseWheel(self, e):			pass
	def onDragStart(self, e):			return False
	def onDragMove(self, e):			return False
	def onDragEnter(self, e):			pass
	def onDragLeave(self, e):			pass
	def onDragEnd(self, e):				pass
	def onDrop(self, e):				pass
	def onResize(self, e):				pass
	def onClose(self, e):				pass
	def onSelect(self, e):				pass
	def onActivate(self, e):			pass
	def onEnter(self, e):				pass
	def onClick(self, e):				pass
	def onCheck(self, e):				pass
	def onChange(self, e):				pass
	def onExpand(self, e):				pass
	def onCollapse(self, e):			pass
	def onSort(self, e):				pass
	def onContextMenu(self, e):			return False
	def onCancel(self, e):				pass
	def onNotify(self, e):				pass
	def onCellEditStart(self, e):		pass
	def onCellEditEnd(self, e):			pass
	def onCellDataChanged(self, e):		pass
	def onModify(self, e):				pass
	def onHeaderColumnMoved(self, e):	pass
	def onHeaderColumnResized(self, e):	pass
	def onLoad(self, e):				pass
	def onPrint(self, e):				pass
	def onPaste(self, e):				return False
	def onCreateObject(self, e):		pass


for name in dir(EventHandler):
	if name.startswith('on'):
		EventHandler.PROPERTIES[name] = EventProperty()



def chain_debug_handler(widget, recursive=False, events=None):
	def do_chain(w, events_list):
		class Handler(EventHandler):
			pass
		handler = w.get_handler()
		if handler is None:
			handler = w
		def makeWatchedProc(name):
			def proc(self, e):
				print '%s on %s (%s): %s' % (name, e.widget.get_name(), e.widget.__class__.__name__, str(e))
				return getattr(handler, name)(e)
			return proc
		
		def makeProc(name):
			def proc(self, e):
				return getattr(handler, name)(e)
			return proc
		
		for name in EventHandler.PROPERTIES:
			if name in events_list:
				setattr(Handler, name, makeWatchedProc(name))
			else:
				setattr(Handler, name, makeProc(name))
		
		w.set_handler(Handler())
		if recursive:
			for child in w.get_children():
				do_chain(child, events_list)
	
	if events is None:
		enabled = EventHandler.PROPERTIES.keys()
	else:
		enabled = []
		if isinstance(events, basestring):
			events = events.split(' ')
		for name in EventHandler.PROPERTIES:
			for event in events:
				event = event.strip()
				if event.endswith('*'):
					if name.startswith(event[:-1]):
						enabled.append(name)
				else:
					if name == event:
						enabled.append(name)
	do_chain(widget, enabled)



class Widget(EventHandler):
	ID				= 1
	NAME			= 'widget'
	AUTO_NAME		= {}

	ALIGN_TOP		= slew.ALIGN_TOP
	ALIGN_BOTTOM	= slew.ALIGN_BOTTOM
	ALIGN_LEFT		= slew.ALIGN_LEFT
	ALIGN_RIGHT		= slew.ALIGN_RIGHT
	ALIGN_HCENTER	= slew.ALIGN_HCENTER
	ALIGN_VCENTER	= slew.ALIGN_VCENTER
	ALIGN_CENTER	= slew.ALIGN_CENTER
	
	
	PROPERTIES = merge(EventHandler.PROPERTIES, {
		'name':		StringProperty(),
		'extra':	ExtraProperty(),
	})
	
	def __new__(cls, *args, **kwargs):
		self = EventHandler.__new__(cls)
		self._impl = self.create_impl(cls.__slew_class_name__)
		self.id = Widget.ID
		Widget.ID += 1
		self.initialize()
		return self
		
	def __init__(self, xml=None, globals=None, locals=None, **kwargs):
		self.__name = ''
		if 'name' not in kwargs:
			class_name = self.__class__.__name__
			if class_name not in Widget.AUTO_NAME:
				Widget.AUTO_NAME[class_name] = 1
			
			name = '%s_%d' % (self.NAME, Widget.AUTO_NAME[class_name])
			if xml is None:
				kwargs['name'] = name
			else:
				self.__name = name
			Widget.AUTO_NAME[class_name] += 1
		self.__extra = None
		
		if xml is not None:
			self.load(xml, globals, locals)
		else:
			for name, prop in self.PROPERTIES.iteritems():
				if name in kwargs:
					prop.set(self, name, kwargs[name])
	
	def create_impl(self, class_name):
		return getattr(slew.get_backend(), class_name)(self)
	
	def initialize(self):
		self.__parent = None
		self.__children = []
	
	def __nonzero__(self):
		return True
	
	def __len__(self):
		return len(self.__children)
	
	def __getitem__(self, index):
		return self.__children[index]
	
	def __setitem__(self, index, widget):
		self.replace(index, widget)
	
	def __delitem__(self, index):
		self.remove(index)
	
	def __contains__(self, widget):
		if isinstance(widget, basestring):
			for child in self.__children:
				if child.__name == widget:
					return True
		elif isinstance(widget, Widget):
			for child in self.__children:
				if child is widget:
					return True
		else:
			raise ValueError('expected str or Widget object')
		return False
	
	def __iter__(self):
		return self.__children.__iter__()
	
	def get_children(self):
		return list(self.__children)
	
	def get_parent(self):
		return self.__parent
	
	def _set_parent(self, parent):
		self.__parent = parent
	
	def is_ancestor_of(self, widget):
		def lookup(parent):
			for child in parent.__children:
				if (child is widget) or lookup(child):
					return True
			return False
		return lookup(self)
	
	def find(self, name, recursive=True, fromroot=True):
		widget = self
		if fromroot:
			parent = widget.__parent
			while parent:
				widget = parent
				parent = widget.__parent
		for child in self.__children:
			if not isinstance(child, Widget):
				continue
			if child.__name == name:
				return child
			if recursive:
				found = child.find(name, True, False)
				if found is not None:
					return found
		return None
	
	def clone(self, name=None):
		cloned = get_factory(self.NAME)()
		for pname, prop in self.PROPERTIES.iteritems():
			prop.copy(self, cloned, pname)
		for child in self.__children:
			cloned.append(child.clone())
		if name is not None:
			cloned.__name = name
		return cloned
	
	def append(self, child):
		return self.insert(-1, child)
	
	def insert(self, index, child):
		count = len(self.__children)
		if index < 0:
			index += count + 1
		if index < 0:
			index = 0
		elif index > count:
			index = count
		parent = child.get_parent()
		if parent is not None:
			if (parent is self) and (self.__children[index] is child):
				return self
			parent.remove(child)
		self._impl.insert(index, child)
		self.__children.insert(index, child)
		child._set_parent(self)
		return self
	
	def remove(self, child):
		if isinstance(child, int):
			count = len(self.__children)
			if child < 0:
				child += count + 1
			if (child < 0) or (child >= count):
				return
			widget = self.__children[child]
			self._impl.remove(widget)
			self.__children.pop(child)
			widget._set_parent(None)
		else:
			if child.get_parent() is self:
				self._impl.remove(child)
				self.__children.remove(child)
				child._set_parent(None)
	
	def replace(self, index, child):
		count = len(self.__children)
		if isinstance(index, basestring):
			for i, widget in enumerate(self.__children):
				if widget.__name == index:
					child.__name = index
					index = i
					break
			else:
				index = -1
		if index < 0:
			index += count + 1
		if index < 0:
			index = 0
		elif index > count:
			index = count
		elif index < count:
			self.remove(index)
		self.insert(index, child)
	
	def clear(self):
		for child in list(self.__children):
			self.remove(child)
	
	def attach(self, child, index=-1, name=None):
		self.insert(index, child)
		if name is not None:
			self.set_name(name)
		return self
	
	def detach(self):
		if self.__parent is not None:
			self.__parent.remove(self)
		return self
	
	def extend(self, widgets):
		for widget in widgets:
			self.insert(-1, widget)
	
	def load(self, xml='', globals=None, locals=None, node=None):
		if node is None:
			node = parse_xml(xml)
		
		if node.tag != self.NAME:
			raise ValueError("tag mismatch, expecting '%s'" % self.NAME)
		
		self._load_properties(node, globals, locals)
		
		for child in node.getchildren():
			widget = get_factory(child.tag)()
			widget.load(None, globals, locals, child)
			self.append(widget)
	
	def _load_properties(self, node, globals, locals):
		for name, prop in self.PROPERTIES.iteritems():
			try:
				prop.load(self, name, node, globals, locals)
			except Exception, e:
				raise ValueError('failed to load property "%s" at line %d, column %d: %s' % (name, node.line, node.column, str(e)))
	
	def get_name(self):
		return self.__name
	
	def set_name(self, name):
		self.__name = str(name)
	
	def get_extra(self):
		return self.__extra
	
	def set_extra(self, extra):
		self.__extra = extra
	
	def get_index(self):
		if self.__parent is None:
			return -1
		return self.__parent.__children.index(self)
	
	def set_index(self, index):
		old_index = self.get_index()
		if old_index < 0:
			return
		if index > old_index:
			index -= 1
		self.__parent.remove(self)
		self.__parent.insert(index, self)
	
	def get_parent_frame(self):
		parent = self
		while (parent is not None) and (not isinstance(parent, slew.Frame)):
			parent = parent.__parent
		return parent
	
	def get_radio(self, group, value=None):
		if isinstance(self, slew.Radio):
			if self.get_group() == group:
				if value is None:
					if self.is_selected():
						return self
				elif value == self.get_value():
					return self
		for child in self.__children:
			radio = child.get_radio(group, value)
			if radio is not None:
				return radio
	
	@deprecated
	def destroy(self):
		self.detach()
	
	@deprecated
	def is_destroyed(self):
		return False
	
	@deprecated
	def parent(self):
		return self.get_parent()
	
	@deprecated
	def children(self):
		return self.get_children()
	
	name = DeprecatedDescriptor('name')
	index = DeprecatedDescriptor('index')
	handler = DeprecatedDescriptor('handler')
