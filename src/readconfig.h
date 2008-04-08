/*
 * ion/readconfig.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef INCLUDED_READCONFIG_H
#define INCLUDED_READCONFIG_H

#include "common.h"
#include <libtu/parser.h>

extern bool read_config(const char *cfgfile);
extern bool read_config_for(const char *module,
							const ConfOpt *opts);

#endif /* INCLUDED_READCONFIG_H */
