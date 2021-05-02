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

#include "console.h"

extern long nowMillis;
int consolePosition;
GQueue* consoleLines = NULL;

void addConsoleLine(char* text, int r, int g, int b) {
	ConsoleLine* line = (ConsoleLine*) malloc(sizeof(ConsoleLine));
	if (line == NULL) {
		fprintf(stderr, "Memory allocation error in addConsoleLine\n");
		exit(1);
	}

	strncpy(line -> text, text, CONSOLE_MAX_LINE_LENGHT);
	line -> r = r;
	line -> g = g;
	line -> b = b;
	line -> addedMillis = nowMillis;

	if (consoleLines == NULL) {
		consoleLines = g_queue_new();
	}
	g_queue_push_tail(consoleLines, line);
}
void deleteOldConsoleLines() {
	if (consoleLines == NULL) {
		return;
	}
	while (g_queue_get_length(consoleLines) > 0) {
		ConsoleLine* line = (ConsoleLine*) g_queue_peek_head(consoleLines);
		if (line -> addedMillis + LINE_ON_SCREEN_MILLIS < nowMillis) {
			g_queue_pop_head(consoleLines);
			free(line);
			consolePosition += 15;
		} else {
			break;
		}
	}
}
