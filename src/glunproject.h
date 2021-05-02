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

#ifndef GLUNPROJECT_H_
#define GLUNPROJECT_H_

void storeMatricesAndViewPort();
GLint gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz, GLfloat * objx, GLfloat * objy, GLfloat * objz);


#endif /* GLUNPROJECT_H_ */
