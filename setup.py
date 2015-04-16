import sys
import glob
import os
import os.path
import re
import subprocess

try:
	from setuptools import setup, Extension
except:
	from distutils.core import setup
	from distutils.extension import Extension

import distutils.ccompiler



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
is_qt5 = False
qt_libs = [
	'QtCore',
	'QtGui',
	'QtOpenGL',
	'QtNetwork',
	'QtWebKit',
	'QtSvg',
]
qt_includes = []

vars = {
	'qt_dir':			qt_dir,
	'debug_prefix':		'd' if debug else '',
}

if sys.platform == 'darwin':
	if qt_dir is None:
		# home = os.environ['HOME']
		# for path in os.listdir(home):
		# 	if path.startswith('Qt5.'):
		# 		qt_ver = path[2:]
		# 		if os.path.exists(os.path.join(home, path, qt_ver)):
		# 			qt_dir = os.path.join(home, path, qt_ver, 'clang_64')
		# 			vars['qt_dir'] = qt_dir
		# 			vars['qt_ver'] = qt_ver
		# 			moc = '%(qt_dir)s/bin/moc'
		# 			cflags = '-I%(qt_dir)s/include -I%(qt_dir)s/include/QtCore -I%(qt_dir)s/include/QtGui -I%(qt_dir)s/include/QtGui/%(qt_ver)s/QtGui -I%(qt_dir)s/include/QtOpenGL -I%(qt_dir)s/lib/QtWebKit.framework/Headers -I%(qt_dir)s/lib/QtNetwork.framework/Headers -I%(qt_dir)s/lib/QtWidgets.framework/Headers -I%(qt_dir)s/lib/QtWebKitWidgets.framework/Headers -I%(qt_dir)s/lib/QtPrintSupport.framework/Headers -I%(qt_dir)s/include/QtSvg '
		# 			ldflags = '-F%(qt_dir)s/lib -F%(qt_dir)s '
		# 			qt5 = True
		# 			break
		# else:

		vars['qt_dir'] = '/Library/Frameworks'
		moc = 'moc'
		is_qt5 = os.path.exists('/Library/Frameworks/QtWidgets.framework')
		ldflags = ''
	else:
		vars['qt_dir'] = os.path.join(qt_dir, 'lib')
		moc = '%s/bin/moc' % qt_dir
		is_qt5 = os.path.exists('%s/lib/QtWidgets.framework' % qt_dir)
		ldflags = '-F%s/lib -F%s ' % (qt_dir, qt_dir)

	if is_qt5:
		qt_libs += [ 'QtWidgets', 'QtWebKitWidgets', 'QtPrintSupport' ]
	qt_includes = [ ('-I%%(qt_dir)s/%s.framework/Headers' % path) for path in qt_libs ]
	if is_qt5:
		qt_ver = qt_dir[qt_dir.rfind('/') + 4:]
		qt_includes += [ '-I%%(qt_dir)s/QtGui.framework/Headers/%s/QtGui' % qt_ver ]
	cflags = (' '.join(qt_includes)) + ' -F%(qt_dir)s '

	if 'clang' in subprocess.check_output('gcc --version', stderr=subprocess.STDOUT, shell=True, universal_newlines=True):
		cflags += '-Qunused-arguments -Wno-unused-private-field -Wno-self-assign '
	
	sdk = os.environ.get('SDK', None)
	for index, arg in enumerate(sys.argv):
		if arg == '--sdk':
			if index + 1 >= len(sys.argv):
				print 'Missing sdk parameter'
				sys.exit(1)
			sdk = sys.argv[index + 1]
			del sys.argv[index:index+2]
			break
		elif arg.startswith('--sdk='):
			sdk = arg[6:].strip()
			del sys.argv[index]
			break

	def find_sdk(version):
		for root in ('/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs', '/Developer/SDKs'):
			path = os.path.join(root, 'MacOSX%s.sdk' % version)
			if os.path.exists(path):
				break
		else:
			return (None, None)
		version_min = '10.%d' % (max(5, int(version.split('.')[-1]) - 2))
		return (path, version_min)

	if sdk is not None:
		sdk, macosx_version_min = find_sdk(sdk)
		if sdk is None:
			print 'Error: unknown SDK:', sdk
			sys.exit(1)
	else:
		for version in ('10.5', '10.6', '10.7', '10.8', '10.9', '10.10'):
			sdk, macosx_version_min = find_sdk(version)
			if sdk is not None:
				break
		else:
			print 'Error: no valid SDK found!'
			sys.exit(1)
	
	cflags += '-g -mmacosx-version-min=%s -isysroot %s -Wno-write-strings -fvisibility=hidden' % (macosx_version_min, sdk)
	ldflags += '-Wl,-syslibroot,%s -framework QtCore -framework QtGui -framework QtOpenGL -framework QtWebKit -framework QtSvg -mmacosx-version-min=%s -headerpad_max_install_names' % (sdk, macosx_version_min)
	if is_qt5:
		ldflags += ' -framework QtWidgets -framework QtWebKitWidgets -framework QtPrintSupport'
	data_files = []
	if develop:
		os.environ['ARCHFLAGS'] = '-arch x86_64 -O0'
	if profile:
		cflags += ' -pg'
		ldflags += ' -pg'
