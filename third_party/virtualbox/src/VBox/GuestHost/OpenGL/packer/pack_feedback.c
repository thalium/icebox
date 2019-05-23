/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"

void PACK_APIENTRY crPackFeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
    (void) size;
    (void) type;
    (void) buffer;

    crWarning("Packer wont pass FeedbackBuffer()\n");
    crWarning("Try using the feedbackspu\n");
}

void PACK_APIENTRY crPackFeedbackBufferSWAP( GLsizei size, GLenum type, GLfloat *buffer )
{
    (void) size;
    (void) type;
    (void) buffer;

    crWarning("Packer wont pass FeedbackBuffer()\n");
    crWarning("Try using the feedbackspu\n");
}

void PACK_APIENTRY crPackSelectBuffer( GLsizei size, GLuint *buffer )
{
    (void) size;
    (void) buffer;

    crWarning("Packer wont pass SelectBuffer()\n");
    crWarning("Try using the feedbackspu\n");
}

void PACK_APIENTRY crPackSelectBufferSWAP( GLsizei size, GLuint *buffer )
{
    (void) size;
    (void) buffer;

    crWarning("Packer wont pass SelectBuffer()\n");
    crWarning("Try using the feedbackspu\n");
}
