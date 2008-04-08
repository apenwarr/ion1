/*
 * ion/splitmisc.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_SPLITMISC_H
#define INCLUDED_SPLITMISC_H

#include "common.h"
#include "split.h"
#include "frame.h"
#include "workspace.h"

extern WFrame *find_sister_frame(WFrame *frame);
extern WFrame *find_frame_at(WWorkspace *ws, int x, int y);

#endif /* INCLUDED_SPLITMISC_H */