elif sys.platform == 'win32':
	moc = '%(qt_dir)s\\bin\\moc.exe'
	includes = [
		"include",
		"include\\QtCore",
		"include\\QtGui",
		"include\\QtOpenGL",
		"include\\QtWebKit",
		"include\\QtNetwork",
		"include\\QtSvg",
	]
	cincludes = ' '.join([ '/I"%%(qt_dir)s\\%s"' % include for include in includes ])
	qt_includes = [ '-I"%%(qt_dir)s\\%s"' % include for include in includes ]
	cflags = '%s /Zc:wchar_t- /Z7' % cincludes
	ldflags = '/DEBUG /LIBPATH:"%(qt_dir)s\\lib" QtCore%(debug_prefix)s4.lib QtGui%(debug_prefix)s4.lib QtOpenGL%(debug_prefix)s4.lib QtNetwork%(debug_prefix)s4.lib QtWebKit%(debug_prefix)s4.lib QtSvg%(debug_prefix)s4.lib user32.lib shell32.lib gdi32.lib advapi32.lib secur32.lib netapi32.lib'
	dlls = [
		'%(qt_dir)s\\bin\\QtCore%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtGui%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtOpenGL%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtNetwork%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtWebKit%(debug_prefix)s4.dll',
		'%(qt_dir)s\\bin\\QtSvg%(debug_prefix)s4.dll',
	]
	data_files = [ (sys.prefix + '/DLLs', [ (dll % vars) for dll in dlls ]) ]
	if profile:
		ldflags += ' /PROFILE'
else:
	moc = 'moc'
	qt5_paths = (
		'/usr/include/qt5',
		'/usr/include/x86_64-linux-gnu/qt5',
		'/usr/include/i386-linux-gnu/qt5',
	)
	for qt_include in qt5_paths:
		if os.path.exists(qt_include):
			qt_libs += [ 'QtWidgets', 'QtWebKitWidgets', 'QtPrintSupport', 'QtX11Extras' ]
			qt_libs = [ name.replace('Qt', 'Qt5') for name in qt_libs ]
			break
	else:
		qt_include = '/usr/include/qt4'

	qt_includes = [ ('-I%s/%s' % (qt_include, name.replace('Qt5', 'Qt'))) for name in qt_libs ]
	cflags = '-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -Wno-write-strings -fvisibility=hidden -I%s ' % qt_include
	cflags += ' '.join(qt_includes)
	ldflags = ' '.join([ ('-l%s' % name) for name in qt_libs ])
	data_files = []


moc = moc % vars
cflags = (cflags % vars).split(' ')
ldflags = (ldflags % vars).split(' ')
defines = [
	('NOUNCRYPT', None),
	('UNICODE', None)
]
qt_includes = [ x % vars for x in qt_includes ]


if not os.path.exists(os.path.join('backends', 'qt', 'moc')):
	os.mkdir(os.path.join('backends', 'qt', 'moc'))
if not os.path.exists(os.path.join('backends', 'qt', 'constants')):
	os.mkdir(os.path.join('backends', 'qt', 'constants'))

sources += glob.glob(os.path.join('backends', 'qt', '*.cpp'))
if sys.platform == 'darwin':
	sources += glob.glob(os.path.join('backends', 'qt', '*.mm'))

for source in sources:
	target = '%s.moc' % os.path.splitext(os.path.split(source)[-1])[0]
	cmd = '%s -nw -i %s -o %s %s' % (moc, ' '.join(qt_includes), os.path.join('backends', 'qt', 'moc', target), source)
	print cmd
	os.system(cmd)

for source in glob.glob(os.path.join('backends', 'qt', '*.h')):
	target = '%s_h.moc' % os.path.split(source)[-1][:-2]
	cmd = '%s -nw %s -o %s %s' % (moc, ' '.join(qt_includes), os.path.join('backends', 'qt', 'moc', target), source)
	print cmd
	os.system(cmd)
	
sources += glob.glob(os.path.join('backends', 'qt', 'minizip', '*.c'))
if sys.platform == 'win32':
	sources += glob.glob(os.path.join('backends', 'qt', 'minizip', 'win32', '*.c'))


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
    name = 'slew',
    version = '0.9.4',
    
    packages = [ 'slew' ],
    package_dir = { 'slew': 'src' },
    
    ext_modules = [ Extension('slew._slew',
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
    description = "Slew GUI library",
    url = "http://code.google.com/p/slew",
    license = "LGPL",
    keywords = [ "gui", "xml", "qt", "web" ],
	
	classifiers = [
		"Programming Language :: Python",
		"Programming Language :: Python :: 2",
		"Programming Language :: C++",
		"Development Status :: 4 - Beta",
		"Environment :: MacOS X",
		"Environment :: Win32 (MS Windows)",
		"Environment :: X11 Applications :: Qt",
		"Environment :: Web Environment",
		"Intended Audience :: Developers",
		"License :: OSI Approved :: GNU Library or Lesser General Public License (LGPL)",
		"Operating System :: OS Independent",
		"Topic :: Software Development :: Libraries :: Python Modules",
		"Topic :: Software Development :: User Interfaces",
		"Topic :: Software Development :: Widget Sets",
	],
	
	long_description = """\
Slew GUI library
----------------

An easy to use, multiplatform GUI library. Features include (but not limited to):

* Backends-based architecture; currently there's one backend coded in C++ and based on Qt
* Support for loading interface definitions from XML files
* Complete widgets set, including (editable) item based views (Grid, TreeView, etc...)
* Printing support
* GDI classes to draw on top of bitmaps, on widgets, and while printing

This version has been developed and tested on Python 2.7; support for Python 3.x may come in the future.
"""
)

