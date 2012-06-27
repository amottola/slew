import sys
import glob
import os
import os.path
import re

try:
	from setuptools import setup, Extension
except:
	from distutils.core import setup
	from distutils.extension import Extension

import distutils.ccompiler


NAME = 'slew'


debug = '--debug' in sys.argv
if debug:
	sys.argv.remove('--debug')

profile = '--profile' in sys.argv
if profile:
	sys.argv.remove('--profile')

develop = '--develop' in sys.argv
if develop:
	sys.argv.remove('--develop')


new_compiler = distutils.ccompiler.new_compiler
def slew_compiler(*args, **kwargs):
	compiler = new_compiler(*args, **kwargs)
	
	def wrapper(func):
		def _wrapper(*args, **kwargs):
			kwargs['debug'] = debug
			return func(*args, **kwargs)
		return _wrapper
	
	compiler.compile = wrapper(compiler.compile)
	compiler.link_shared_object = wrapper(compiler.link_shared_object)
	compiler.create_static_lib = wrapper(compiler.create_static_lib)
	compiler.find_library_file = wrapper(compiler.find_library_file)
	if sys.platform == 'darwin':
		compiler.language_map['.mm'] = "objc"
		compiler.src_extensions.append(".mm")
	elif sys.platform == 'win32':
		compiler.initialize()
		compiler.compile_options.append('/Z7')
	return compiler

distutils.ccompiler.new_compiler = slew_compiler



sources = []

qt_dir = os.environ.get('QTDIR')

vars = {
	'qt_dir':			qt_dir,
	'debug_prefix':		'd' if debug else '',
}

if sys.platform == 'darwin':
	moc = 'moc'
	if qt_dir is None:
		vars['qt_dir'] = '/Library/Frameworks'
		cflags = '-I/Library/Frameworks/QtCore.framework/Headers -I/Library/Frameworks/QtGui.framework/Headers -I/Library/Frameworks/QtOpenGL.framework/Headers -I/Library/Frameworks/QtWebKit.framework/Headers -I/Library/Frameworks/QtNetwork.framework/Headers '
		ldflags = ''
	else:
		cflags = '-I%(qt_dir)s/include -I%(qt_dir)s/include/QtCore -I%(qt_dir)s/include/QtGui -I%(qt_dir)s/include/QtOpenGL -I%(qt_dir)s/lib/QtWebKit.framework/Headers -I%(qt_dir)s/lib/QtNetwork.framework/Headers '
		ldflags = '-F%(qt_dir)s/lib -F%(qt_dir)s '
	cflags += '-gstabs+ -mmacosx-version-min=10.5 -isysroot /Developer/SDKs/MacOSX10.5.sdk -Wno-write-strings -fvisibility=hidden'
	ldflags += '-Wl,-syslibroot,/Developer/SDKs/MacOSX10.5.sdk -framework QtCore -framework QtGui -framework QtOpenGL -framework QtWebKit -mmacosx-version-min=10.5 -headerpad_max_install_names'
	data_files = []
	if develop:
		os.environ['ARCHFLAGS'] = '-arch x86_64'
	if profile:
		cflags += ' -pg'
		ldflags += ' -pg'
elif sys.platform == 'win32':
	moc = '%(qt_dir)s\\bin\\moc.exe'
	cflags = '/I"%(qt_dir)s\\include" /I"%(qt_dir)s\\include\\QtCore" /I"%(qt_dir)s\\include\\QtGui" /I"%(qt_dir)s\\include\\QtOpenGL" /I"%(qt_dir)s\\include\\QtWebKit" /I"%(qt_dir)s\\include\\QtNetwork" /Zc:wchar_t-'
	ldflags = '/DEBUG /PDB:None /LIBPATH:"%(qt_dir)s\\lib" QtCore%(debug_prefix)s4.lib QtGui%(debug_prefix)s4.lib QtOpenGL%(debug_prefix)s4.lib QtNetwork%(debug_prefix)s4.lib QtWebKit%(debug_prefix)s4.lib user32.lib shell32.lib gdi32.lib advapi32.lib secur32.lib'
	dlls = [
		'%(qt_dir)s\\bin\\QtCore%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtGui%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtOpenGL%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtNetwork%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtWebKit%(debug_prefix)s4.dll',
	]
	data_files = [ (sys.prefix + '/DLLs', [ (dll % vars) for dll in dlls ]) ]
	if profile:
		ldflags += ' /PROFILE'
