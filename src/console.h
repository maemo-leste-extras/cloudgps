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


#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-2.0/glib.h>

#include "main.h"

#define CONSOLE_MAX_LINE_LENGHT 200
#define CONSOLE_MAX_LINES 200

#define LINE_ON_SCREEN_MILLIS 4000

typedef struct {
	char text[CONSOLE_MAX_LINE_LENGHT+1];
	GLfloat r, g, b;
	long addedMillis;
} ConsoleLine;

extern int consolePosition;
extern GQueue* consoleLines;

void addConsoleLine(char* text, int r, int g, int b);
void deleteOldConsoleLines();

#endif /* CONSOLE_H_ */
