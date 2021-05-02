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

#include "geometry.h"

double snapx, snapy;

double distancePointToPointSquared(double x1, double y1, double x2, double y2) {
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

double distancePointFromLineSquared(double px, double py, double lx1, double ly1, double lx2, double ly2) {
	double xd = lx2 - lx1, yd = ly2 - ly1;
	if (xd == 0 && yd == 0) {
		snapx = lx1;
		snapy = ly1;
		return distancePointToPointSquared(px, py, lx1, ly1);
	}
	double u = ((px - lx1) * xd + (py - ly1) * yd) / (xd * xd + yd * yd);
	if (u < 0) {
		snapx = lx1;
		snapy = ly1;
	} else if (u > 1) {
		snapx = lx2;
		snapy = ly2;
	} else {
		snapx = lx1 + u * xd;
		snapy = ly1 + u * yd;
	}
	return distancePointToPointSquared(px, py, snapx, snapy);
}

GLfloat pointDistance(Point* p1, Point *p2) {
	GLfloat dx = p2 -> x - p1 -> x;
	GLfloat dy = p2 -> y - p1 -> y;
	return sqrt(dx * dx + dy * dy);
}
