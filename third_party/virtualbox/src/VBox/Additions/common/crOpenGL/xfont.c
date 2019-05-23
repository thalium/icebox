/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "stub.h"

/** code borrowed from Mesa */


/** Fill a BITMAP with a character C from thew current font
   in the graphics context GC.  WIDTH is the width in bytes
   and HEIGHT is the height in bits.

   Note that the generated bitmaps must be used with

        glPixelStorei (GL_UNPACK_SWAP_BYTES, GL_FALSE);
        glPixelStorei (GL_UNPACK_LSB_FIRST, GL_FALSE);
        glPixelStorei (GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
        glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

   Possible optimizations:

     * use only one reusable pixmap with the maximum dimensions.
     * draw the entire font into a single pixmap (careful with
       proportional fonts!).
*/


/**
 * Generate OpenGL-compatible bitmap.
 */
static void
fill_bitmap(Display *dpy, Window win, GC gc,
			unsigned int width, unsigned int height,
			int x0, int y0, unsigned int c, GLubyte *bitmap)
{
	XImage *image;
	unsigned int x, y;
	Pixmap pixmap;
	XChar2b char2b;

	pixmap = XCreatePixmap(dpy, win, 8*width, height, 1);
	XSetForeground(dpy, gc, 0);
	XFillRectangle(dpy, pixmap, gc, 0, 0, 8*width, height);
	XSetForeground(dpy, gc, 1);

	char2b.byte1 = (c >> 8) & 0xff;
	char2b.byte2 = (c & 0xff);

	XDrawString16(dpy, pixmap, gc, x0, y0, &char2b, 1);

	image = XGetImage(dpy, pixmap, 0, 0, 8*width, height, 1, XYPixmap);
	if (image) {
		/* Fill the bitmap (X11 and OpenGL are upside down wrt each other).  */
		for (y = 0; y < height; y++)
			for (x = 0; x < 8*width; x++)
				if (XGetPixel(image, x, y))
					bitmap[width*(height - y - 1) + x/8] |= (1 << (7 - (x % 8)));
		XDestroyImage(image);
	}

	XFreePixmap(dpy, pixmap);
}

/*
 * determine if a given glyph is valid and return the
 * corresponding XCharStruct.
 */
static XCharStruct *isvalid(XFontStruct *fs, unsigned int which)
{
	unsigned int rows, pages;
	unsigned int byte1 = 0, byte2 = 0;
	int i, valid = 1;

	rows = fs->max_byte1 - fs->min_byte1 + 1;
	pages = fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1;

	if (rows == 1) {
		/* "linear" fonts */
		if ((fs->min_char_or_byte2 > which) ||
			(fs->max_char_or_byte2 < which)) valid = 0;
	}
	else {
		/* "matrix" fonts */
		byte2 = which & 0xff;
		byte1 = which >> 8;
		if ((fs->min_char_or_byte2 > byte2) ||
			(fs->max_char_or_byte2 < byte2) ||
			(fs->min_byte1 > byte1) ||
			(fs->max_byte1 < byte1)) valid = 0;
	}

	if (valid) {
		if (fs->per_char) {
			if (rows == 1) {
				/* "linear" fonts */
				return fs->per_char + (which-fs->min_char_or_byte2);
			}
			else {
				/* "matrix" fonts */
				i = ((byte1 - fs->min_byte1) * pages) +
					(byte2 - fs->min_char_or_byte2);
				return fs->per_char + i;
			}
		}
		else {
			return &fs->min_bounds;
		}
	}
	return NULL;
}


void stubUseXFont( Display *dpy, Font font, int first, int count, int listbase )
{
	Window win;
	Pixmap pixmap;
	GC gc;
	XGCValues values;
	unsigned long valuemask;
	XFontStruct *fs;
	GLint swapbytes, lsbfirst, rowlength;
	GLint skiprows, skippixels, alignment;
	unsigned int max_width, max_height, max_bm_width, max_bm_height;
	GLubyte *bm;
	int i;

	win = RootWindow(dpy, DefaultScreen(dpy));

	fs = XQueryFont(dpy, font);
	if (!fs) {
		crWarning("Couldn't get font structure information");
		return;
	}

	/* Allocate a bitmap that can fit all characters.  */
	max_width = fs->max_bounds.rbearing - fs->min_bounds.lbearing;
	max_height = fs->max_bounds.ascent + fs->max_bounds.descent;
	max_bm_width = (max_width + 7) / 8;
	max_bm_height = max_height;

	bm = (GLubyte *) crAlloc((max_bm_width * max_bm_height) * sizeof(GLubyte));
	if (!bm) {
		XFreeFontInfo( NULL, fs, 1 );
		crWarning("Couldn't allocate bitmap in glXUseXFont()");
		return;
	}

	/* Save the current packing mode for bitmaps.  */
	glGetIntegerv(GL_UNPACK_SWAP_BYTES, &swapbytes);
	glGetIntegerv(GL_UNPACK_LSB_FIRST, &lsbfirst);
	glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowlength);
	glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skiprows);
	glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skippixels);
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);

	/* Enforce a standard packing mode which is compatible with
	   fill_bitmap() from above.  This is actually the default mode,
	   except for the (non)alignment.  */
	glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
	glPixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	pixmap = XCreatePixmap(dpy, win, 10, 10, 1);
	values.foreground = BlackPixel(dpy, DefaultScreen (dpy));
	values.background = WhitePixel(dpy, DefaultScreen (dpy));
	values.font = fs->fid;
	valuemask = GCForeground | GCBackground | GCFont;
	gc = XCreateGC(dpy, pixmap, valuemask, &values);
	XFreePixmap(dpy, pixmap);

	for (i = 0; i < count; i++) {
		unsigned int width, height, bm_width, bm_height;
		GLfloat x0, y0, dx, dy;
		XCharStruct *ch;
		int x, y;
		unsigned int c = first + i;
		int list = listbase + i;
		int valid;

		/* check on index validity and get the bounds */
		ch = isvalid(fs, c);
		if (!ch) {
			ch = &fs->max_bounds;
			valid = 0;
		}
		else {
			valid = 1;
		}

		/* glBitmap()' parameters:
		   straight from the glXUseXFont(3) manpage.  */
		width = ch->rbearing - ch->lbearing;
		height = ch->ascent + ch->descent;
		x0 = -ch->lbearing;
		y0 = ch->descent - 0;  /* XXX used to subtract 1 here */
		                       /* but that caused a conformance failure */
		dx = ch->width;
		dy = 0;

		/* X11's starting point.  */
		x = -ch->lbearing;
		y = ch->ascent;

		/* Round the width to a multiple of eight.  We will use this also
		for the pixmap for capturing the X11 font.  This is slightly
		inefficient, but it makes the OpenGL part real easy.  */
		bm_width = (width + 7) / 8;
		bm_height = height;

		glNewList(list, GL_COMPILE);
		if (valid && (bm_width > 0) && (bm_height > 0)) {
			crMemset(bm, '\0', bm_width * bm_height);
			fill_bitmap(dpy, win, gc, bm_width, bm_height, x, y, c, bm);
			glBitmap(width, height, x0, y0, dx, dy, bm);
		}
		else {
			glBitmap(0, 0, 0.0, 0.0, dx, dy, NULL);
		}
		glEndList();
	}

	crFree(bm);
	XFreeFontInfo(NULL, fs, 1);
	XFreeGC(dpy, gc);

	/* Restore saved packing modes.  */
	glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbytes);
	glPixelStorei(GL_UNPACK_LSB_FIRST, lsbfirst);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, rowlength);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, skiprows);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, skippixels);
	glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
}
