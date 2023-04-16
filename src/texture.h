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

#ifndef TEXTURE_H_
#define TEXTURE_H_

GLuint createTexture(GLushort * pixels4444, int w, int h, int isTile);
extern double avgR, avgG, avgB, clearR, clearG, clearB;
void deleteTexture(GLuint texture);

#endif /* TEXTURE_H_ */
