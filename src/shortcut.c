/*
 * ion/shortcut.c
 *
 * Copyright (c) Lukas Schroeder 2002
 * See the included file LICENSE for details.
 */

#include <ctype.h>

#include "common.h"
#include "binding.h"
#include "global.h"
#include "key.h"
#include "event.h"
#include "cursor.h"
#include "client.h"
#include "shortcut.h"
#include "grab.h"

bool shortcut_is_valid(int cut)
{
	return ((cut>='0' && cut<='9') || (cut>='A' && cut<='z'));
}

void shortcut_remove(int cut)
{
	WThing *thing;

	if(!shortcut_is_valid(cut))
		return;

	thing=wglobal.shortcuts[cut];
	if(!thing)
		return;

	if(WTHING_IS(thing, WFrame)){
		WFrame *frame=(WFrame*)thing;
		frame->shortcut=0;
	} 

	wglobal.shortcuts[cut]=NULL;
}

static void shortcut_remove_from_frame(WFrame *frame)
{
	shortcut_remove(frame->shortcut);
	assert(frame->shortcut==0);
	frame_recalc_bar(frame);
}

void shortcut_set(WThing *thing, int cut)
{
	bool valid=shortcut_is_valid(cut);

	if(valid){
		WThing *oldthing=wglobal.shortcuts[cut];
		if(oldthing && WTHING_IS(thing, WFrame))
			shortcut_remove_from_frame((WFrame*)oldthing);
	}

	if(WTHING_IS(thing, WFrame))
		shortcut_remove_from_frame((WFrame*)thing);

	if(!valid)
		return;

	shortcut_remove(cut);

	wglobal.shortcuts[cut]=thing;
	if(WTHING_IS(thing, WFrame)){
		WFrame *frame=(WFrame*)thing;
		frame->shortcut=cut;
		frame_recalc_bar(frame);
	}
}

static char *decode_keysym(XKeyEvent *ev)
{
	static char buffer[10];
	int len;

	len = XLookupString(ev, buffer, 10, NULL, NULL);
	if(!len || !shortcut_is_valid((int)buffer[0]))
		buffer[0]=0;
	buffer[1]=0;
	return buffer;
}

static bool setshortcut_handler(WThing *thing, XEvent *ev)
{
	KeySym sym;
	char *sc;

	if(ismod(ev->xkey.keycode))
		return FALSE;

	sc=decode_keysym(&ev->xkey);
	shortcut_set(thing, (int)sc[0]);
	return TRUE;
}

static bool gotoshortcut_handler(WThing *thing, XEvent *ev)
{
	WThing *target;
	char *sc;

	if(ismod(ev->xkey.keycode))
		return FALSE;

	sc=decode_keysym(&ev->xkey);
	if(sc[0] && shortcut_is_valid((int)sc[0])){

		target=wglobal.shortcuts[(int)sc[0]];
		if(!target)
			return TRUE;

		if(WTHING_IS(target, WFrame)){
			goto_window((WWindow*)target);
		}else if(WTHING_IS(target, WClient))
			goto_client((WClient*)target);
		/* else: "Ooops. target is not a WFrame" */
	}
	/*skip_focusenter();*/
	return TRUE;
}

void set_shortcut(WFrame *frame)
{
	grab_establish((WThing*)frame, setshortcut_handler, FocusChangeMask|KeyReleaseMask);
}

void goto_shortcut(WFrame *frame)
{
	grab_establish((WThing*)frame, gotoshortcut_handler, FocusChangeMask|KeyReleaseMask);
}

