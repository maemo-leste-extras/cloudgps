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

#include "navigation.h"

void updateCurrentDirection(double distance, int minIdx) {
	canvas.navigationStatus.currentDirection = NULL;
	int i;
	if (distance < options.outsideRouteToleranceMeters) {
		for (i = 0; i < canvas.route.directions -> len; i++) {
			RouteDirection direction = g_array_index(canvas.route.directions, RouteDirection, i);
			if (direction.position + i > minIdx) {
				canvas.navigationStatus.currentDirection = &g_array_index(canvas.route.directions, RouteDirection, i);
				canvas.navigationStatus.currentDirectionIdx = i;
				break;
			}
		}
		if (canvas.navigationStatus.currentDirection == NULL) {
			canvas.navigationStatus.currentDirection = &g_array_index(canvas.route.directions, RouteDirection, canvas.route.directions -> len - 1);
		}
	}
}

double calculateDistanceToNextDirection(int minIdx) {
	double result = 0;
	int i, upperBound;
	if (canvas.navigationStatus.currentDirectionIdx < canvas.route.directions -> len - 1) {
		upperBound = canvas.navigationStatus.currentDirection -> position + canvas.navigationStatus.currentDirectionIdx - 1;
	} else {
		upperBound = canvas.route.optimizedRoute.lineStripTileCoordinates -> len - 2;
	}
	TileCoordinate tc1, tc2;
	WorldCoordinate wc1, wc2;
	if (minIdx < canvas.route.optimizedRoute.lineStripTileCoordinates -> len - 1) {
		tc1 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, minIdx + 1);
	} else {
		tc1 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, minIdx);
	}
	toWorldCoordinate(&tc1, &wc1);
	result += getDistance(&wc1, &canvas.navigationStatus.snapped);

	for (i = minIdx + 1; i <= upperBound; i++) {
		tc1 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, i);
		tc2 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, i + 1);
		toWorldCoordinate(&tc1, &wc1);
		toWorldCoordinate(&tc2, &wc2);
		result += getDistance(&wc1, &wc2);
	}
	return result;
}

int getRoundedValue(double value) {
	if (value > 10000) {
		return ((int) value / 1000) * 1000;
	} else if (value > 500) {
		return ((int) value / 100) * 100;
	} else if (value > 100) {
		return ((int) value / 50) * 50;
	} else {
		return ((int) value / 10) * 10;
	}
}

void updateDistances(int minIdx) {
	canvas.navigationStatus.distanceToTarget = 0;
	if (canvas.navigationStatus.currentDirection != NULL) {
		if (canvas.navigationStatus.currentDirectionIdx < canvas.route.directions -> len) {
			canvas.navigationStatus.distanceToTarget = canvas.route.lengthMeters - g_array_index(canvas.route.directions, RouteDirection, canvas.navigationStatus.currentDirectionIdx).metersFromStart;
		}
		double distanceToNextDirection = calculateDistanceToNextDirection(minIdx);
		canvas.navigationStatus.distanceToTarget += distanceToNextDirection;
		canvas .navigationStatus.roundedDistanceToNextDirection = getRoundedValue(distanceToNextDirection);
	}
}

void updateNavigation() {
	if (canvas.route.optimizedRoute.lineStripTileCoordinates != NULL) {
		int i;
		double minDistance = INFINITY;
		double minSnapx, minSnapy;
		int minIdx = -1;

		TileCoordinate currentPosTc;
		Point currentPos;
		// TODO canvas.currentPos instead of canvas.center
		toTileCoordinate(&canvas.center, &currentPosTc, canvas.route.optimizedRoute.lineStripZoom);
		toScreenCoordinate(&currentPosTc, &currentPos);

		for (i = 0; i < canvas.route.optimizedRoute.lineStripTileCoordinates -> len; i++) {
			TileCoordinate tc1 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, i);
			TileCoordinate tc2 = g_array_index(canvas.route.optimizedRoute.lineStripTileCoordinates, TileCoordinate, i + 1);
			Point sc1;
			Point sc2;
			toScreenCoordinate(&tc1, &sc1);
			toScreenCoordinate(&tc2, &sc2);

			double distance = distancePointFromLineSquared(currentPos.x, currentPos.y, sc1.x, sc1.y, sc2.x, sc2.y);
			if (distance <= minDistance) {
				minIdx = i;
				minDistance = distance;
				minSnapx = snapx;
				minSnapy = snapy;
			}
		}

		Point snap;
		TileCoordinate snapTc;
		WorldCoordinate snapWc;

		snap.x = minSnapx;
		snap.y = minSnapy;

		// TODO canvas.currentPos instead of canvas.center
		fromScreenCoordinate(&snap, &snapTc);
		snapTc.zoom = canvas.route.optimizedRoute.lineStripZoom;
		toWorldCoordinate(&snapTc, &snapWc);

		double distance = getDistance(&snapWc, &canvas.center);

		canvas.navigationStatus.snapped = snapWc;
		canvas.navigationStatus.snappedDistance = distance;
		if (minIdx >= 0) {
			canvas.navigationStatus.currentLineIdx = minIdx;
			updateCurrentDirection(distance, minIdx);
			updateDistances(minIdx);
		}
	} else {
		canvas.navigationStatus.currentDirection = NULL;
	}
}
