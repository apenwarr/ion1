/*
 * ion/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_QUERY_H
#define INCLUDED_QUERY_H

#include "common.h"
#include "frame.h"
#include "client.h"

extern void query_exec(WFrame *frame);
extern void query_runwith(WFrame *frame, char *cmd, char *prompt);
extern void query_runfile(WFrame *frame, char *cmd, char *prompt);
extern void query_attachclient(WFrame *frame);
extern void query_gotoclient(WFrame *frame);
extern void query_workspace(WFrame *frame);
extern void query_workspace_with(WFrame *frame);
extern void query_renameworkspace(WFrame *frame);
extern void query_yesno(WFrame *frame, char *fn, char *prompt);
extern void query_function(WFrame *frame);
extern bool command_sequence(WThing *thing, char *fn);
extern void query_check_function_thing(WThing *t);
extern void query_set_function_thing(WThing *t);

/* Warning: fwarn will free(p)! */
extern void fwarn(WFrame *frame, char *p);

#endif /* INCLUDED_QUERY_H */
