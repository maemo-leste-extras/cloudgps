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

/*
 * The code in this file comes from iPhone Ray Picking gluUnProject sample:
 * http://blog.nova-box.com/2010/05/iphone-ray-picking-glunproject-sample.html
 *
 */

#include <string.h>
#include <math.h>
#if defined(N900) || defined(N950)
#include <GLES/gl.h>
#include <GLES/glext.h>
#else
#include <GL/gl.h>
#endif

//
//  glu.c
//  RayPicking Sample
//
//  Created by Nova-Box on 5/24/10.
//
//  This document contains programming examples.
//
//  Nova-box grants you a nonexclusive copyright license to use all programming code examples
//  from which you can generate similar function tailored to your own specific needs.
//
//  All sample code is provided by Nova-box for illustrative purposes only.
//  These examples have not been thoroughly tested under all conditions.
//  Nova-box, therefore, cannot guarantee or imply reliability, serviceability, or function of these programs.
//
//  All programs contained herein are provided to you "AS IS" without any warranties of any kind.
//  The implied warranties of non-infringement, merchantability and fitness for a particular purpose are expressly disclaimed.

GLfloat modelMatrix[16];
GLfloat projectionMatrix[16];
GLint viewport[4];


void storeMatricesAndViewPort() {
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelMatrix);
    glGetIntegerv(GL_VIEWPORT, viewport);
}


