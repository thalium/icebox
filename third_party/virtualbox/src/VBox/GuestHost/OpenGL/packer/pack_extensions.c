/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"

int __packTexParameterNumParams( GLenum param )
{
    switch( param )
    {
#ifdef CR_EXT_texture_filter_anisotropic
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            return 1;
#endif
        default:
            break;
    }
    return 0;
}

int __packFogParamsLength( GLenum param )
{
    static int one_param = sizeof( GLfloat );
        (void) one_param;
    switch( param )
    {
#ifdef CR_NV_fog_distance
        case GL_FOG_DISTANCE_MODE_NV:
            return one_param;
#endif
        default:
            break;
    }
    return 0;
}
