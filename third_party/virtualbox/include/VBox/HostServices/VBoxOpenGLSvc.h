/** @file
 * OpenGL: Common header for host service and guest clients.
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

#ifndef ___VBox_HostService_VBoxOpenGLSvc_h
#define ___VBox_HostService_VBoxOpenGLSvc_h

#include <VBox/VMMDevCoreTypes.h>
#include <VBox/VBoxGuestCoreTypes.h>
#include <VBox/hgcmsvc.h>

/* OpenGL command buffer size */
#define VBOX_OGL_MAX_CMD_BUFFER                     (128*1024)
#define VBOX_OGL_CMD_ALIGN                          4
#define VBOX_OGL_CMD_ALIGN_MASK                     (VBOX_OGL_CMD_ALIGN-1)
#define VBOX_OGL_CMD_MAGIC                          0x1234ABCD

/* for debugging */
#define VBOX_OGL_CMD_STRICT

/* OpenGL command block */
typedef struct
{
#ifdef VBOX_OGL_CMD_STRICT
    uint32_t    Magic;
#endif
    uint32_t    enmOp;
    uint32_t    cbCmd;
    uint32_t    cParams;
    /* start of variable size parameter array */
} VBOX_OGL_CMD, *PVBOX_OGL_CMD;

typedef struct
{
#ifdef VBOX_OGL_CMD_STRICT
    uint32_t    Magic;
#endif
    uint32_t    cbParam;
    /* start of variable size parameter */
} VBOX_OGL_VAR_PARAM, *PVBOX_OGL_VAR_PARAM;

/** OpenGL Folders service functions. (guest)
 *  @{
 */

/** Query mappings changes. */
#define VBOXOGL_FN_GLGETSTRING          (1)
#define VBOXOGL_FN_GLFLUSH              (2)
#define VBOXOGL_FN_GLFLUSHPTR           (3)
#define VBOXOGL_FN_GLCHECKEXT           (4)

/** @} */

/** Function parameter structures.
 *  @{
 */

/**
 * VBOXOGL_FN_GLGETSTRING
 */

/** Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** 32bit, in: name
     * GLenum name parameter
     */
    HGCMFunctionParameter   name;

    /** pointer, in/out
     * Buffer for requested string
     */
    HGCMFunctionParameter   pString;
} VBoxOGLglGetString;

/** Number of parameters */
#define VBOXOGL_CPARMS_GLGETSTRING (2)



/**
 * VBOXOGL_FN_GLFLUSH
 */

/** Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in
     * Command buffer
     */
    HGCMFunctionParameter   pCmdBuffer;

    /** 32bit, out: cCommands
     * Number of commands in the buffer
     */
    HGCMFunctionParameter   cCommands;

    /** 64bit, out: retval
     * uint64_t return code of last command
     */
    HGCMFunctionParameter   retval;

    /** 32bit, out: lasterror
     * GLenum current last error
     */
    HGCMFunctionParameter   lasterror;

} VBoxOGLglFlush;

/** Number of parameters */
#define VBOXOGL_CPARMS_GLFLUSH (4)

/**
 * VBOXOGL_FN_GLFLUSHPTR
 */

/** Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in
     * Command buffer
     */
    HGCMFunctionParameter   pCmdBuffer;

    /** 32bit, out: cCommands
     * Number of commands in the buffer
     */
    HGCMFunctionParameter   cCommands;

    /** pointer, in
     * Last command's final parameter memory block
     */
    HGCMFunctionParameter   pLastParam;

    /** 64bit, out: retval
     * uint64_t return code of last command
     */
    HGCMFunctionParameter   retval;

    /** 32bit, out: lasterror
     * GLenum current last error
     */
    HGCMFunctionParameter   lasterror;

} VBoxOGLglFlushPtr;

/** Number of parameters */
#define VBOXOGL_CPARMS_GLFLUSHPTR (5)


/**
 * VBOXOGL_FN_GLCHECKEXT
 */

/** Parameters structure. */
typedef struct
{
    VBGLIOCHGCMCALL   hdr;

    /** pointer, in
     * Extension function name
     */
    HGCMFunctionParameter   pszExtFnName;

} VBoxOGLglCheckExt;

/** Number of parameters */
#define VBOXOGL_CPARMS_GLCHECKEXT (1)

/** @} */


#endif

