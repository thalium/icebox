/** @file
 * VirtualBox OpenGL command pack/unpack header
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef ___VBox_HostService_VBoxOGLOp_h
#define ___VBox_HostService_VBoxOGLOp_h

#include <iprt/types.h>

#ifdef VBOX_OGL_GUEST_SIDE
/************************************************************************************************************
 * Guest side macro's for packing OpenGL function calls into the command buffer.                            *
 *                                                                                                          *
 ************************************************************************************************************/

#define VBOX_OGL_NAME_PREFIX(Function)                    gl##Function

#define OGL_CMD(op, numpar, size)                                                           \
    VBoxCmdStart(VBOX_OGL_OP_##op, numpar, size);

#define OGL_PARAM(val, size)                                                                \
    VBoxCmdSaveParameter((uint8_t *)&val, size);

#define OGL_MEMPARAM(ptr, size)                                                             \
    VBoxCmdSaveMemParameter((uint8_t *)ptr, size);

#define OGL_CMD_END(op)                                                                     \
    VBoxCmdStop(VBOX_OGL_OP_##op);


#define VBOX_OGL_GEN_OP(op)                                                                 \
    OGL_CMD(op, 0, 0);                                                                      \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP1(op, p1)                                                            \
    OGL_CMD(op, 1, sizeof(p1));                                                             \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP2(op, p1, p2)                                                        \
    OGL_CMD(op, 2, sizeof(p1)+sizeof(p2));                                                  \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP3(op, p1, p2, p3)                                                    \
    OGL_CMD(op, 3, sizeof(p1)+sizeof(p2)+sizeof(p3));                                       \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP4(op, p1, p2, p3, p4)                                                \
    OGL_CMD(op, 4, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4));                            \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_PARAM(p4, sizeof(p4));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP5(op, p1, p2, p3, p4, p5)                                            \
    OGL_CMD(op, 5, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5));                 \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_PARAM(p4, sizeof(p4));                                                              \
    OGL_PARAM(p5, sizeof(p5));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP6(op, p1, p2, p3, p4, p5, p6)                                        \
    OGL_CMD(op, 6, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6));      \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_PARAM(p4, sizeof(p4));                                                              \
    OGL_PARAM(p5, sizeof(p5));                                                              \
    OGL_PARAM(p6, sizeof(p6));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP7(op, p1, p2, p3, p4, p5, p6, p7)                                    \
    OGL_CMD(op, 7, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+sizeof(p7));    \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_PARAM(p4, sizeof(p4));                                                              \
    OGL_PARAM(p5, sizeof(p5));                                                              \
    OGL_PARAM(p6, sizeof(p6));                                                              \
    OGL_PARAM(p7, sizeof(p7));                                                              \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP8(op, p1, p2, p3, p4, p5, p6, p7, p8)                                \
    OGL_CMD(op, 8, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+sizeof(p7)+sizeof(p8));    \
    OGL_PARAM(p1, sizeof(p1));                                                              \
    OGL_PARAM(p2, sizeof(p2));                                                              \
    OGL_PARAM(p3, sizeof(p3));                                                              \
    OGL_PARAM(p4, sizeof(p4));                                                              \
    OGL_PARAM(p5, sizeof(p5));                                                              \
    OGL_PARAM(p6, sizeof(p6));                                                              \
    OGL_PARAM(p7, sizeof(p7));                                                              \
    OGL_PARAM(p8, sizeof(p8));                                                              \
    OGL_CMD_END(op);


