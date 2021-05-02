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

#include "downloadQueue.h"

SDL_mutex *mutex = NULL;
SDL_sem *sem = NULL;

GQueue* createQueue() {
    GQueue* queue = g_queue_new();
    if (queue == NULL) {
        fprintf(stderr, "memory allocation error in createQueue\n");
        return NULL;
    }
    if (mutex == NULL) {
        mutex = SDL_CreateMutex();
    }
    if (sem == NULL) {
        sem = SDL_CreateSemaphore(0);
        if (sem == NULL) {
        	fprintf(stderr, "SDL_CreateSemaphore\n");
        	exit(1);
        }
    }
//    fprintf(stderr, "createQueue success\n");
    return queue;
}

void enqueue(GQueue* queue, gpointer data) {
    if (SDL_mutexP(mutex) == -1) {
        fprintf(stderr, "mutexP failed in enqueue\n");
        return;
    }
//#define DISABLE_NON_ZOOM_TILES
#ifdef DISABLE_NON_ZOOM_TILES
    t_tile* tile = (t_tile*) data;
    if (tile -> zoomHelper == 0) {
		if (SDL_mutexV(mutex) == -1) {
			fprintf(stderr, "mutexV failed in enqueue\n");
		}
        fprintf(stderr, "skipping non helper tile\n");
    	return;
    }
#endif
    g_queue_push_tail(queue, data);
    if (SDL_mutexV(mutex) == -1) {
        fprintf(stderr, "mutexV failed in enqueue\n");
    }
    if (SDL_SemPost(sem) == -1) {
        fprintf(stderr, "SemPost failed in enqueue\n");
        return;
    }
}

long lastMinPriorityDequeue = 0;
gpointer dequeue(GQueue* queue) {
//	long start = SDL_GetTicks();
    if (queue -> length == 0) {
        return NULL;
    }
    if (SDL_mutexP(mutex) == -1) {
        fprintf(stderr, "mutexP failed in dequeue\n");
        return NULL;
    }
    if (queue -> length == 0) {
        if (SDL_mutexV(mutex) == -1) {
            fprintf(stderr, "mutexV failed in dequeue (1)\n");
        }
        return NULL;
    }
    gpointer data = NULL;

    GList * elem = queue -> head;
    if (canvas.destScale != 0.0) {
		while (elem != NULL) {
			t_tile * tile = (t_tile*) elem -> data;
			if (tile -> zoomHelper) {
				data = elem->data;
				g_queue_remove_all(queue, tile);
				break;
			}
			elem = elem -> next;
		}
    }

    if (data == NULL) {
		elem = queue -> head;
        while (elem != NULL) {
            t_tile * tile = (t_tile*) elem -> data;
            if (tile -> visible) {
                data = elem->data;
                g_queue_remove_all(queue, tile);
                break;
            }
            elem = elem -> next;
        }
    }
    if (data == NULL) {
        // TODO return tile nearest to visible tile
        data = g_queue_pop_head(queue);
    }
	if (SDL_mutexV(mutex) == -1) {
		fprintf(stderr, "mutexV failed in dequeue(2)\n");
	}

//    if (data != NULL) {
//    	t_tile* tile = (t_tile*)data;
//    	tile -> state = STATE_LOADING;
//    	if (tile -> zoomHelper == 0 && tile -> visible == 0) {
//    		long tmp = SDL_GetTicks() - lastMinPriorityDequeue;
//    		lastMinPriorityDequeue = SDL_GetTicks();
//    		if (tmp < 10) {
//    			SDL_Delay(10);
//    		}
//    	}
//    }
//    long end = SDL_GetTicks();
//    if (end - start > 1) {
//		fprintf(stderr, "dequeue took %d ms\n", end - start);
//    }
    return data;
}

void deleteElems(GQueue * queue, gpointer data) {
    if (SDL_mutexP(mutex) == -1) {
        fprintf(stderr, "mutexP failed in deleteElem\n");
        return;
    }
    g_queue_remove_all(queue, data);

    if (SDL_mutexV(mutex) == -1) {
        fprintf(stderr, "mutexV failed in deleteElem\n");
    }
}

void purgeQueue(GQueue *queue) {
    if (SDL_mutexP(mutex) == -1) {
        fprintf(stderr, "mutexP failed in purgeQueue\n");
        return;
    }

    g_queue_clear(queue);

    if (SDL_mutexV(mutex) == -1) {
        fprintf(stderr, "mutexV failed in purgeQueue\n");
    }
}

int queueSize(GQueue * queue) {
    return queue -> length;
}
