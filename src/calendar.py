import slew

from utils import *
from gdi import *


@factory
class Calendar(slew.Window):
	
	#defs{SL_CALENDAR_
	NAMES_NONE					= 0
	NAMES_LETTER				= 1
	NAMES_SHORT					= 2
	NAMES_LONG					= 3
	
	STYLE_GRID					= 0x00010000
	STYLE_SELECTABLE			= 0x00020000
	STYLE_EDITABLE				= 0x00040000
	STYLE_CONTROLS				= 0x00080000
	#}
	
	NAME						= 'calendar'
	
	PROPERTIES = merge(slew.Window.PROPERTIES, {
		'style':				BitsProperty(merge(slew.Window.STYLE_BITS, {
									"grid":			STYLE_GRID,
									"selectable":	STYLE_SELECTABLE,
									"editable":		STYLE_EDITABLE,
									"controls":		STYLE_CONTROLS,
								})),
		'names':				ChoiceProperty(['none','letter','short','long']),
		'date':					DateProperty(),
		'min_date':				DateProperty(),
		'max_date':				DateProperty(),
		'first_day_of_week':	IntProperty(),
	})
	
	def initialize(self):
		slew.Window.initialize(self)
		self.__next_icon = None
		self.__prev_icon = None
	
# properties
	
	def get_names(self):
		return self._impl.get_names()
	
	def set_names(self, names):
		self._impl.set_names(names)
	
	def get_date(self):
		return self._impl.get_date()
	
	def set_date(self, date):
		self._impl.set_date(date)
	
	def get_min_date(self):
		return self._impl.get_min_date()
	
	def set_min_date(self, date):
		self._impl.set_min_date(date)
	
	def get_max_date(self):
		return self._impl.get_max_date()
	
	def set_max_date(self, date):
		self._impl.set_max_date(date)
	
	def get_previous_icon(self):
		return self.__prev_icon
	
	def set_previous_icon(self, icon):
		icon = Icon.ensure(icon)
		self.__prev_icon = icon
		self._impl.set_previous_icon(icon)
	
	def get_next_icon(self):
		return self.__next_icon
	
	def set_next_icon(self, icon):
		icon = Icon.ensure(icon)
		self.__next_icon = icon
		self._impl.set_next_icon(icon)
	
	def get_first_day_of_week(self):
		return self._impl.get_first_day_of_week()
	
	def set_first_day_of_week(self, day):
		self._impl.set_first_day_of_week(day)



