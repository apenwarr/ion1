/*
 * ion/workspace.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_WORKSPACE_H
#define INCLUDED_WORKSPACE_H

#include "common.h"

INTROBJ(WWorkspace)

#include "thing.h"
#include "split.h"
#include "window.h"
#include "screen.h"


DECLOBJ(WWorkspace){
	WThing thing;
	char *name;
	WObj *splitree;
};


extern WWorkspace *create_workspace(WScreen *scr, const char *name,
									bool ci);
extern void rename_workspace(WWorkspace *ws, const char *name);
extern void deinit_workspace(WWorkspace *ws);
extern void workspace_remove_child(WWorkspace *ws, WThing *thing);

extern void init_workspaces(WScreen *scr);

extern bool visible_workspace(WWorkspace *ws);
extern bool on_visible_workspace(WThing *thing);
extern bool active_workspace(WWorkspace *ws);
extern bool on_active_workspace(WThing *thing);
extern WWorkspace* workspace_of(WThing *thing);

extern bool do_activate_workspace(WScreen *scr, WWorkspace *ws);
extern void switch_workspace(WWorkspace *ws);
extern bool switch_workspace_nth(WScreen *scr, int num);
extern bool switch_workspace_nth2(int scrnum, int num);
extern void switch_workspace_next(WScreen *scr);
extern void switch_workspace_prev(WScreen *scr);
extern bool switch_workspace_name(const char *name);
extern void switch_workspace_next_n(WScreen *scr, int num);
extern void switch_workspace_prev_n(WScreen *scr, int num);

extern WWorkspace *lookup_workspace(const char *name);
extern int complete_workspace(char *nam, char ***cp_ret, char **beg);

extern void add_workspace_window(WWorkspace *ws, WWindow *nw);
extern void workspace_remove_window(WWorkspace *ws, WWindow *wwin);
extern bool remove_split(WWorkspace *ws, WWsSplit *split);
extern WWindow *find_current(WWorkspace *ws);

#endif /* INCLUDED_WORKSPACE_H */