else:
	moc = '%(qt_dir)s/bin/moc'
	cflags = '-Wno-write-strings -fvisibility=hidden -I%(qt_dir)s/include -I%(qt_dir)s/include/QtCore -I%(qt_dir)s/include/QtGui -I%(qt_dir)s/include/QtOpenGL -I%(qt_dir)s/include/QtNetwork'
	ldflags = '-L%(qt_dir)s/lib -lQtCore -lQtGui -lQtOpenGL -lQtWebKit -Wl,-rpath=%(qt_dir)s/lib'
	data_files = []


moc = moc % vars
cflags = (cflags % vars).split(' ')
ldflags = (ldflags % vars).split(' ')
defines = [
	('NOUNCRYPT', None),
	('UNICODE', None)
]


if not os.path.exists(os.path.join('backends', 'qt', 'moc')):
	os.mkdir(os.path.join('backends', 'qt', 'moc'))
if not os.path.exists(os.path.join('backends', 'qt', 'constants')):
	os.mkdir(os.path.join('backends', 'qt', 'constants'))

sources += glob.glob(os.path.join('backends', 'qt', '*.cpp'))
for source in sources:
	target = '%s.moc' % os.path.split(source)[-1][:-4]
	cmd = '%s -nw -i -o %s %s' % (moc, os.path.join('backends', 'qt', 'moc', target), source)
	os.system(cmd)

for source in glob.glob(os.path.join('backends', 'qt', '*.h')):
	target = '%s_h.moc' % os.path.split(source)[-1][:-2]
	cmd = '%s -nw -o %s %s' % (moc, os.path.join('backends', 'qt', 'moc', target), source)
	os.system(cmd)
	
sources += glob.glob(os.path.join('backends', 'qt', 'minizip', '*.c'))
if sys.platform == 'win32':
	sources += glob.glob(os.path.join('backends', 'qt', 'minizip', 'win32', '*.c'))

if sys.platform == 'darwin':
	sources += glob.glob(os.path.join('backends', 'qt', '*.mm'))


for py in glob.glob(os.path.join('src', '*.py')):
	target = os.path.join('backends', 'qt', 'constants', os.path.basename(py)[:-3] + ".h")
	defs = []
	file = open(py, 'rU')
	lines = file.readlines()
	file.close()
	in_defs = False
	prefix = ''
	
	for line in lines:
		if in_defs:
			pos = line.find('#}')
			if pos >= 0:
				in_defs = False
				prefix = ''
			else:
				line = line.strip()
				if line:
					defs.append('#define %s%s' % (prefix, line.replace('=', '')))
		else:
			pos = line.find('#defs{')
			if pos >= 0:
				in_defs = True
				prefix = line[pos + 6:-1]
	
	if defs:
		file = open(target, 'w')
		file.write('\n'.join(defs))
		file.close()


setup(
    name = NAME,
    version = '0.1',
    
    packages = [ NAME ],
    package_dir = { NAME: 'src' },
    
    ext_modules = [ Extension('%s._%s' % (NAME, NAME),
    	sources,
    	include_dirs = [
    		os.path.join('backends', 'qt'),
    		os.path.join('backends', 'qt', 'moc'),
    		os.path.join('backends', 'qt', 'zlib'),
    		os.path.join('backends', 'qt', 'minizip'),
    	],
    	define_macros = defines,
		extra_compile_args = cflags,
		extra_link_args = ldflags,
	) ],
	
	data_files = data_files,
	
	zip_safe = True,

    # metadata for upload to PyPI
    author = "Angelo Mottola",
    author_email = "a.mottola@gmail.com",
    description = "%s user interface library" % NAME.capitalize(),
    license = "PSF",
    keywords = "%s gui qt web pyjs" % NAME,
)

