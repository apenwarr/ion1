/*
 * ion/complete_file.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2001. 
 * See the included file LICENSE for details.
 */

#ifndef EDLN_COMPLETE_FILE_H
#define EDLN_COMPLETE_FILE_H

int complete_file(char *pathname, char ***avp, char **beg);
int complete_file_with_path(char *pathname, char ***avp, char **beg);

#endif /* EDLN_COMPLETE_FILE_H */
