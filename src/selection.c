/*
 * ion/selection.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#include <X11/Xmd.h>
#include <string.h>
#include "common.h"
#include "global.h"
#include "property.h"
#include "wedln.h"


static char *selection_data=NULL;
static int selection_length;

void send_selection(XSelectionRequestEvent *ev)
{
/*	XTextProperty tp;*/
	XSelectionEvent sev;
	
	if(selection_data==NULL)
		return;
	
/*	XmbTextListToTextProperty(wglobal.dpy, &selection_data, 1,
							  XCompoundTextStyle, &tp);
	
	XChangeProperty(wglobal.dpy, ev->requestor, ev->property, ev->target,
					tp.format, PropModeReplace, tp.value, tp.nitems);*/
	XChangeProperty(wglobal.dpy, ev->requestor, ev->property, XA_STRING,
					8, PropModeReplace, (uchar*)selection_data,
					selection_length);
	
	sev.type=SelectionNotify;
	sev.requestor=ev->requestor;
	sev.selection=ev->selection;
	sev.target=ev->target;
	sev.time=ev->time;
	sev.property=ev->property;
	XSendEvent(wglobal.dpy, ev->requestor, False, 0L, (XEvent*)&sev);
}


static void insert_selection(WEdln *wedln, Window win, Atom prop)
{
	Atom real_type;
	int format;
	ulong n, left;
	int status;
	int total;
	char *p;

	status=XGetWindowProperty(wglobal.dpy, win, prop, 0L, 0L, False,
							  AnyPropertyType, &real_type, &format,
							  &n, &left, (uchar**)&p);
	
	if(status!=Success)
		return;
	
	if(p!=NULL)
		XFree(p);
	
	if(real_type==None)
		return;
	
	total=0;
	
	while(left>0){
		status=XGetWindowProperty(wglobal.dpy, win, prop, total/4,
								  1+left/4, False, AnyPropertyType,
								  &real_type, &format,
								  &n, &left, (uchar**)&p);
		if(status!=Success)
			break;
		
		n*=(format/8);
		
		edln_insstr_n(&(wedln->edln), p, n);

		XFree(p);
		total+=n;
	}
	wedln_draw(wedln, FALSE);
}


static void insert_cutbuffer(WEdln *wedln)
{
	char *p;
	int n;
	
	p=XFetchBytes(wglobal.dpy, &n);
	
	if(n<=0 || p==NULL)
		return;
	
	edln_insstr_n(&(wedln->edln), p, n);
	wedln_draw(wedln, FALSE);
}


void receive_selection(XSelectionEvent *ev)
{
	Atom prop=ev->property;
	Window win=ev->requestor;
	WEdln *edln;
	
	edln=FIND_WINDOW_T(win, WEdln);
	
	if(edln==NULL)
		return;
	
	if(prop==None){
		insert_cutbuffer(edln);
		return;
	}

	insert_selection(edln, win, prop);
	XDeleteProperty(wglobal.dpy, win, prop);
}


void clear_selection()
{
	if(selection_data!=NULL){
		free(selection_data);
		selection_data=NULL;
	}
}


void set_selection(const char *p, int n)
{
	if(selection_data!=NULL)
		free(selection_data);
	
	selection_data=ALLOC_N(char, n+1);
	
	if(selection_data==NULL){
		warn_err();
		return;
	}
	memcpy(selection_data, p, n);
	selection_data[n]='\0';
	selection_length=n;
	
	XStoreBytes(wglobal.dpy, p, n);
	
	/* Use tabdrag_win for this in lack of better... */
	XSetSelectionOwner(wglobal.dpy, XA_PRIMARY,
					   DefaultRootWindow(wglobal.dpy),
					   CurrentTime);
}


void request_selection(Window win)
{
/*	Atom cta=XInternAtom(wglobal.display, "COMPOUND_TEXT", False);*/
	XConvertSelection(wglobal.dpy, XA_PRIMARY, XA_STRING,
					  wglobal.atom_selection, win, CurrentTime);
}

