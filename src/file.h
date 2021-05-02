/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Damian Waradzyn
 */

#ifndef FILE_H_
#define FILE_H_

#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <glib-2.0/glib.h>

void ensureDirExists(char *filename);

#endif /* FILE_H_ */
