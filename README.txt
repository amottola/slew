Slew GUI library
----------------

An easy to use, multiplatform GUI library. Features include (but not limited to):
- Backends-based architecture; currently there's one backend coded in C++ and based on Qt
- Support for loading interface definitions from XML files
- Complete widgets set, including (editable) item based views (Grid, TreeView, etc...)
- Printing support
- GDI classes to draw on top of bitmaps, on widgets, and while printing

This version has been developed and tested on Python 2.7; support for Python 3.x may come in the future.


Prerequisites
-------------

To install Slew, you need the following software installed:

- Python 2.7.x (works on 2.6.x and 2.5.x too)
- a working C/C++ compiler (gcc on MacOS X/Linux, MSVC on Win32)
- Qt 4.8.1 or newer


How to install
--------------

Slew comes as a standard distutil package, so to install it is sufficient to type:

python setup.py install

And you are done. Now have fun!