/* last parameter is a memory block */
#define VBOX_OGL_GEN_OP1PTR(op, size, p1ptr)                                                        \
    OGL_CMD(op, 1, size);                                                                         \
    OGL_MEMPARAM(p1ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP2PTR(op, p1, size, p2ptr)                                                    \
    OGL_CMD(op, 2, sizeof(p1)+size);                                                              \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_MEMPARAM(p2ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP3PTR(op, p1, p2, size, p3ptr)                                                \
    OGL_CMD(op, 3, sizeof(p1)+sizeof(p2)+size);                                                   \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_MEMPARAM(p3ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP4PTR(op, p1, p2, p3, size, p4ptr)                                            \
    OGL_CMD(op, 4, sizeof(p1)+sizeof(p2)+sizeof(p3)+size);                                        \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_MEMPARAM(p4ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP5PTR(op, p1, p2, p3, p4, size, p5ptr)                                        \
    OGL_CMD(op, 5, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+size);                             \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_MEMPARAM(p5ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP6PTR(op, p1, p2, p3, p4, p5, size, p6ptr)                                    \
    OGL_CMD(op, 6, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+size);                  \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_PARAM(p5, sizeof(p5));                                                                     \
    OGL_MEMPARAM(p6ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP7PTR(op, p1, p2, p3, p4, p5, p6, size, p7ptr)                                \
    OGL_CMD(op, 7, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+size);       \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_PARAM(p5, sizeof(p5));                                                                     \
    OGL_PARAM(p6, sizeof(p6));                                                                     \
    OGL_MEMPARAM(p7ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP8PTR(op, p1, p2, p3, p4, p5, p6, p7, size, p8ptr)                            \
    OGL_CMD(op, 8, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+sizeof(p7)+size);       \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_PARAM(p5, sizeof(p5));                                                                     \
    OGL_PARAM(p6, sizeof(p6));                                                                     \
    OGL_PARAM(p7, sizeof(p7));                                                                     \
    OGL_MEMPARAM(p8ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP9PTR(op, p1, p2, p3, p4, p5, p6, p7, p8, size, p9ptr)                        \
    OGL_CMD(op, 9, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+sizeof(p7)+sizeof(p8)+size);       \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_PARAM(p5, sizeof(p5));                                                                     \
    OGL_PARAM(p6, sizeof(p6));                                                                     \
    OGL_PARAM(p7, sizeof(p7));                                                                     \
    OGL_PARAM(p8, sizeof(p8));                                                                     \
    OGL_MEMPARAM(p9ptr, size);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP10PTR(op, p1, p2, p3, p4, p5, p6, p7, p8, p9, size, p10ptr)                  \
    OGL_CMD(op, 10, sizeof(p1)+sizeof(p2)+sizeof(p3)+sizeof(p4)+sizeof(p5)+sizeof(p6)+sizeof(p7)+sizeof(p8)+sizeof(p9)+size);       \
    OGL_PARAM(p1, sizeof(p1));                                                                     \
    OGL_PARAM(p2, sizeof(p2));                                                                     \
    OGL_PARAM(p3, sizeof(p3));                                                                     \
    OGL_PARAM(p4, sizeof(p4));                                                                     \
    OGL_PARAM(p5, sizeof(p5));                                                                     \
    OGL_PARAM(p6, sizeof(p6));                                                                     \
    OGL_PARAM(p7, sizeof(p7));                                                                     \
    OGL_PARAM(p8, sizeof(p8));                                                                     \
    OGL_PARAM(p9, sizeof(p9));                                                                     \
    OGL_MEMPARAM(p10ptr, size);                                                                    \
    OGL_CMD_END(op);


/* two memory blocks */
#define VBOX_OGL_GEN_OP2PTRPTR(op, size1, p1ptr, size2, p2ptr)                                      \
    OGL_CMD(op, 2, size1+size2);                                                                    \
    OGL_MEMPARAM(p1ptr, size1);                                                                     \
    OGL_MEMPARAM(p2ptr, size2);                                                                     \
    OGL_CMD_END(op);

#define VBOX_OGL_GEN_OP3PTRPTR(op, p1, size2, p2ptr, size3, p3ptr)                                  \
    OGL_CMD(op, 3, sizeof(p1)+size2+size3);                                                         \
    OGL_PARAM(p1, sizeof(p1));                                                                      \
    OGL_MEMPARAM(p2ptr, size2);                                                                     \
    OGL_MEMPARAM(p3ptr, size3);                                                                     \
    OGL_CMD_END(op);

/* Note: sync operations always set the last error */
/* sync operation that returns a value */
#define VBOX_OGL_GEN_SYNC_OP_RET(rettype, op)                                                       \
    VBOX_OGL_GEN_OP(op)                                                                             \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP1_RET(rettype, op, p1)                                                  \
    VBOX_OGL_GEN_OP1(op, p1)                                                                        \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP2_RET(rettype, op, p1, p2)                                              \
    VBOX_OGL_GEN_OP2(op, p1, p2)                                                                    \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP3_RET(rettype, op, p1, p2, p3)                                          \
    VBOX_OGL_GEN_OP3(op, p1, p2, p3)                                                                \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP4_RET(rettype, op, p1, p2, p3, p4)                                      \
    VBOX_OGL_GEN_OP4(op, p1, p2, p3, p4)                                                            \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP5_RET(rettype, op, p1, p2, p3, p4, p5)                                  \
    VBOX_OGL_GEN_OP5(op, p1, p2, p3, p4, p5)                                                        \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP6_RET(rettype, op, p1, p2, p3, p4, p5, p6)                              \
    VBOX_OGL_GEN_OP6(op, p1, p2, p3, p4, p5, p6)                                                    \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP7_RET(rettype, op, p1, p2, p3, p4, p5, p6, p7)                          \
    VBOX_OGL_GEN_OP7(op, p1, p2, p3, p4, p5, p6, p7)                                                \
    rettype retval = (rettype)VBoxOGLFlush();


#define VBOX_OGL_GEN_SYNC_OP(op)                                                                    \
    VBOX_OGL_GEN_OP(op)                                                                             \
    VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP1(op, p1)                                                               \
    VBOX_OGL_GEN_OP1(op, p1)                                                                        \
    VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP2(op, p1, p2)                                                           \
    VBOX_OGL_GEN_OP2(op, p1, p2)                                                                    \
    VBoxOGLFlush();


/* Sync operation whose last parameter is a block of memory */
#define VBOX_OGL_GEN_SYNC_OP2_PTR(op, p1, size, p2ptr)                                              \
    VBOX_OGL_GEN_OP2PTR(op, p1, size, p2ptr);                                                       \
    VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP5_PTR(op, p1, p2, p3, p4, size, p5ptr)                                  \
    VBOX_OGL_GEN_OP2PTR(op, p1, p2, p3, p4, size, p5ptr);                                           \
    VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP6_PTR(op, p1, p2, p3, p4, p5, size, p6ptr)                              \
    VBOX_OGL_GEN_OP6PTR(op, p1, p2, p3, p4, p5, size, p6ptr);                                       \
    VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP7_PTR(op, p1, p2, p3, p4, p5, p6, size, p7ptr)                          \
    VBOX_OGL_GEN_OP7PTR(op, p1, p2, p3, p4, p5, p6, size, p7ptr);                                   \
    VBoxOGLFlush();

/* Sync operation whose last parameter is a block of memory in which results are returned */
#define VBOX_OGL_GEN_SYNC_OP1_PASS_PTR(op, size, p1ptr)                                             \
    VBOX_OGL_GEN_OP(op);                                                                            \
    VBoxOGLFlushPtr(p1ptr, size);

#define VBOX_OGL_GEN_SYNC_OP2_PASS_PTR(op, p1, size, p2ptr)                                         \
    VBOX_OGL_GEN_OP1(op, p1);                                                                       \
    VBoxOGLFlushPtr(p2ptr, size);

#define VBOX_OGL_GEN_SYNC_OP3_PASS_PTR(op, p1, p2, size, p3ptr)                                     \
    VBOX_OGL_GEN_OP2(op, p1, p2);                                                                   \
    VBoxOGLFlushPtr(p3ptr, size);

#define VBOX_OGL_GEN_SYNC_OP4_PASS_PTR(op, p1, p2, p3, size, p4ptr)                                 \
    VBOX_OGL_GEN_OP3(op, p1, p2, p3);                                                               \
    VBoxOGLFlushPtr(p4ptr, size);

#define VBOX_OGL_GEN_SYNC_OP5_PASS_PTR(op, p1, p2, p3, p4, size, p5ptr)                             \
    VBOX_OGL_GEN_OP4(op, p1, p2, p3, p4);                                                           \
    VBoxOGLFlushPtr(p5ptr, size);

#define VBOX_OGL_GEN_SYNC_OP6_PASS_PTR(op, p1, p2, p3, p4, p5, size, p6ptr)                         \
    VBOX_OGL_GEN_OP5(op, p1, p2, p3, p4, p5);                                                       \
    VBoxOGLFlushPtr(p6ptr, size);

#define VBOX_OGL_GEN_SYNC_OP7_PASS_PTR(op, p1, p2, p3, p4, p5, p6, size, p7ptr)                     \
    VBOX_OGL_GEN_OP6(op, p1, p2, p3, p4, p5, p6);                                                   \
    VBoxOGLFlushPtr(p7ptr, size);


/* Sync operation whose last parameter is a block of memory and return a value */
#define VBOX_OGL_GEN_SYNC_OP2_PTR_RET(rettype, op, p1, size, p2ptr)                                 \
    VBOX_OGL_GEN_OP2PTR(op, p1, size, p2ptr);                                                       \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP4_PTR_RET(rettype, op, p1, p2, p3, size, p4ptr)                         \
    VBOX_OGL_GEN_OP4PTR(op, p1, p2, p3, size, p4ptr);                                               \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP5_PTR_RET(rettype, op, p1, p2, p3, p4, size, p5ptr)                     \
    VBOX_OGL_GEN_OP5PTR(op, p1, p2, p3, p4, size, p5ptr);                                           \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP6_PTR_RET(rettype, op, p1, p2, p3, p4, p5, size, p6ptr)                 \
    VBOX_OGL_GEN_OP6PTR(op, p1, p2, p3, p4, p5, size, p6ptr);                                       \
    rettype retval = (rettype)VBoxOGLFlush();

#define VBOX_OGL_GEN_SYNC_OP7_PTR_RET(rettype, op, p1, p2, p3, p4, p5, p6, size, p7ptr)             \
    VBOX_OGL_GEN_OP7PTR(op, p1, p2, p3, p4, p5, p6, size, p7ptr);                                   \
    rettype retval = (rettype)VBoxOGLFlush();


/* Sync operation whose last parameter is a block of memory in which results are returned and return a value */
#define VBOX_OGL_GEN_SYNC_OP2_PASS_PTR_RET(rettype, op, p1, size, p2ptr)                            \
    VBOX_OGL_GEN_OP1(op, p1);                                                                       \
    rettype retval = (rettype)VBoxOGLFlushPtr(p2ptr, size);

#define VBOX_OGL_GEN_SYNC_OP4_PASS_PTR_RET(rettype, op, p1, p2, p3, size, p4ptr)                    \
    VBOX_OGL_GEN_OP3(op, p1, p2, p3);                                                               \
    rettype retval = (rettype)VBoxOGLFlushPtr(p4ptr, size);

#define VBOX_OGL_GEN_SYNC_OP5_PASS_PTR_RET(rettype, op, p1, p2, p3, p4, size, p5ptr)                \
    VBOX_OGL_GEN_OP4(op, p1, p2, p3, p4);                                                           \
    rettype retval = (rettype)VBoxOGLFlushPtr(p5ptr, size);

#define VBOX_OGL_GEN_SYNC_OP6_PASS_PTR_RET(rettype, op, p1, p2, p3, p4, p5, size, p6ptr)            \
    VBOX_OGL_GEN_OP5(op, p1, p2, p3, p4, p5);                                                       \
    rettype retval = (rettype)VBoxOGLFlushPtr(p6ptr, size);

#define VBOX_OGL_GEN_SYNC_OP7_PASS_PTR_RET(rettype, op, p1, p2, p3, p4, p5, p6, size, p7ptr)        \
    VBOX_OGL_GEN_OP6(op, p1, p2, p3, p4, p5, p6);                                                   \
    rettype retval = (rettype)VBoxOGLFlushPtr(p7ptr, size);


/* Generate async functions elements in the command queue */
#define GL_GEN_FUNC(Function)                                                       \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (void)                                               \
    {                                                                               \
        VBOX_OGL_GEN_OP(Function);                                                  \
    }

#define GL_GEN_FUNC1(Function, Type)                                                \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a)                                             \
    {                                                                               \
        VBOX_OGL_GEN_OP1(Function, a);                                              \
    }

#define GL_GEN_FUNC1V(Function, Type)                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a)                                             \
    {                                                                               \
        VBOX_OGL_GEN_OP1(Function, a);                                              \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (const Type *v)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP1(Function, v[0]);                                           \
    }                                                                               \

#define GL_GEN_FUNC2(Function, Type)                                                \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b)                                     \
    {                                                                               \
        VBOX_OGL_GEN_OP2(Function, a, b);                                           \
    }

#define GL_GEN_FUNC2V(Function, Type)                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b)                                     \
    {                                                                               \
        VBOX_OGL_GEN_OP2(Function, a, b);                                           \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (const Type *v)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP2(Function, v[0], v[1]);                                     \
    }                                                                               \

#define GL_GEN_FUNC3(Function, Type)                                                \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b, Type c)                             \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, b, c);                                        \
    }

#define GL_GEN_FUNC3V(Function, Type)                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b, Type c)                             \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, b, c);                                        \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (const Type *v)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, v[0], v[1], v[2]);                               \
    }                                                                               \

#define GL_GEN_FUNC4(Function, Type)                                                \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b, Type c, Type d)                     \
    {                                                                               \
        VBOX_OGL_GEN_OP4(Function, a, b, c, d);                                     \
    }

#define GL_GEN_FUNC4V(Function, Type)                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b, Type c, Type d)                     \
    {                                                                               \
        VBOX_OGL_GEN_OP4(Function, a, b, c, d);                                     \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (const Type *v)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP4(Function, v[0], v[1], v[2], v[3]);                         \
    }                                                                               \

#define GL_GEN_FUNC6(Function, Type)                                                \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type a, Type b, Type c, Type d, Type e, Type f)     \
    {                                                                               \
        VBOX_OGL_GEN_OP6(Function, a, b, c, d, e, f);                               \
    }

#define GL_GEN_VPAR_FUNC2(Function, Type1, Type2)                                   \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP2(Function, a, b);                                           \
    }

#define GL_GEN_VPAR_FUNC2V(Function, Type1, Type2)                                  \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b)                                   \
    {                                                                               \
        VBOX_OGL_GEN_OP2(Function, a, b);                                           \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (Type1 a, const Type2 *v)                         \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, v[0], v[1]);                                  \
    }                                                                               \

#define GL_GEN_VPAR_FUNC3(Function, Type1, Type2, Type3)                            \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c)                          \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, b, c);                                        \
    }

#define GL_GEN_VPAR_FUNC3V(Function, Type1, Type2, Type3)                           \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c)                          \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, b, c);                                        \
    }                                                                               \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function)##v (Type1 a, Type2 b, const Type3 *v)                \
    {                                                                               \
        VBOX_OGL_GEN_OP3(Function, a, v[0], v[1]);                                  \
    }                                                                               \

#define GL_GEN_VPAR_FUNC4(Function, Type1, Type2, Type3, Type4)                     \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d)                 \
    {                                                                               \
        VBOX_OGL_GEN_OP4(Function, a, b, c, d);                                     \
    }

#define GL_GEN_VPAR_FUNC5(Function, Type1, Type2, Type3, Type4, Type5)              \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d, Type5 e)        \
    {                                                                               \
        VBOX_OGL_GEN_OP5(Function, a, b, c, d, e);                                  \
    }

#define GL_GEN_VPAR_FUNC6(Function, Type1, Type2, Type3, Type4, Type5, Type6)       \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d, Type5 e, Type6 f)        \
    {                                                                               \
        VBOX_OGL_GEN_OP6(Function, a, b, c, d, e, f);                               \
    }

#define GL_GEN_VPAR_FUNC7(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7)       \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d, Type5 e, Type6 f, Type7 g)        \
    {                                                                               \
        VBOX_OGL_GEN_OP7(Function, a, b, c, d, e, f, g);                               \
    }

#define GL_GEN_VPAR_FUNC8(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8)       \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d, Type5 e, Type6 f, Type7 g, Type8 h)        \
    {                                                                               \
        VBOX_OGL_GEN_OP8(Function, a, b, c, d, e, f, g, h);                               \
    }

#define GL_GEN_VPAR_FUNC9(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8 ,Type9)       \
    void APIENTRY VBOX_OGL_NAME_PREFIX(Function) (Type1 a, Type2 b, Type3 c, Type4 d, Type5 e, Type6 f, Type7 g, Type8 h, Type9 i)        \
    {                                                                               \
        VBOX_OGL_GEN_OP9(Function, a, b, c, d, e, f, g, h, i);                               \
    }

#elif defined(VBOX_OGL_HOST_SIDE)

/************************************************************************************************************
 * Host side macro's for generating OpenGL function calls from the packed commands in the command buffer.   *
 *                                                                                                          *
 ************************************************************************************************************/

#include <iprt/assert.h>

#define VBOX_OGL_NAME_PREFIX(Function)                    vboxgl##Function

#ifdef VBOX_OGL_CMD_STRICT
#define VBOX_OGL_CHECK_MAGIC(pParVal)                       Assert(pParVal->Magic == VBOX_OGL_CMD_MAGIC)
#else
#define VBOX_OGL_CHECK_MAGIC(pParVal)
#endif

#define OGL_CMD(op, numpar)                                                                 \
    PVBOX_OGL_CMD pCmd = (PVBOX_OGL_CMD)pCmdBuffer;                                         \
    Assert(pCmd->enmOp == VBOX_OGL_OP_##op);                                                \
    Assert(pCmd->cParams == numpar);                                                        \
    uint8_t      *pParam = (uint8_t *)(pCmd+1);                                             \
    NOREF(pParam)

#define OGL_PARAM(Type, par)                                                                \
    Type         par;                                                                       \
    par = *(Type *)pParam;                                                                  \
    pParam += sizeof(par);                                                                  \
    pParam = RT_ALIGN_PT(pParam, VBOX_OGL_CMD_ALIGN, uint8_t *);

#define OGL_MEMPARAM(Type, par)                                                             \
    PVBOX_OGL_VAR_PARAM pParVal = (PVBOX_OGL_VAR_PARAM)pParam;                              \
    Type        *par;                                                                       \
    VBOX_OGL_CHECK_MAGIC(pParVal);                                                          \
    if (pParVal->cbParam)                                                                   \
        par = (Type *)(pParVal+1);                                                          \
    else                                                                                    \
        par = NULL;                                                                         \
    pParam += sizeof(*pParVal) + pParVal->cbParam;                                          \
    pParam = RT_ALIGN_PT(pParam, VBOX_OGL_CMD_ALIGN, uint8_t *);

#define OGL_MEMPARAM_NODEF(Type, par)                                                       \
    pParVal = (PVBOX_OGL_VAR_PARAM)pParam;                                                  \
    Type        *par;                                                                       \
    VBOX_OGL_CHECK_MAGIC(pParVal);                                                          \
    if (pParVal->cbParam)                                                                   \
        par = (Type *)(pParVal+1);                                                          \
    else                                                                                    \
        par = NULL;                                                                         \
    pParam += sizeof(*pParVal) + pParVal->cbParam;                                          \
    pParam = RT_ALIGN_PT(pParam, VBOX_OGL_CMD_ALIGN, uint8_t *);

#define VBOX_OGL_GEN_OP(op)                                                                 \
    OGL_CMD(op, 0);                                                                         \
    gl##op();

#define VBOX_OGL_GEN_OP1(op, Type1)                                                         \
    OGL_CMD(op, 1);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    gl##op(p1);

#define VBOX_OGL_GEN_OP2(op, Type1, Type2)                                                  \
    OGL_CMD(op, 2);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    gl##op(p1, p2);

#define VBOX_OGL_GEN_OP3(op, Type1, Type2, Type3)                                           \
    OGL_CMD(op, 3);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    gl##op(p1, p2, p3);

#define VBOX_OGL_GEN_OP4(op, Type1, Type2, Type3, Type4)                                    \
    OGL_CMD(op, 4);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    OGL_PARAM(Type4, p4);                                                                   \
    gl##op(p1, p2, p3, p4);

#define VBOX_OGL_GEN_OP5(op, Type1, Type2, Type3, Type4, Type5)                             \
    OGL_CMD(op, 5);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    OGL_PARAM(Type4, p4);                                                                   \
    OGL_PARAM(Type5, p5);                                                                   \
    gl##op(p1, p2, p3, p4, p5);

#define VBOX_OGL_GEN_OP6(op, Type1, Type2, Type3, Type4, Type5, Type6)                      \
    OGL_CMD(op, 6);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    OGL_PARAM(Type4, p4);                                                                   \
    OGL_PARAM(Type5, p5);                                                                   \
    OGL_PARAM(Type6, p6);                                                                   \
    gl##op(p1, p2, p3, p4, p5, p6);

#define VBOX_OGL_GEN_OP7(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7)               \
    OGL_CMD(op, 7);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    OGL_PARAM(Type4, p4);                                                                   \
    OGL_PARAM(Type5, p5);                                                                   \
    OGL_PARAM(Type6, p6);                                                                   \
    OGL_PARAM(Type7, p7);                                                                   \
    gl##op(p1, p2, p3, p4, p5, p6, p7);

#define VBOX_OGL_GEN_OP8(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8)        \
    OGL_CMD(op, 8);                                                                         \
    OGL_PARAM(Type1, p1);                                                                   \
    OGL_PARAM(Type2, p2);                                                                   \
    OGL_PARAM(Type3, p3);                                                                   \
    OGL_PARAM(Type4, p4);                                                                   \
    OGL_PARAM(Type5, p5);                                                                   \
    OGL_PARAM(Type6, p6);                                                                   \
    OGL_PARAM(Type7, p7);                                                                   \
    OGL_PARAM(Type8, p8);                                                                   \
    gl##op(p1, p2, p3, p4, p5, p6, p7, p8);


/* last parameter is a memory block */
#define VBOX_OGL_GEN_OP1PTR(op, Type1)                                                          \
    OGL_CMD(op, 1);                                                                             \
    OGL_MEMPARAM(Type1, p1);                                                                        \
    gl##op(p1);

#define VBOX_OGL_GEN_OP2PTR(op, Type1, Type2)                                                   \
    OGL_CMD(op, 2);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_MEMPARAM(Type2, p2);                                                                        \
    gl##op(p1, p2);

#define VBOX_OGL_GEN_OP3PTR(op, Type1, Type2, Type3)                                            \
    OGL_CMD(op, 3);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_MEMPARAM(Type3, p3);                                                                        \
    gl##op(p1, p2, p3);

#define VBOX_OGL_GEN_OP4PTR(op, Type1, Type2, Type3, Type4)                                     \
    OGL_CMD(op, 4);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_MEMPARAM(Type4, p4);                                                                        \
    gl##op(p1, p2, p3, p4);

#define VBOX_OGL_GEN_OP5PTR(op, Type1, Type2, Type3, Type4, Type5)                              \
    OGL_CMD(op, 5);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_MEMPARAM(Type5, p5);                                                                        \
    gl##op(p1, p2, p3, p4, p5);

#define VBOX_OGL_GEN_OP6PTR(op, Type1, Type2, Type3, Type4, Type5, Type6)                       \
    OGL_CMD(op, 6);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_PARAM(Type5, p5);                                                                       \
    OGL_MEMPARAM(Type6, p6);                                                                        \
    gl##op(p1, p2, p3, p4, p5, p6);

#define VBOX_OGL_GEN_OP7PTR(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7)                \
    OGL_CMD(op, 7);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_PARAM(Type5, p5);                                                                       \
    OGL_PARAM(Type6, p6);                                                                       \
    OGL_MEMPARAM(Type7, p7);                                                                        \
    gl##op(p1, p2, p3, p4, p5, p6, p7);

#define VBOX_OGL_GEN_OP8PTR(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8)         \
    OGL_CMD(op, 8);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_PARAM(Type5, p5);                                                                       \
    OGL_PARAM(Type6, p6);                                                                       \
    OGL_PARAM(Type7, p7);                                                                       \
    OGL_MEMPARAM(Type8, p8);                                                                        \
    gl##op(p1, p2, p3, p4, p5, p6, p7, p8);

#define VBOX_OGL_GEN_OP9PTR(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8, Type9)  \
    OGL_CMD(op, 9);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_PARAM(Type5, p5);                                                                       \
    OGL_PARAM(Type6, p6);                                                                       \
    OGL_PARAM(Type7, p7);                                                                       \
    OGL_PARAM(Type8, p8);                                                                       \
    OGL_MEMPARAM(Type9, p9);                                                                        \
    gl##op(p1, p2, p3, p4, p5, p6, p7, p8 ,p9);

#define VBOX_OGL_GEN_OP10PTR(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8, Type9, Type10)                  \
    OGL_CMD(op, 10);                                                                            \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_PARAM(Type3, p3);                                                                       \
    OGL_PARAM(Type4, p4);                                                                       \
    OGL_PARAM(Type5, p5);                                                                       \
    OGL_PARAM(Type6, p6);                                                                       \
    OGL_PARAM(Type7, p7);                                                                       \
    OGL_PARAM(Type8, p8);                                                                       \
    OGL_PARAM(Type9, p9);                                                                       \
    OGL_MEMPARAM(Type10, p10);                                                                       \
    gl##op(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);


/* two memory blocks */
#define VBOX_OGL_GEN_OP2PTRPTR(op, Type1, Type2)                                                \
    OGL_CMD(op, 2);                                                                             \
    OGL_MEMPARAM(Type1, p1);                                                                        \
    OGL_MEMPARAM_NODEF(Type2, p2);                                                                        \
    gl##op(p1, p2);

#define VBOX_OGL_GEN_OP3PTRPTR(op, Type1, Type2, Type3)                                         \
    OGL_CMD(op, 3);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_MEMPARAM(Type2, p2);                                                                        \
    OGL_MEMPARAM_NODEF(Type3, p3);                                                                        \
    gl##op(p1, p2, p3);

/* Note: sync operations always set the last error */
/* sync operation that returns a value */
#define VBOX_OGL_GEN_SYNC_OP_RET(rettype, op)                                                   \
    OGL_CMD(op, 0);                                                                             \
    pClient->lastretval = gl##op();

#define VBOX_OGL_GEN_SYNC_OP1_RET(rettype, op, Type1)                                              \
    OGL_CMD(op, 1);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    pClient->lastretval = gl##op(p1);

#define VBOX_OGL_GEN_SYNC_OP2_RET(rettype, op, Type1, Type2)                                          \
    OGL_CMD(op, 2);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    pClient->lastretval = gl##op(p1, p2);

#define VBOX_OGL_GEN_SYNC_OP3_RET(rettype, op, Type1, Type2, Type3)                                          \
    OGL_CMD(op, 3);                                                                             \
    OGL_PARAM(Type1, p1);                                                                       \
    OGL_PARAM(Type2, p2);                                                                       \
    OGL_MEMPARAM(Type3, p3);                                                                        \
    pClient->lastretval = gl##op(p1, p2, p3);

#define VBOX_OGL_GEN_SYNC_OP(op)                                                                    \
    VBOX_OGL_GEN_OP(op);

#define VBOX_OGL_GEN_SYNC_OP1(op, p1)                                                               \
    VBOX_OGL_GEN_OP1(op, p1);

#define VBOX_OGL_GEN_SYNC_OP2(op, p1, p2)                                                           \
    VBOX_OGL_GEN_OP2(op, p1, p2);


/* Sync operation whose last parameter is a block of memory */
#define VBOX_OGL_GEN_SYNC_OP2_PTR(op, p1, p2ptr)                                                    \
    VBOX_OGL_GEN_OP2PTR(op, p1, p2ptr);

#define VBOX_OGL_GEN_SYNC_OP5_PTR(op, p1, p2, p3, p4, p5ptr)                                        \
    VBOX_OGL_GEN_OP2PTR(op, p1, p2, p3, p4, size, p5ptr);

#define VBOX_OGL_GEN_SYNC_OP6_PTR(op, p1, p2, p3, p4, p5, p6ptr)                                    \
    VBOX_OGL_GEN_OP6PTR(op, p1, p2, p3, p4, p5, size, p6ptr);

#define VBOX_OGL_GEN_SYNC_OP7_PTR(op, p1, p2, p3, p4, p5, p6, p7ptr)                                \
    VBOX_OGL_GEN_OP7PTR(op, p1, p2, p3, p4, p5, p6, p7ptr);


/* Sync operation whose last parameter is a block of memory in which results are returned */
#define VBOX_OGL_GEN_SYNC_OP1_PASS_PTR(op, Type1)                                                   \
    OGL_CMD(op, 0);                                                                                 \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op((Type1 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP2_PASS_PTR(op, Type1, Type2)                                            \
    OGL_CMD(op, 1);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, (Type2 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP3_PASS_PTR(op, Type1, Type2, Type3)                                     \
    OGL_CMD(op, 2);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, p2, (Type3 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP4_PASS_PTR(op, Type1, Type2, Type3, Type4)                              \
    OGL_CMD(op, 3);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, p2, p3, (Type4 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP5_PASS_PTR(op, Type1, Type2, Type3, Type4, Type5)                       \
    OGL_CMD(op, 4);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, p2, p3, p4, (Type5 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP6_PASS_PTR(op, Type1, Type2, Type3, Type4, Type5, Type6)                \
    OGL_CMD(op, 5);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    OGL_PARAM(Type5, p5);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, p2, p3, p4, p5, (Type6 *)pClient->pLastParam);

#define VBOX_OGL_GEN_SYNC_OP7_PASS_PTR(op, Type1, Type2, Type3, Type4, Type5, Type6, Type7)         \
    OGL_CMD(op, 6);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    OGL_PARAM(Type5, p5);                                                                           \
    OGL_PARAM(Type6, p6);                                                                           \
    Assert(pClient->pLastParam && pClient->cbLastParam);                                            \
    gl##op(p1, p2, p3, p4, p5, p6, (Type7 *)pClient->pLastParam);


/* Sync operation whose last parameter is a block of memory and returns a value */
#define VBOX_OGL_GEN_SYNC_OP2_PTR_RET(rettype, op, Type1, Type2)                                    \
    OGL_CMD(op, 2);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_MEMPARAM(Type2, p2);                                                                        \
    pClient->lastretval = gl##op(p1);

#define VBOX_OGL_GEN_SYNC_OP4_PTR_RET(rettype, op, Type1, Type2, Type3, Type4)                      \
    OGL_CMD(op, 4);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_MEMPARAM(Type4, p4);                                                                        \
    pClient->lastretval = gl##op(p1, p2, p3, p4);

#define VBOX_OGL_GEN_SYNC_OP5_PTR_RET(rettype, op, Type1, Type2, Type3, Type4, Type5)               \
    OGL_CMD(op, 5);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    OGL_MEMPARAM(Type5, p5);                                                                        \
    pClient->lastretval = gl##op(p1, p2, p3, p4, p5);

#define VBOX_OGL_GEN_SYNC_OP6_PTR_RET(rettype, op, Type1, Type2, Type3, Type4, Type5, Type6)        \
    OGL_CMD(op, 6);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    OGL_PARAM(Type5, p5);                                                                           \
    OGL_MEMPARAM(Type6, p6);                                                                        \
    pClient->lastretval = gl##op(p1, p2, p3, p4, p5, p6);

#define VBOX_OGL_GEN_SYNC_OP7_PTR_RET(rettype, op, Type1, Type2, Type3, Type4, Type5, Type6, Type7) \
    OGL_CMD(op, 7);                                                                                 \
    OGL_PARAM(Type1, p1);                                                                           \
    OGL_PARAM(Type2, p2);                                                                           \
    OGL_PARAM(Type3, p3);                                                                           \
    OGL_PARAM(Type4, p4);                                                                           \
    OGL_PARAM(Type5, p5);                                                                           \
    OGL_PARAM(Type6, p6);                                                                           \
    OGL_MEMPARAM(Type7, p7);                                                                        \
    pClient->lastretval = gl##op(p1, p2, p3, p4, p5, p6, p7);





/* Generate async functions elements in the command queue */
#define GL_GEN_FUNC(Function)                                                                           \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP(Function);                                                                      \
    }

#define GL_GEN_FUNC1(Function, Type)                                                                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP1(Function, Type);                                                               \
    }

#define GL_GEN_FUNC1V(Function, Type)       GL_GEN_FUNC1(Function, Type)

#define GL_GEN_FUNC2(Function, Type)                                                                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP2(Function, Type, Type);                                                         \
    }

#define GL_GEN_FUNC2V(Function, Type)       GL_GEN_FUNC2(Function, Type)

#define GL_GEN_FUNC3(Function, Type)                                                                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP3(Function, Type, Type, Type);                                                   \
    }

#define GL_GEN_FUNC3V(Function, Type)       GL_GEN_FUNC3(Function, Type)

#define GL_GEN_FUNC4(Function, Type)                                                                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP4(Function, Type, Type, Type, Type);                                             \
    }

#define GL_GEN_FUNC4V(Function, Type)       GL_GEN_FUNC4(Function, Type)

#define GL_GEN_FUNC6(Function, Type)                                                                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP6(Function, Type, Type, Type, Type, Type, Type);                                 \
    }

#define GL_GEN_VPAR_FUNC2(Function, Type1, Type2)                                                       \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP2(Function, Type1, Type2);                                                       \
    }

