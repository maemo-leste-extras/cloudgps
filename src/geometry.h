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


#ifndef GEOMETRY_H_
#define GEOMETRY_H_

#include "main.h"

extern double snapx, snapy;

double distancePointToPointSquared(double x1, double y1, double x2, double y2);
double distancePointFromLineSquared(double px, double py, double lx1, double ly1, double lx2, double ly2);
GLfloat pointDistance(Point* p1, Point *p2);

#endif /* GEOMETRY_H_ */