int invert_matrix(const GLfloat * m, GLfloat * out)
{
    /* NB. OpenGL Matrices are COLUMN major. */
#define SWAP_ROWS(a, b) { GLfloat *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

    GLfloat wtmp[4][8];
    GLfloat m0, m1, m2, m3, s;
    GLfloat *r0, *r1, *r2, *r3;

    r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

    r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
    r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
    r0[4] = 1.f, r0[5] = r0[6] = r0[7] = 0.f,
    r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
    r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
    r1[5] = 1.f, r1[4] = r1[6] = r1[7] = 0.f,
    r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
    r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
    r2[6] = 1.f, r2[4] = r2[5] = r2[7] = 0.f,
    r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
    r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
    r3[7] = 1.f, r3[4] = r3[5] = r3[6] = 0.f;

    /* choose pivot - or die */
    if (fabsf(r3[0]) > fabsf(r2[0]))
        SWAP_ROWS(r3, r2);
    if (fabsf(r2[0]) > fabsf(r1[0]))
        SWAP_ROWS(r2, r1);
    if (fabsf(r1[0]) > fabsf(r0[0]))
        SWAP_ROWS(r1, r0);
    if (0.f == r0[0])
        return GL_FALSE;

    /* eliminate first variable     */
    m1 = r1[0] / r0[0];
    m2 = r2[0] / r0[0];
    m3 = r3[0] / r0[0];
    s = r0[1];
    r1[1] -= m1 * s;
    r2[1] -= m2 * s;
    r3[1] -= m3 * s;
    s = r0[2];
    r1[2] -= m1 * s;
    r2[2] -= m2 * s;
    r3[2] -= m3 * s;
    s = r0[3];
    r1[3] -= m1 * s;
    r2[3] -= m2 * s;
    r3[3] -= m3 * s;
    s = r0[4];
    if (s != 0.f) {
        r1[4] -= m1 * s;
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r0[5];
    if (s != 0.f) {
        r1[5] -= m1 * s;
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r0[6];
    if (s != 0.f) {
        r1[6] -= m1 * s;
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r0[7];
    if (s != 0.f) {
        r1[7] -= m1 * s;
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* choose pivot - or die */
    if (fabsf(r3[1]) > fabsf(r2[1]))
        SWAP_ROWS(r3, r2);
    if (fabsf(r2[1]) > fabsf(r1[1]))
        SWAP_ROWS(r2, r1);
    if (0.f == r1[1])
        return GL_FALSE;

    /* eliminate second variable */
    m2 = r2[1] / r1[1];
    m3 = r3[1] / r1[1];
    r2[2] -= m2 * r1[2];
    r3[2] -= m3 * r1[2];
    r2[3] -= m2 * r1[3];
    r3[3] -= m3 * r1[3];
    s = r1[4];
    if (0.f != s) {
        r2[4] -= m2 * s;
        r3[4] -= m3 * s;
    }
    s = r1[5];
    if (0.f != s) {
        r2[5] -= m2 * s;
        r3[5] -= m3 * s;
    }
    s = r1[6];
    if (0.f != s) {
        r2[6] -= m2 * s;
        r3[6] -= m3 * s;
    }
    s = r1[7];
    if (0.f != s) {
        r2[7] -= m2 * s;
        r3[7] -= m3 * s;
    }

    /* choose pivot - or die */
    if (fabs(r3[2]) > fabs(r2[2]))
        SWAP_ROWS(r3, r2);
    if (0.f == r2[2])
        return GL_FALSE;

    /* eliminate third variable */
    m3 = r3[2] / r2[2];
    r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
    r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

    /* last check */
    if (0.f == r3[3])
        return GL_FALSE;

    s = 1.f / r3[3];        /* now back substitute row 3 */
    r3[4] *= s;
    r3[5] *= s;
    r3[6] *= s;
    r3[7] *= s;

    m2 = r2[3];         /* now back substitute row 2 */
    s = 1.f / r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
    r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
    r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
    m0 = r0[3];
    r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
    r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

    m1 = r1[2];         /* now back substitute row 1 */
    s = 1.f / r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
    r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
    r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

    m0 = r0[1];         /* now back substitute row 0 */
    s = 1.f / r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
    r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

    MAT(out, 0, 0) = r0[4];
    MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
    MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
    MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
    MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
    MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
    MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
    MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
    MAT(out, 3, 3) = r3[7];

    return GL_TRUE;

#undef MAT
#undef SWAP_ROWS
}

void inline transform_point(GLfloat out[4], const GLfloat m[16], const GLfloat in[4])
{
#define M(row,col)  m[col*4+row]
    out[0] =
    M(0, 0) * in[0] + M(0, 1) * in[1] + M(0, 2) * in[2] + M(0, 3) * in[3];
    out[1] =
    M(1, 0) * in[0] + M(1, 1) * in[1] + M(1, 2) * in[2] + M(1, 3) * in[3];
    out[2] =
    M(2, 0) * in[0] + M(2, 1) * in[1] + M(2, 2) * in[2] + M(2, 3) * in[3];
    out[3] =
    M(3, 0) * in[0] + M(3, 1) * in[1] + M(3, 2) * in[2] + M(3, 3) * in[3];
#undef M
}

void matmul(GLfloat * product, const GLfloat * a, const GLfloat * b)
{
    /* This matmul was contributed by Thomas Malik */
    GLfloat temp[16];
    GLint i;

#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define T(row,col)  temp[(col<<2)+row]

    /* i-te Zeile */
    for (i = 0; i < 4; i++) {
        T(i, 0) =
        A(i, 0) * B(0, 0) + A(i, 1) * B(1, 0) + A(i, 2) * B(2, 0) + A(i,
                                                                      3) *
        B(3, 0);
        T(i, 1) =
        A(i, 0) * B(0, 1) + A(i, 1) * B(1, 1) + A(i, 2) * B(2, 1) + A(i,
                                                                      3) *
        B(3, 1);
        T(i, 2) =
        A(i, 0) * B(0, 2) + A(i, 1) * B(1, 2) + A(i, 2) * B(2, 2) + A(i,
                                                                      3) *
        B(3, 2);
        T(i, 3) =
        A(i, 0) * B(0, 3) + A(i, 1) * B(1, 3) + A(i, 2) * B(2, 3) + A(i,
                                                                      3) *
        B(3, 3);
    }

#undef A
#undef B
#undef T
    memcpy(product, temp, 16 * sizeof(GLfloat));
}

GLint gluUnProject(GLfloat winx, GLfloat winy, GLfloat winz,
             GLfloat * objx, GLfloat * objy, GLfloat * objz)
{
    /* matrice de transformation */
    GLfloat m[16], A[16];
    GLfloat in[4], out[4];
    out[3] = 0.0;

    /* transformation coordonnees normalisees entre -1 et 1 */
    in[0] = (winx - viewport[0]) * 2 / viewport[2] - 1.f;
    in[1] = (winy - viewport[1]) * 2 / viewport[3] - 1.f;
    in[2] = 2 * winz - 1.f;
    in[3] = 1.f;

    /* calcul transformation inverse */
    matmul(A, projectionMatrix, modelMatrix);
    invert_matrix(A, m);

    /* d'ou les coordonnees objets */
    transform_point(out, m, in);
//    if (out[3] == 0.f)
//        return GL_FALSE;
    *objx = out[0] / out[3];
    *objy = out[1] / out[3];
    *objz = out[2] / out[3];
    return GL_TRUE;
}