#define GL_GEN_VPAR_FUNC2V(Function, Type)       GL_GEN_VPAR_FUNC2(Function, Type)

#define GL_GEN_VPAR_FUNC3(Function, Type1, Type2, Type3)                                                \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP3(Function, Type1, Type2, Type3);                                                \
    }

#define GL_GEN_VPAR_FUNC3V(Function, Type)       GL_GEN_VPAR_FUNC3(Function, Type)

#define GL_GEN_VPAR_FUNC4(Function, Type1, Type2, Type3, Type4)                                         \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP4(Function, Type1, Type2, Type3, Type4);                                         \
    }

#define GL_GEN_VPAR_FUNC5(Function, Type1, Type2, Type3, Type4, Type5)                                  \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP5(Function, Type1, Type2, Type3, Type4 ,Type5);                                  \
    }

#define GL_GEN_VPAR_FUNC6(Function, Type1, Type2, Type3, Type4, Type5, Type6)                           \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP6(Function, Type1, Type2, Type3, Type4 ,Type5, Type6);                           \
    }

#define GL_GEN_VPAR_FUNC7(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7)                    \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP7(Function, Type1, Type2, Type3, Type4 ,Type5, Type6, Type7);                    \
    }

#define GL_GEN_VPAR_FUNC8(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8)             \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP8(Function, Type1, Type2, Type3, Type4 ,Type5, Type6, Type7, Type8);             \
    }

