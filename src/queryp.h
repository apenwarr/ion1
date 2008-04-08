/*
 * ion/queryp.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_QUERYP_H
#define INCLUDED_QUERYP_H

#include "query.h"
#include "frame.h"
#include "wedln.h"
#include "edln.h"

extern WEdln *do_query_edln(WFrame *frame, WEdlnHandler *handler,
							const char *prompt, const char *dflt,
							EdlnCompletionHandler *chnd);

extern void handler_runfile(WThing *thing, char *str, char *userdata);
extern void handler_runwith(WThing *thing, char *str, char *userdata);
extern void handler_exec(WThing *thing, char *str, char *userdata);
extern void handler_attachclient(WThing *thing, char *str, char *userdata);
extern void handler_gotoclient(WThing *thing, char *str, char *userdata);
extern void handler_workspace(WThing *thing, char *name, char *userdata);
extern void handler_workspace_with(WThing *thing, char *name, char *userdata);
extern void handler_renameworkspace(WThing *thing, char *name, char *userdata);
extern void handler_function(WThing *thing, char *fn, char *userdata);
extern void handler_yesno(WThing *thing, char *yesno, char *fn);
										
#endif /* INCLUDED_QUERYP_H */
