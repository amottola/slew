#include "slew.h"

#ifdef Q_OS_LINUX

#include <QX11Info>
#include <X11/Xlib.h>


// MWM support
struct MWMHints {
	unsigned long flags, functions, decorations;
	long input_mode;
	unsigned long status;
};


enum {
	MWM_HINTS_FUNCTIONS   = (1L << 0),
	
	MWM_FUNC_ALL      = (1L << 0),
	MWM_FUNC_RESIZE   = (1L << 1),
	MWM_FUNC_MOVE     = (1L << 2),
	MWM_FUNC_MINIMIZE = (1L << 3),
	MWM_FUNC_MAXIMIZE = (1L << 4),
	MWM_FUNC_CLOSE    = (1L << 5),
	
	MWM_HINTS_DECORATIONS = (1L << 1),
	
	MWM_DECOR_ALL      = (1L << 0),
	MWM_DECOR_BORDER   = (1L << 1),
	MWM_DECOR_RESIZEH  = (1L << 2),
	MWM_DECOR_TITLE    = (1L << 3),
	MWM_DECOR_MENU     = (1L << 4),
	MWM_DECOR_MINIMIZE = (1L << 5),
	MWM_DECOR_MAXIMIZE = (1L << 6),
	
	MWM_HINTS_INPUT_MODE = (1L << 2),
	
	MWM_INPUT_MODELESS                  = 0L,
	MWM_INPUT_PRIMARY_APPLICATION_MODAL = 1L,
	MWM_INPUT_FULL_APPLICATION_MODAL    = 3L
};


static MWMHints
GetMWMHints(Display *display, Window window)
{
    MWMHints mwmhints;
	
	Atom _motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", true);
    Atom type;
    int format;
    ulong nitems, bytesLeft;
    uchar *data = 0;
    if ((_motif_wm_hints != None) &&
		(XGetWindowProperty(display, window, _motif_wm_hints, 0, 5, false, _motif_wm_hints, &type, &format, &nitems, &bytesLeft, &data) == Success) &&
		(type == _motif_wm_hints) && (format == 32) && (nitems >= 5)) {
        mwmhints = *(reinterpret_cast<MWMHints *>(data));
    } else {
        mwmhints.flags = 0L;
        mwmhints.functions = MWM_FUNC_ALL;
        mwmhints.decorations = MWM_DECOR_ALL;
        mwmhints.input_mode = 0L;
        mwmhints.status = 0L;
    }

    if (data)
        XFree(data);

    return mwmhints;
}


static void
SetMWMHints(Display *display, Window window, const MWMHints &mwmhints)
{
	Atom _motif_wm_hints = XInternAtom(display, "_MOTIF_WM_HINTS", true);
	if (_motif_wm_hints == None)
		return;
	
    if (mwmhints.flags != 0l) {
        XChangeProperty(display, window, _motif_wm_hints, _motif_wm_hints, 32,
                        PropModeReplace, (unsigned char *) &mwmhints, 5);
    } else {
        XDeleteProperty(display, window, _motif_wm_hints);
    }
}


void
helper_set_resizeable(QWidget *widget, bool enabled)
{
	Window window = (Window)widget->winId();
	Display *display = widget->x11Info().display();
	
	MWMHints mwmHints = GetMWMHints(display, window);
	const bool wasFuncResize = mwmHints.functions & MWM_FUNC_RESIZE;
	
	if ((enabled) && (widget->minimumSize() != widget->maximumSize()))
		mwmHints.functions |= MWM_FUNC_RESIZE;
	else
		mwmHints.functions &= ~MWM_FUNC_RESIZE;
	if (wasFuncResize != (mwmHints.functions & MWM_FUNC_RESIZE))
		SetMWMHints(display, window, mwmHints);
}

#endif