#define GL_GEN_VPAR_FUNC9(Function, Type1, Type2, Type3, Type4, Type5, Type6, Type7, Type8 ,Type9)      \
    void  VBOX_OGL_NAME_PREFIX(Function) (VBOXOGLCTX *pClient, uint8_t *pCmdBuffer)             \
    {                                                                                                   \
        VBOX_OGL_GEN_OP9(Function, Type1, Type2, Type3, Type4 ,Type5, Type6, Type7, Type8, Type9);      \
    }

#endif /* VBOX_OGL_HOST_SIDE */




/* OpenGL opcodes */
/* Note: keep all three tables in sync! */
typedef enum
{
    VBOX_OGL_OP_Illegal                     = 0,
    VBOX_OGL_OP_ArrayElement,
    VBOX_OGL_OP_Begin,
    VBOX_OGL_OP_BindTexture,
    VBOX_OGL_OP_BlendFunc,
    VBOX_OGL_OP_CallList,
    VBOX_OGL_OP_Color3b,
    VBOX_OGL_OP_Color3d,
    VBOX_OGL_OP_Color3f,
    VBOX_OGL_OP_Color3i,
    VBOX_OGL_OP_Color3s,
    VBOX_OGL_OP_Color3ub,
    VBOX_OGL_OP_Color3ui,
    VBOX_OGL_OP_Color3us,
    VBOX_OGL_OP_Color4b,
    VBOX_OGL_OP_Color4d,
    VBOX_OGL_OP_Color4f,
    VBOX_OGL_OP_Color4i,
    VBOX_OGL_OP_Color4s,
    VBOX_OGL_OP_Color4ub,
    VBOX_OGL_OP_Color4ui,
    VBOX_OGL_OP_Color4us,
    VBOX_OGL_OP_Clear,
    VBOX_OGL_OP_ClearAccum,
    VBOX_OGL_OP_ClearColor,
    VBOX_OGL_OP_ClearDepth,
    VBOX_OGL_OP_ClearIndex,
    VBOX_OGL_OP_ClearStencil,
    VBOX_OGL_OP_Accum,
    VBOX_OGL_OP_AlphaFunc,
    VBOX_OGL_OP_Vertex2d,
    VBOX_OGL_OP_Vertex2f,
    VBOX_OGL_OP_Vertex2i,
    VBOX_OGL_OP_Vertex2s,
    VBOX_OGL_OP_Vertex3d,
    VBOX_OGL_OP_Vertex3f,
    VBOX_OGL_OP_Vertex3i,
    VBOX_OGL_OP_Vertex3s,
    VBOX_OGL_OP_Vertex4d,
    VBOX_OGL_OP_Vertex4f,
    VBOX_OGL_OP_Vertex4i,
    VBOX_OGL_OP_Vertex4s,
    VBOX_OGL_OP_TexCoord1d,
    VBOX_OGL_OP_TexCoord1f,
    VBOX_OGL_OP_TexCoord1i,
    VBOX_OGL_OP_TexCoord1s,
    VBOX_OGL_OP_TexCoord2d,
    VBOX_OGL_OP_TexCoord2f,
    VBOX_OGL_OP_TexCoord2i,
    VBOX_OGL_OP_TexCoord2s,
    VBOX_OGL_OP_TexCoord3d,
    VBOX_OGL_OP_TexCoord3f,
    VBOX_OGL_OP_TexCoord3i,
    VBOX_OGL_OP_TexCoord3s,
    VBOX_OGL_OP_TexCoord4d,
    VBOX_OGL_OP_TexCoord4f,
    VBOX_OGL_OP_TexCoord4i,
    VBOX_OGL_OP_TexCoord4s,
    VBOX_OGL_OP_Normal3b,
    VBOX_OGL_OP_Normal3d,
    VBOX_OGL_OP_Normal3f,
    VBOX_OGL_OP_Normal3i,
    VBOX_OGL_OP_Normal3s,
    VBOX_OGL_OP_RasterPos2d,
    VBOX_OGL_OP_RasterPos2f,
    VBOX_OGL_OP_RasterPos2i,
    VBOX_OGL_OP_RasterPos2s,
    VBOX_OGL_OP_RasterPos3d,
    VBOX_OGL_OP_RasterPos3f,
    VBOX_OGL_OP_RasterPos3i,
    VBOX_OGL_OP_RasterPos3s,
    VBOX_OGL_OP_RasterPos4d,
    VBOX_OGL_OP_RasterPos4f,
    VBOX_OGL_OP_RasterPos4i,
    VBOX_OGL_OP_RasterPos4s,
    VBOX_OGL_OP_EvalCoord1d,
    VBOX_OGL_OP_EvalCoord1f,
    VBOX_OGL_OP_EvalCoord2d,
    VBOX_OGL_OP_EvalCoord2f,
    VBOX_OGL_OP_EvalPoint1,
    VBOX_OGL_OP_EvalPoint2,
    VBOX_OGL_OP_Indexd,
    VBOX_OGL_OP_Indexf,
    VBOX_OGL_OP_Indexi,
    VBOX_OGL_OP_Indexs,
    VBOX_OGL_OP_Indexub,
    VBOX_OGL_OP_Rotated,
    VBOX_OGL_OP_Rotatef,
    VBOX_OGL_OP_Scaled,
    VBOX_OGL_OP_Scalef,
    VBOX_OGL_OP_Translated,
    VBOX_OGL_OP_Translatef,
    VBOX_OGL_OP_DepthFunc,
    VBOX_OGL_OP_DepthMask,
    VBOX_OGL_OP_Finish,
    VBOX_OGL_OP_Flush,
    VBOX_OGL_OP_DeleteLists,
    VBOX_OGL_OP_CullFace,
    VBOX_OGL_OP_DeleteTextures,
    VBOX_OGL_OP_DepthRange,
    VBOX_OGL_OP_DisableClientState,
    VBOX_OGL_OP_EnableClientState,
    VBOX_OGL_OP_EvalMesh1,
    VBOX_OGL_OP_EvalMesh2,
    VBOX_OGL_OP_Fogf,
    VBOX_OGL_OP_Fogfv,
    VBOX_OGL_OP_Fogi,
    VBOX_OGL_OP_Fogiv,
    VBOX_OGL_OP_LightModelf,
    VBOX_OGL_OP_LightModelfv,
    VBOX_OGL_OP_LightModeli,
    VBOX_OGL_OP_LightModeliv,
    VBOX_OGL_OP_Lightf,
    VBOX_OGL_OP_Lightfv,
    VBOX_OGL_OP_Lighti,
    VBOX_OGL_OP_Lightiv,
    VBOX_OGL_OP_LineStipple,
    VBOX_OGL_OP_LineWidth,
    VBOX_OGL_OP_ListBase,
    VBOX_OGL_OP_DrawArrays,
    VBOX_OGL_OP_DrawBuffer,
    VBOX_OGL_OP_EdgeFlag,
    VBOX_OGL_OP_End,
    VBOX_OGL_OP_EndList,
    VBOX_OGL_OP_CopyTexImage1D,
    VBOX_OGL_OP_CopyTexImage2D,
    VBOX_OGL_OP_ColorMaterial,
    VBOX_OGL_OP_Materiali,
    VBOX_OGL_OP_Materialf,
    VBOX_OGL_OP_Materialfv,
    VBOX_OGL_OP_Materialiv,
    VBOX_OGL_OP_PopAttrib,
    VBOX_OGL_OP_PopClientAttrib,
    VBOX_OGL_OP_PopMatrix,
    VBOX_OGL_OP_PopName,
    VBOX_OGL_OP_PushAttrib,
    VBOX_OGL_OP_PushClientAttrib,
    VBOX_OGL_OP_PushMatrix,
    VBOX_OGL_OP_PushName,
    VBOX_OGL_OP_ReadBuffer,
    VBOX_OGL_OP_TexGendv,
    VBOX_OGL_OP_TexGenf,
    VBOX_OGL_OP_TexGend,
    VBOX_OGL_OP_TexGeni,
    VBOX_OGL_OP_TexEnvi,
    VBOX_OGL_OP_TexEnvf,
    VBOX_OGL_OP_TexEnviv,
    VBOX_OGL_OP_TexEnvfv,
    VBOX_OGL_OP_TexGeniv,
    VBOX_OGL_OP_TexGenfv,
    VBOX_OGL_OP_TexParameterf,
    VBOX_OGL_OP_TexParameteri,
    VBOX_OGL_OP_TexParameterfv,
    VBOX_OGL_OP_TexParameteriv,
    VBOX_OGL_OP_LoadIdentity,
    VBOX_OGL_OP_LoadName,
    VBOX_OGL_OP_LoadMatrixd,
    VBOX_OGL_OP_LoadMatrixf,
    VBOX_OGL_OP_StencilFunc,
    VBOX_OGL_OP_ShadeModel,
    VBOX_OGL_OP_StencilMask,
    VBOX_OGL_OP_StencilOp,
    VBOX_OGL_OP_Scissor,
    VBOX_OGL_OP_Viewport,
    VBOX_OGL_OP_Rectd,
    VBOX_OGL_OP_Rectf,
    VBOX_OGL_OP_Recti,
    VBOX_OGL_OP_Rects,
    VBOX_OGL_OP_Rectdv,
    VBOX_OGL_OP_Rectfv,
    VBOX_OGL_OP_Rectiv,
    VBOX_OGL_OP_Rectsv,
    VBOX_OGL_OP_MultMatrixd,
    VBOX_OGL_OP_MultMatrixf,
    VBOX_OGL_OP_NewList,
    VBOX_OGL_OP_Hint,
    VBOX_OGL_OP_IndexMask,
    VBOX_OGL_OP_InitNames,
    VBOX_OGL_OP_TexCoordPointer,
    VBOX_OGL_OP_VertexPointer,
    VBOX_OGL_OP_ColorPointer,
    VBOX_OGL_OP_EdgeFlagPointer,
    VBOX_OGL_OP_IndexPointer,
    VBOX_OGL_OP_NormalPointer,
    VBOX_OGL_OP_PolygonStipple,
    VBOX_OGL_OP_CallLists,
    VBOX_OGL_OP_ClipPlane,
    VBOX_OGL_OP_Frustum,
    VBOX_OGL_OP_GenTextures,
    VBOX_OGL_OP_Map1d,
    VBOX_OGL_OP_Map1f,
    VBOX_OGL_OP_Map2d,
    VBOX_OGL_OP_Map2f,
    VBOX_OGL_OP_MapGrid1d,
    VBOX_OGL_OP_MapGrid1f,
    VBOX_OGL_OP_MapGrid2d,
    VBOX_OGL_OP_MapGrid2f,
    VBOX_OGL_OP_CopyPixels,
    VBOX_OGL_OP_TexImage1D,
    VBOX_OGL_OP_TexImage2D,
    VBOX_OGL_OP_TexSubImage1D,
    VBOX_OGL_OP_TexSubImage2D,
    VBOX_OGL_OP_FeedbackBuffer,
    VBOX_OGL_OP_SelectBuffer,
    VBOX_OGL_OP_IsList,
    VBOX_OGL_OP_IsTexture,
    VBOX_OGL_OP_RenderMode,
    VBOX_OGL_OP_ReadPixels,
    VBOX_OGL_OP_IsEnabled,
    VBOX_OGL_OP_GenLists,
    VBOX_OGL_OP_PixelTransferf,
    VBOX_OGL_OP_PixelTransferi,
    VBOX_OGL_OP_PixelZoom,
    VBOX_OGL_OP_PixelStorei,
    VBOX_OGL_OP_PixelStoref,
    VBOX_OGL_OP_PixelMapfv,
    VBOX_OGL_OP_PixelMapuiv,
    VBOX_OGL_OP_PixelMapusv,
    VBOX_OGL_OP_PointSize,
    VBOX_OGL_OP_PolygonMode,
    VBOX_OGL_OP_PolygonOffset,
    VBOX_OGL_OP_PassThrough,
    VBOX_OGL_OP_Ortho,
    VBOX_OGL_OP_MatrixMode,
    VBOX_OGL_OP_LogicOp,
    VBOX_OGL_OP_ColorMask,
    VBOX_OGL_OP_CopyTexSubImage1D,
    VBOX_OGL_OP_CopyTexSubImage2D,
    VBOX_OGL_OP_FrontFace,
    VBOX_OGL_OP_Disable,
    VBOX_OGL_OP_Enable,
    VBOX_OGL_OP_PrioritizeTextures,
    VBOX_OGL_OP_GetBooleanv,
    VBOX_OGL_OP_GetDoublev,
    VBOX_OGL_OP_GetFloatv,
    VBOX_OGL_OP_GetIntegerv,
    VBOX_OGL_OP_GetLightfv,
    VBOX_OGL_OP_GetLightiv,
    VBOX_OGL_OP_GetMaterialfv,
    VBOX_OGL_OP_GetMaterialiv,
    VBOX_OGL_OP_GetPixelMapfv,
    VBOX_OGL_OP_GetPixelMapuiv,
    VBOX_OGL_OP_GetPixelMapusv,
    VBOX_OGL_OP_GetTexEnviv,
    VBOX_OGL_OP_GetTexEnvfv,
    VBOX_OGL_OP_GetTexGendv,
    VBOX_OGL_OP_GetTexGenfv,
    VBOX_OGL_OP_GetTexGeniv,
    VBOX_OGL_OP_GetTexParameterfv,
    VBOX_OGL_OP_GetTexParameteriv,
    VBOX_OGL_OP_GetClipPlane,
    VBOX_OGL_OP_GetPolygonStipple,
    VBOX_OGL_OP_GetTexLevelParameterfv,
    VBOX_OGL_OP_GetTexLevelParameteriv,
    VBOX_OGL_OP_GetTexImage,

    /* Windows ICD exports */
    VBOX_OGL_OP_DrvReleaseContext,
    VBOX_OGL_OP_DrvCreateContext,
    VBOX_OGL_OP_DrvDeleteContext,
    VBOX_OGL_OP_DrvCopyContext,
    VBOX_OGL_OP_DrvSetContext,
    VBOX_OGL_OP_DrvCreateLayerContext,
    VBOX_OGL_OP_DrvShareLists,
    VBOX_OGL_OP_DrvDescribeLayerPlane,
    VBOX_OGL_OP_DrvSetLayerPaletteEntries,
    VBOX_OGL_OP_DrvGetLayerPaletteEntries,
    VBOX_OGL_OP_DrvRealizeLayerPalette,
    VBOX_OGL_OP_DrvSwapLayerBuffers,
    VBOX_OGL_OP_DrvDescribePixelFormat,
    VBOX_OGL_OP_DrvSetPixelFormat,
    VBOX_OGL_OP_DrvSwapBuffers,

    /* OpenGL Extensions */
    VBOX_OGL_OP_wglSwapIntervalEXT,
    VBOX_OGL_OP_wglGetSwapIntervalEXT,

    VBOX_OGL_OP_Last,

    VBOX_OGL_OP_SizeHack                     = 0x7fffffff
} VBOX_OGL_OP;

