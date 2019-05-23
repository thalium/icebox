/* $Id: cconvApplyAYUV.c $ */
void vboxCConvApplyAYUV(vec4 color)
{
    float y, u, v, r, g, b;
    y = color.g;
    u = color.r;
    v = color.a;
    u = u - 0.5;
    v = v - 0.5;
    y = 1.164*(y-0.0625);
    b = y + 2.018*u;
    g = y - 0.813*v - 0.391*u;
    r = y + 1.596*v;
    gl_FragColor = vec4(r,g,b,1.0);
}
