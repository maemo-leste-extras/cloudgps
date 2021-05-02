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
#include "markerIterator.h"

int beforeFirstCanvasMarker, beforeFirstRouteMarker;
GList* canvasMarkerIterator;
GList* routeMarkerIterator;

ScreenMarker* markerIteratorNext() {
	if (beforeFirstCanvasMarker && canvasMarkerIterator != NULL) {
		beforeFirstCanvasMarker = FALSE;
		return canvasMarkerIterator -> data;
	}
	if (canvasMarkerIterator != NULL) {
		canvasMarkerIterator = canvasMarkerIterator -> next;
		if (canvasMarkerIterator != NULL) {
			return canvasMarkerIterator -> data;
		}
	}

	if (beforeFirstRouteMarker && routeMarkerIterator != NULL) {
		beforeFirstRouteMarker = FALSE;
		return routeMarkerIterator -> data;
	}
	if (routeMarkerIterator != NULL) {
		routeMarkerIterator = routeMarkerIterator -> next;
		if (routeMarkerIterator != NULL) {
			return routeMarkerIterator -> data;
		}
	}
	return NULL;
}

ScreenMarker* resetMarkerIterator() {
	if (canvasMarkerIterator != NULL || routeMarkerIterator != NULL) {
		fprintf(stderr, "Warning: resetting iterator while it is used\n");
	}
	canvasMarkerIterator = canvas.markers;
	routeMarkerIterator = canvas.routeMarkers;
	beforeFirstCanvasMarker = TRUE;
	beforeFirstRouteMarker = TRUE;
	return markerIteratorNext();
}
