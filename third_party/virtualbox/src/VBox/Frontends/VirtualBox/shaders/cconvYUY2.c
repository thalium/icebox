/* $Id: cconvYUY2.c $ */
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect uSrcTex;
void vboxCConvApplyAYUV(vec4 color);
void vboxCConv()
{
    vec2 srcCoord = vec2(gl_TexCoord[0]);
    float x = srcCoord.x;
    int pix = int(x);
    vec4 srcClr = texture2DRect(uSrcTex, vec2(float(pix), srcCoord.y));
    float u = srcClr.g;
    float v = srcClr.a;
    float part = x - float(pix);
    float y;
    if(part < 0.5)
    {
        y = srcClr.b;
    }
    else
    {
        y = srcClr.r;
    }
    vboxCConvApplyAYUV(vec4(u, y, 0.0, v));
}