#if defined(DEBUG) && defined(VBOX_OGL_WITH_CMD_STRINGS)
static const char *pszVBoxOGLCmd[VBOX_OGL_OP_Last] =
{
    "ILLEGAL",
    "glArrayElement",
    "glBegin",
    "glBindTexture",
    "glBlendFunc",
    "glCallList",
    "glColor3b",
    "glColor3d",
    "glColor3f",
    "glColor3i",
    "glColor3s",
    "glColor3ub",
    "glColor3ui",
    "glColor3us",
    "glColor4b",
    "glColor4d",
    "glColor4f",
    "glColor4i",
    "glColor4s",
    "glColor4ub",
    "glColor4ui",
    "glColor4us",
    "glClear",
    "glClearAccum",
    "glClearColor",
    "glClearDepth",
    "glClearIndex",
    "glClearStencil",
    "glAccum",
    "glAlphaFunc",
    "glVertex2d",
    "glVertex2f",
    "glVertex2i",
    "glVertex2s",
    "glVertex3d",
    "glVertex3f",
    "glVertex3i",
    "glVertex3s",
    "glVertex4d",
    "glVertex4f",
    "glVertex4i",
    "glVertex4s",
    "glTexCoord1d",
    "glTexCoord1f",
    "glTexCoord1i",
    "glTexCoord1s",
    "glTexCoord2d",
    "glTexCoord2f",
    "glTexCoord2i",
    "glTexCoord2s",
    "glTexCoord3d",
    "glTexCoord3f",
    "glTexCoord3i",
    "glTexCoord3s",
    "glTexCoord4d",
    "glTexCoord4f",
    "glTexCoord4i",
    "glTexCoord4s",
    "glNormal3b",
    "glNormal3d",
    "glNormal3f",
    "glNormal3i",
    "glNormal3s",
    "glRasterPos2d",
    "glRasterPos2f",
    "glRasterPos2i",
    "glRasterPos2s",
    "glRasterPos3d",
    "glRasterPos3f",
    "glRasterPos3i",
    "glRasterPos3s",
    "glRasterPos4d",
    "glRasterPos4f",
    "glRasterPos4i",
    "glRasterPos4s",
    "glEvalCoord1d",
    "glEvalCoord1f",
    "glEvalCoord2d",
    "glEvalCoord2f",
    "glEvalPoint1",
    "glEvalPoint2",
    "glIndexd",
    "glIndexf",
    "glIndexi",
    "glIndexs",
    "glIndexub",
    "glRotated",
    "glRotatef",
    "glScaled",
    "glScalef",
    "glTranslated",
    "glTranslatef",
    "glDepthFunc",
    "glDepthMask",
    "glFinish",
    "glFlush",
    "glDeleteLists",
    "glCullFace",
    "glDeleteTextures",
    "glDepthRange",
    "glDisableClientState",
    "glEnableClientState",
    "glEvalMesh1",
    "glEvalMesh2",
    "glFogf",
    "glFogfv",
    "glFogi",
    "glFogiv",
    "glLightModelf",
    "glLightModelfv",
    "glLightModeli",
    "glLightModeliv",
    "glLightf",
    "glLightfv",
    "glLighti",
    "glLightiv",
    "glLineStipple",
    "glLineWidth",
    "glListBase",
    "glDrawArrays",
    "glDrawBuffer",
    "glEdgeFlag",
    "glEnd",
    "glEndList",
    "glCopyTexImage1D",
    "glCopyTexImage2D",
    "glColorMaterial",
    "glMateriali",
    "glMaterialf",
    "glMaterialfv",
    "glMaterialiv",
    "glPopAttrib",
    "glPopClientAttrib",
    "glPopMatrix",
    "glPopName",
    "glPushAttrib",
    "glPushClientAttrib",
    "glPushMatrix",
    "glPushName",
    "glReadBuffer",
    "glTexGendv",
    "glTexGenf",
    "glTexGend",
    "glTexGeni",
    "glTexEnvi",
    "glTexEnvf",
    "glTexEnviv",
    "glTexEnvfv",
    "glTexGeniv",
    "glTexGenfv",
    "glTexParameterf",
    "glTexParameteri",
    "glTexParameterfv",
    "glTexParameteriv",
    "glLoadIdentity",
    "glLoadName",
    "glLoadMatrixd",
    "glLoadMatrixf",
    "glStencilFunc",
    "glShadeModel",
    "glStencilMask",
    "glStencilOp",
    "glScissor",
    "glViewport",
    "glRectd",
    "glRectf",
    "glRecti",
    "glRects",
    "glRectdv",
    "glRectfv",
    "glRectiv",
    "glRectsv",
    "glMultMatrixd",
    "glMultMatrixf",
    "glNewList",
    "glHint",
    "glIndexMask",
    "glInitNames",
    "glTexCoordPointer",
    "glVertexPointer",
    "glColorPointer",
    "glEdgeFlagPointer",
    "glIndexPointer",
    "glNormalPointer",
    "glPolygonStipple",
    "glCallLists",
    "glClipPlane",
    "glFrustum",
    "glGenTextures",
    "glMap1d",
    "glMap1f",
    "glMap2d",
    "glMap2f",
    "glMapGrid1d",
    "glMapGrid1f",
    "glMapGrid2d",
    "glMapGrid2f",
    "glCopyPixels",
    "glTexImage1D",
    "glTexImage2D",
    "glTexSubImage1D",
    "glTexSubImage2D",
    "glFeedbackBuffer",
    "glSelectBuffer",
    "glIsList",
    "glIsTexture",
    "glRenderMode",
    "glReadPixels",
    "glIsEnabled",
    "glGenLists",
    "glPixelTransferf",
    "glPixelTransferi",
    "glPixelZoom",
    "glPixelStorei",
    "glPixelStoref",
    "glPixelMapfv",
    "glPixelMapuiv",
    "glPixelMapusv",
    "glPointSize",
    "glPolygonMode",
    "glPolygonOffset",
    "glPassThrough",
    "glOrtho",
    "glMatrixMode",
    "glLogicOp",
    "glColorMask",
    "glCopyTexSubImage1D",
    "glCopyTexSubImage2D",
    "glFrontFace",
    "glDisable",
    "glEnable",
    "glPrioritizeTextures",
    "glGetBooleanv",
    "glGetDoublev",
    "glGetFloatv",
    "glGetIntegerv",
    "glGetLightfv",
    "glGetLightiv",
    "glGetMaterialfv",
    "glGetMaterialiv",
    "glGetPixelMapfv",
    "glGetPixelMapuiv",
    "glGetPixelMapusv",
    "glGetTexEnviv",
    "glGetTexEnvfv",
    "glGetTexGendv",
    "glGetTexGenfv",
    "glGetTexGeniv",
    "glGetTexParameterfv",
    "glGetTexParameteriv",
    "glGetClipPlane",
    "glGetPolygonStipple",
    "glGetTexLevelParameterfv",
    "glGetTexLevelParameteriv",
    "glGetTexImage",

    /* Windows ICD exports */
    "DrvReleaseContext",
    "DrvCreateContext",
    "DrvDeleteContext",
    "DrvCopyContext",
    "DrvSetContext",
    "DrvCreateLayerContext",
    "DrvShareLists",
    "DrvDescribeLayerPlane",
    "DrvSetLayerPaletteEntries",
    "DrvGetLayerPaletteEntries",
    "DrvRealizeLayerPalette",
    "DrvSwapLayerBuffers",
    "DrvDescribePixelFormat",
    "DrvSetPixelFormat",
    "DrvSwapBuffers",

    /* OpenGL Extensions */
    "wglSwapIntervalEXT",
    "wglGetSwapIntervalEXT",
};
#endif

