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

#ifndef DOWNLOADQUEUE_H_
#define DOWNLOADQUEUE_H_

#include <SDL/SDL.h>
#include <glib-2.0/glib.h>

#include "tile.h"

GQueue* createQueue();
void enqueue(GQueue* queue, gpointer data);
gpointer dequeue(GQueue* queue);
void deleteElems(GQueue * queue, gpointer data);
void purgeQueue(GQueue *queue);
int queueSize(GQueue * queue);

#endif /* DOWNLOADQUEUE_H_ */
