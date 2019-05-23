/* $Id: cconvBGR.c $ */
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect uSrcTex;
void vboxCConv()
{
    vec2 srcCoord = vec2(gl_TexCoord[0]);
    gl_FragColor = texture2DRect(uSrcTex, srcCoord);
}