#ifdef VBOX_OGL_WITH_FUNCTION_WRAPPERS
/* OpenGL function wrappers. */
static PFN_VBOXGLWRAPPER pfnOGLWrapper[VBOX_OGL_OP_Last] =
{
    NULL,
    vboxglArrayElement,
    vboxglBegin,
    vboxglBindTexture,
    vboxglBlendFunc,
    vboxglCallList,
    vboxglColor3b,
    vboxglColor3d,
    vboxglColor3f,
    vboxglColor3i,
    vboxglColor3s,
    vboxglColor3ub,
    vboxglColor3ui,
    vboxglColor3us,
    vboxglColor4b,
    vboxglColor4d,
    vboxglColor4f,
    vboxglColor4i,
    vboxglColor4s,
    vboxglColor4ub,
    vboxglColor4ui,
    vboxglColor4us,
    vboxglClear,
    vboxglClearAccum,
    vboxglClearColor,
    vboxglClearDepth,
    vboxglClearIndex,
    vboxglClearStencil,
    vboxglAccum,
    vboxglAlphaFunc,
    vboxglVertex2d,
    vboxglVertex2f,
    vboxglVertex2i,
    vboxglVertex2s,
    vboxglVertex3d,
    vboxglVertex3f,
    vboxglVertex3i,
    vboxglVertex3s,
    vboxglVertex4d,
    vboxglVertex4f,
    vboxglVertex4i,
    vboxglVertex4s,
    vboxglTexCoord1d,
    vboxglTexCoord1f,
    vboxglTexCoord1i,
    vboxglTexCoord1s,
    vboxglTexCoord2d,
    vboxglTexCoord2f,
    vboxglTexCoord2i,
    vboxglTexCoord2s,
    vboxglTexCoord3d,
    vboxglTexCoord3f,
    vboxglTexCoord3i,
    vboxglTexCoord3s,
    vboxglTexCoord4d,
    vboxglTexCoord4f,
    vboxglTexCoord4i,
    vboxglTexCoord4s,
    vboxglNormal3b,
    vboxglNormal3d,
    vboxglNormal3f,
    vboxglNormal3i,
    vboxglNormal3s,
    vboxglRasterPos2d,
    vboxglRasterPos2f,
    vboxglRasterPos2i,
    vboxglRasterPos2s,
    vboxglRasterPos3d,
    vboxglRasterPos3f,
    vboxglRasterPos3i,
    vboxglRasterPos3s,
    vboxglRasterPos4d,
    vboxglRasterPos4f,
    vboxglRasterPos4i,
    vboxglRasterPos4s,
    vboxglEvalCoord1d,
    vboxglEvalCoord1f,
    vboxglEvalCoord2d,
    vboxglEvalCoord2f,
    vboxglEvalPoint1,
    vboxglEvalPoint2,
    vboxglIndexd,
    vboxglIndexf,
    vboxglIndexi,
    vboxglIndexs,
    vboxglIndexub,
    vboxglRotated,
    vboxglRotatef,
    vboxglScaled,
    vboxglScalef,
    vboxglTranslated,
    vboxglTranslatef,
    vboxglDepthFunc,
    vboxglDepthMask,
    vboxglFinish,
    vboxglFlush,
    vboxglDeleteLists,
    vboxglCullFace,
    vboxglDeleteTextures,
    vboxglDepthRange,
    vboxglDisableClientState,
    vboxglEnableClientState,
    vboxglEvalMesh1,
    vboxglEvalMesh2,
    vboxglFogf,
    vboxglFogfv,
    vboxglFogi,
    vboxglFogiv,
    vboxglLightModelf,
    vboxglLightModelfv,
    vboxglLightModeli,
    vboxglLightModeliv,
    vboxglLightf,
    vboxglLightfv,
    vboxglLighti,
    vboxglLightiv,
    vboxglLineStipple,
    vboxglLineWidth,
    vboxglListBase,
    vboxglDrawArrays,
    vboxglDrawBuffer,
    vboxglEdgeFlag,
    vboxglEnd,
    vboxglEndList,
    vboxglCopyTexImage1D,
    vboxglCopyTexImage2D,
    vboxglColorMaterial,
    vboxglMateriali,
    vboxglMaterialf,
    vboxglMaterialfv,
    vboxglMaterialiv,
    vboxglPopAttrib,
    vboxglPopClientAttrib,
    vboxglPopMatrix,
    vboxglPopName,
    vboxglPushAttrib,
    vboxglPushClientAttrib,
    vboxglPushMatrix,
    vboxglPushName,
    vboxglReadBuffer,
    vboxglTexGendv,
    vboxglTexGenf,
    vboxglTexGend,
    vboxglTexGeni,
    vboxglTexEnvi,
    vboxglTexEnvf,
    vboxglTexEnviv,
    vboxglTexEnvfv,
    vboxglTexGeniv,
    vboxglTexGenfv,
    vboxglTexParameterf,
    vboxglTexParameteri,
    vboxglTexParameterfv,
    vboxglTexParameteriv,
    vboxglLoadIdentity,
    vboxglLoadName,
    vboxglLoadMatrixd,
    vboxglLoadMatrixf,
    vboxglStencilFunc,
    vboxglShadeModel,
    vboxglStencilMask,
    vboxglStencilOp,
    vboxglScissor,
    vboxglViewport,
    vboxglRectd,
    vboxglRectf,
    vboxglRecti,
    vboxglRects,
    vboxglRectdv,
    vboxglRectfv,
    vboxglRectiv,
    vboxglRectsv,
    vboxglMultMatrixd,
    vboxglMultMatrixf,
    vboxglNewList,
    vboxglHint,
    vboxglIndexMask,
    vboxglInitNames,
    vboxglTexCoordPointer,
    vboxglVertexPointer,
    vboxglColorPointer,
    vboxglEdgeFlagPointer,
    vboxglIndexPointer,
    vboxglNormalPointer,
    vboxglPolygonStipple,
    vboxglCallLists,
    vboxglClipPlane,
    vboxglFrustum,
    vboxglGenTextures,
    vboxglMap1d,
    vboxglMap1f,
    vboxglMap2d,
    vboxglMap2f,
    vboxglMapGrid1d,
    vboxglMapGrid1f,
    vboxglMapGrid2d,
    vboxglMapGrid2f,
    vboxglCopyPixels,
    vboxglTexImage1D,
    vboxglTexImage2D,
    vboxglTexSubImage1D,
    vboxglTexSubImage2D,
    vboxglFeedbackBuffer,
    vboxglSelectBuffer,
    vboxglIsList,
    vboxglIsTexture,
    vboxglRenderMode,
    vboxglReadPixels,
    vboxglIsEnabled,
    vboxglGenLists,
    vboxglPixelTransferf,
    vboxglPixelTransferi,
    vboxglPixelZoom,
    vboxglPixelStorei,
    vboxglPixelStoref,
    vboxglPixelMapfv,
    vboxglPixelMapuiv,
    vboxglPixelMapusv,
    vboxglPointSize,
    vboxglPolygonMode,
    vboxglPolygonOffset,
    vboxglPassThrough,
    vboxglOrtho,
    vboxglMatrixMode,
    vboxglLogicOp,
    vboxglColorMask,
    vboxglCopyTexSubImage1D,
    vboxglCopyTexSubImage2D,
    vboxglFrontFace,
    vboxglDisable,
    vboxglEnable,
    vboxglPrioritizeTextures,
    vboxglGetBooleanv,
    vboxglGetDoublev,
    vboxglGetFloatv,
    vboxglGetIntegerv,
    vboxglGetLightfv,
    vboxglGetLightiv,
    vboxglGetMaterialfv,
    vboxglGetMaterialiv,
    vboxglGetPixelMapfv,
    vboxglGetPixelMapuiv,
    vboxglGetPixelMapusv,
    vboxglGetTexEnviv,
    vboxglGetTexEnvfv,
    vboxglGetTexGendv,
    vboxglGetTexGenfv,
    vboxglGetTexGeniv,
    vboxglGetTexParameterfv,
    vboxglGetTexParameteriv,
    vboxglGetClipPlane,
    vboxglGetPolygonStipple,
    vboxglGetTexLevelParameterfv,
    vboxglGetTexLevelParameteriv,
    vboxglGetTexImage,

    /* Windows ICD exports */
    vboxglDrvReleaseContext,
    vboxglDrvCreateContext,
    vboxglDrvDeleteContext,
    vboxglDrvCopyContext,
    vboxglDrvSetContext,
    vboxglDrvCreateLayerContext,
    vboxglDrvShareLists,
    vboxglDrvDescribeLayerPlane,
    vboxglDrvSetLayerPaletteEntries,
    vboxglDrvGetLayerPaletteEntries,
    vboxglDrvRealizeLayerPalette,
    vboxglDrvSwapLayerBuffers,
    vboxglDrvDescribePixelFormat,
    vboxglDrvSetPixelFormat,
    vboxglDrvSwapBuffers,

#ifdef RT_OS_WINDOWS
    /* OpenGL Extensions */
    vboxwglSwapIntervalEXT,
    vboxwglGetSwapIntervalEXT,
#endif
};
#endif


