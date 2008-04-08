/*
 * ion/config.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_CONFIG_H
#define INCLUDED_CONFIG_H

#include "../version.h"


/* Behaviour-controlling booleans.
 *
 * CF_NO_LOCK_HACK
 *  Disable hack to ignore states of evil locking modifier keys.
 *
 * CF_SWITCH_NEW_CLIENTS
 *  Switch new clients visible
 *
 * CF_WARP
 *  Warp pointer to newly focused frame
 * 
 * CF_WARP_WS
 *  When CF_WARP is enabled, warp pointer to last on active frame
 *  on workspace when switching workspace.
 * 
 * CF_PLACEMENT_GEOM
 *  Place new windows that have top-left coordinate set in the frame
 *  under that coordinate.
 * 
 * CF_NO_SETPGID
 *  Don't setpgid(0, 0) on forked processes.
 */

/*#define CF_NO_LOCK_HACK*/
#define CF_SWITCH_NEW_CLIENTS
/*#define CF_WARP*/
/*#define CF_WARP_WS*/
/*#define CF_NO_SETPGID*/

/* You don't modify these
 */

#ifndef ETCDIR
#define ETCDIR "/etc/"
#endif


/* Configurable
 */

#define CF_MAX_MOVERES_STR_SIZE 32
#define CF_MOVERES_WIN_X 5
#define CF_MOVERES_WIN_Y 5

#define CF_DRAG_TRESHOLD 2
#define CF_DBLCLICK_DELAY 250
#define CF_RESIZE_DELAY 1500
#define CF_EDGE_RESISTANCE 16
#define CF_STEP_SIZE 16
#define CF_CORNER_SIZE (16+8)

#define CF_TAB_TEXT_ALIGN ALIGN_CENTER

#define CF_FALLBACK_FONT_NAME "fixed"

#define CF_MIN_WIDTH 20
#define CF_MIN_HEIGHT 3

/* Windows whose (x, y)-coordinates are less than this much from both
 * top and left border of the screen are automatically considered stubborn.
 */
#define CF_STUBBORN_TRESH 1

/* Cursors
 */

#define CF_CURSOR_DEFAULT XC_left_ptr
#define CF_CURSOR_RESIZE XC_sizing
#define CF_CURSOR_MOVE XC_fleur
#define CF_CURSOR_DRAG XC_cross
#define CF_CURSOR_WAITKEY XC_icon

#define CF_XTESTFAKEKEY_DELAY 10

#endif /* INCLUDED_CONFIG_H */
