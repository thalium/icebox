/* $Id: mainOverlayNoDiscard2.c $ */
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect uDstTex;
uniform vec4 uDstClr;
void vboxCConv();
void main(void)
{
    vec4 dstClr = texture2DRect(uDstTex, vec2(gl_TexCoord[2]));
    vec3 difClr = dstClr.rgb - uDstClr.rgb;
    if(any(greaterThan(difClr, vec3(0.01, 0.01, 0.01)))
        || any(lessThan(difClr, vec3(-0.01, -0.01, -0.01))))
    {
        gl_FragColor = dstClr;
    }
    else
    {
        vboxCConv();
    }
}
