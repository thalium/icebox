/* $Id: cconvYV12.c $ */
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect uSrcTex;
uniform sampler2DRect uVTex;
uniform sampler2DRect uUTex;
void vboxCConvApplyAYUV(vec4 color);
void vboxCConv()
{
    vec2 clrCoordY = vec2(gl_TexCoord[0]);
    vec2 clrCoordV = vec2(gl_TexCoord[1]);
    int ixY = int(clrCoordY.x);
    vec2 coordY = vec2(float(ixY), clrCoordY.y);
    int ixV = int(clrCoordV.x);
    vec2 coordV = vec2(float(ixV), clrCoordV.y);
    vec4 clrY = texture2DRect(uSrcTex, coordY);
    vec4 clrV = texture2DRect(uVTex, coordV);
    vec4 clrU = texture2DRect(uUTex, coordV);
    float partY = clrCoordY.x - float(ixY);
    float partVU = clrCoordV.x - float(ixV);
    float y;
    float v;
    float u;
    if(partY < 0.25)
    {
        y = clrY.b;
    }
    else if(partY < 0.5)
    {
        y = clrY.g;
    }
    else if(partY < 0.75)
    {
        y = clrY.r;
    }
    else
    {
        y = clrY.a;
    }

    if(partVU < 0.25)
    {
        v = clrV.b;
        u = clrU.b;
    }
    else if(partVU < 0.5)
    {
        v = clrV.g;
        u = clrU.g;
    }
    else if(partVU < 0.75)
    {
        v = clrV.r;
        u = clrU.r;
    }
    else
    {
        v = clrV.a;
        u = clrU.a;
    }
    vboxCConvApplyAYUV(vec4(u, y, 0.0, v));
}
