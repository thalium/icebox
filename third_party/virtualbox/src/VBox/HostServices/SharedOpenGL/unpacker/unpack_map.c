/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "state/cr_limits.h"


void crUnpackMap2d(void)
{
    GLenum target = READ_DATA(sizeof(int) + 0, GLenum);
    GLdouble u1 = READ_DOUBLE(sizeof(int) + 4);
    GLdouble u2 = READ_DOUBLE(sizeof(int) + 12);
    GLint ustride = READ_DATA(sizeof(int) + 20, GLint);
    GLint uorder = READ_DATA(sizeof(int) + 24, GLint);
    GLdouble v1 = READ_DOUBLE(sizeof(int) + 28);
    GLdouble v2 = READ_DOUBLE(sizeof(int) + 36);
    GLint vstride = READ_DATA(sizeof(int) + 44, GLint);
    GLint vorder = READ_DATA(sizeof(int) + 48, GLint);
    GLdouble *points = DATA_POINTER(sizeof(int) + 52, GLdouble);
    GLint cbMax;

    if (uorder < 1 || uorder > CR_MAX_EVAL_ORDER ||
        vorder < 1 || vorder > CR_MAX_EVAL_ORDER ||
        ustride < 1 || ustride > INT32_MAX / (ssize_t)sizeof(double) / uorder / 8 ||
        vstride < 1 || vstride > INT32_MAX / (ssize_t)sizeof(double) / vorder / 8 )
    {
        crError("crUnpackMap2d: parameters out of range");
        return;
    }

    cbMax = (ustride * uorder + vstride * vorder) * sizeof(double);
    if (!DATA_POINTER_CHECK(cbMax - 1))
    {
        crError("crUnpackMap2d: parameters out of range");
        return;
    }

    cr_unpackDispatch.Map2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    INCR_VAR_PTR();
}

void crUnpackMap2f(void)
{
    GLenum target = READ_DATA(sizeof(int) + 0, GLenum);
    GLfloat u1 = READ_DATA(sizeof(int) + 4, GLfloat);
    GLfloat u2 = READ_DATA(sizeof(int) + 8, GLfloat);
    GLint ustride = READ_DATA(sizeof(int) + 12, GLint);
    GLint uorder = READ_DATA(sizeof(int) + 16, GLint);
    GLfloat v1 = READ_DATA(sizeof(int) + 20, GLfloat);
    GLfloat v2 = READ_DATA(sizeof(int) + 24, GLfloat);
    GLint vstride = READ_DATA(sizeof(int) + 28, GLint);
    GLint vorder = READ_DATA(sizeof(int) + 32, GLint);
    GLfloat *points = DATA_POINTER(sizeof(int) + 36, GLfloat);
    GLint cbMax;

    if (uorder < 1 || uorder > CR_MAX_EVAL_ORDER ||
        vorder < 1 || vorder > CR_MAX_EVAL_ORDER ||
        ustride < 1 || ustride > INT32_MAX / (ssize_t)sizeof(float) / uorder / 8 ||
        vstride < 1 || vstride > INT32_MAX / (ssize_t)sizeof(float) / vorder / 8)
    {
        crError("crUnpackMap2d: parameters out of range");
        return;
    }

    cbMax = (ustride * uorder + vstride * vorder) * sizeof(float);
    if (!DATA_POINTER_CHECK(cbMax - 1))
    {
        crError("crUnpackMap2f: parameters out of range");
        return;
    }

    cr_unpackDispatch.Map2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);

    INCR_VAR_PTR();
}

void crUnpackMap1d(void)
{
    GLenum target = READ_DATA(sizeof(int) + 0, GLenum);
    GLdouble u1 = READ_DOUBLE(sizeof(int) + 4);
    GLdouble u2 = READ_DOUBLE(sizeof(int) + 12);
    GLint stride = READ_DATA(sizeof(int) + 20, GLint);
    GLint order = READ_DATA(sizeof(int) + 24, GLint);
    GLdouble *points = DATA_POINTER(sizeof(int) + 28, GLdouble);
    GLint cbMax;

    if (order < 1 || order > CR_MAX_EVAL_ORDER ||
        stride < 1 || stride > INT32_MAX / (ssize_t)sizeof(double) / order / 8)
    {
        crError("crUnpackMap1d: parameters out of range");
        return;
    }

    cbMax = stride * order * sizeof(double);
    if (!DATA_POINTER_CHECK(cbMax - 1))
    {
        crError("crUnpackMap1d: parameters out of range");
        return;
    }

    cr_unpackDispatch.Map1d(target, u1, u2, stride, order, points);

    INCR_VAR_PTR();
}

void crUnpackMap1f(void)
{
    GLenum target = READ_DATA(sizeof(int) + 0, GLenum);
    GLfloat u1 = READ_DATA(sizeof(int) + 4, GLfloat);
    GLfloat u2 = READ_DATA(sizeof(int) + 8, GLfloat);
    GLint stride = READ_DATA(sizeof(int) + 12, GLint);
    GLint order = READ_DATA(sizeof(int) + 16, GLint);
    GLfloat *points = DATA_POINTER(sizeof(int) + 20, GLfloat);
    GLint cbMax;

    if (order < 1 || order > CR_MAX_EVAL_ORDER ||
        stride < 1 || stride > INT32_MAX / (ssize_t)sizeof(float) / order / 8)
    {
        crError("crUnpackMap1f: parameters out of range");
        return;
    }

    cbMax = stride * order * sizeof(float);
    if (!DATA_POINTER_CHECK(cbMax - 1))
    {
        crError("crUnpackMap1f: parameters out of range");
        return;
    }

    cr_unpackDispatch.Map1f(target, u1, u2, stride, order, points);
    INCR_VAR_PTR();
}
