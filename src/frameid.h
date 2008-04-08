/*
 * ion/frameid.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_FRAMEID_H
#define INCLUDED_FRAMEID_H

#include "frame.h"

#define FRAME_ID_START_CLIENT 256
#define FRAME_ID_REORDER_INTERVAL 1024

extern WFrame *find_frame_by_id(int id);
extern int use_frame_id(int id);
extern int new_frame_id();
extern void free_frame_id(int id);

#endif /* INCLUDED_FRAMEID_H */
