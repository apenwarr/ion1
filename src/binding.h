/*
 * ion/binding.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_BINDING_H
#define INCLUDED_BINDING_H

#include <libtu/tokenizer.h>

#include "common.h"
#include "function.h"
#include "obj.h"


#define ACT_KEYPRESS 		0
#define ACT_BUTTONPRESS		1
#define ACT_BUTTONMOTION	2
#define ACT_BUTTONCLICK		3
#define ACT_BUTTONDBLCLICK	4

#define BINDING_MAXARGS		3
#define BINDING_INIT		{0, 0, 0, FALSE, NULL, NULL, 0, {TOK_INIT, TOK_INIT, TOK_INIT}}


INTRSTRUCT(WBinding)
INTRSTRUCT(WBindmap)


DECLSTRUCT(WBinding){
	uint kcb;	/* keycode or button */
	uint state;
	uint act;
	bool waitrel;
	WBindmap *submap;
	WFunction *func;
	int nargs;
	Token args[BINDING_MAXARGS];
};


DECLSTRUCT(WBindmap){
	int nbindings;
	uint confdefmod;
	WBinding *bindings;
	WBindmap *parent;
};

extern void init_bindings();
extern WBindmap *create_bindmap();
extern void destroy_bindmap(WBindmap *bindmap);
extern void destroy_binding(WBinding *binding);
extern bool add_binding(WBindmap *bindmap, const WBinding *binding);
extern WBinding *lookup_binding(WBindmap *bindmap, int act, uint state,
								uint kcb);
extern void grab_bindings(WBindmap *bindmap, Window win);
extern void call_binding(WBinding *binding, WThing *thing);
extern int unmod(int state, int keycode);
extern bool ismod(int keycode);
extern void update_modmap();

#endif /* INCLUDED_BINDING_H */