#ifdef VBOX_OGL_WITH_EXTENSION_ARRAY
typedef struct
{
    const char *pszExtName;
    const char *pszExtFunctionName;
#ifdef VBOX_OGL_GUEST_SIDE
    RTUINTPTR   pfnFunction;
#else
    RTUINTPTR  *ppfnFunction;
#endif
    bool        fAvailable;
} OPENGL_EXT, *POPENGL_EXT;

#ifdef VBOX_OGL_GUEST_SIDE
#define VBOX_OGL_EXTENSION(a)   (RTUINTPTR)a
#else
#define VBOX_OGL_EXTENSION(a)   (RTUINTPTR *)&pfn##a

static PFNWGLSWAPINTERVALEXTPROC        pfnwglSwapIntervalEXT       = NULL;
static PFNWGLGETSWAPINTERVALEXTPROC     pfnwglGetSwapIntervalEXT    = NULL;

#endif

static OPENGL_EXT OpenGLExtensions[] =
{
    {   "WGL_EXT_swap_control",             "wglSwapIntervalEXT",               VBOX_OGL_EXTENSION(wglSwapIntervalEXT),                      false },
    {   "WGL_EXT_swap_control",             "wglGetSwapIntervalEXT",            VBOX_OGL_EXTENSION(wglGetSwapIntervalEXT),                   false },
};
#endif /* VBOX_OGL_WITH_EXTENSION_ARRAY */

#endif

