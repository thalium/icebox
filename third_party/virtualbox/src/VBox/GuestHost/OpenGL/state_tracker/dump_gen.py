from __future__ import print_function
import sys

import apiutil

import sys, re, string


line_re = re.compile(r'^(\S+)\s+(GL_\S+)\s+(.*)\s*$')
extensions_line_re = re.compile(r'^(\S+)\s+(GL_\S+)\s(\S+)\s+(.*)\s*$')

params = {}
extended_params = {}

input = open( sys.argv[2]+"/state_isenabled.txt", 'r' )
for line in input.readlines():
    match = line_re.match( line )
    if match:
        type = match.group(1)
        pname = match.group(2)
        fields = string.split( match.group(3) )
        params[pname] = ( type, fields )

input = open( sys.argv[2]+"/state_extensions_isenabled.txt", 'r' )
for line in input.readlines():
    match = extensions_line_re.match( line )
    if match:
        type = match.group(1)
        pname = match.group(2)
        ifdef = match.group(3)
        fields = string.split( match.group(4) )
        extended_params[pname] = ( type, ifdef, fields )


apiutil.CopyrightC()

print("""#include "cr_blitter.h"
#include "cr_spu.h"
#include "chromium.h"
#include "cr_error.h"
#include "cr_net.h"
#include "cr_mem.h"
#include "cr_string.h"
#include <cr_dump.h>
#include "cr_pixeldata.h"

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/mem.h>

#include <stdio.h>

#ifdef VBOX_WITH_CRDUMPER
""")

from get_sizes import *;

getprops = apiutil.ParamProps("GetDoublev")
enableprops = apiutil.ParamProps("Enable")

#print "//missing get props:"
#for prop in getprops:
#    try:
#        tmp = num_get_values[prop]
#    except KeyError:
#        try:
#            keyvalues = extensions_num_get_values[prop]
#        except KeyError:
#            print "//%s" % prop
#
print("""
static void crRecDumpPrintVal(CR_DUMPER *pDumper, struct nv_struct *pDesc, float *pfData)
{
    char aBuf[4096];
    crDmpFormatArray(aBuf, sizeof (aBuf), "%f", sizeof (float), pfData, pDesc->num_values);
    crDmpStrF(pDumper, "%s = %s;", pDesc->pszName, aBuf);
}


void crRecDumpGlGetState(CR_RECORDER *pRec, CRContext *ctx)
{
    float afData[CR_MAX_GET_VALUES];
    struct nv_struct *pDesc;
    
    for (pDesc = num_values_array; pDesc->num_values != 0 ; pDesc++)
    {
        memset(afData, 0, sizeof(afData));
        pRec->pDispatch->GetFloatv(pDesc->pname, afData);
        crRecDumpPrintVal(pRec->pDumper, pDesc, afData);
    }
}

void crRecDumpGlEnableState(CR_RECORDER *pRec, CRContext *ctx)
{
    GLboolean fEnabled;
""")
for pname in sorted(params.keys()):
    print("\tfEnabled = pRec->pDispatch->IsEnabled(%s);" % pname)
    print("\tcrDmpStrF(pRec->pDumper, \"%s = %%d;\", fEnabled);" % pname)

for pname in sorted(extended_params.keys()):
    (srctype,ifdef,fields) = extended_params[pname]
    ext = ifdef[3:]  # the extension name with the "GL_" prefix removed
    ext = ifdef
    print('#ifdef CR_%s' % ext)
    print("\tfEnabled = pRec->pDispatch->IsEnabled(%s);" % pname)
    print("\tcrDmpStrF(pRec->pDumper, \"%s = %%d;\", fEnabled);" % pname)
    print('#endif /* CR_%s */' % ext)

#print "//missing enable props:"
#for prop in enableprops:
#    try:
#        keyvalues = params[prop]
#    except KeyError:
#        try:
#            keyvalues = extended_params[prop]
#        except KeyError:
#            print "//%s" % prop
#
print("""
}
#endif
""")

texenv_mappings = {
    'GL_TEXTURE_ENV' : [
        'GL_TEXTURE_ENV_MODE',
        'GL_TEXTURE_ENV_COLOR',
        'GL_COMBINE_RGB',
        'GL_COMBINE_ALPHA',
        'GL_RGB_SCALE',
        'GL_ALPHA_SCALE', 
        'GL_SRC0_RGB',
        'GL_SRC1_RGB',
        'GL_SRC2_RGB',
        'GL_SRC0_ALPHA',
        'GL_SRC1_ALPHA',
        'GL_SRC2_ALPHA'
    ],
    'GL_TEXTURE_FILTER_CONTROL' : [
        'GL_TEXTURE_LOD_BIAS'
    ],
    'GL_POINT_SPRITE' : [
        'GL_COORD_REPLACE'
    ]
}

texgen_coords = [
    'GL_S',
    'GL_T',
    'GL_R',
    'GL_Q'
]

texgen_names = [
    'GL_TEXTURE_GEN_MODE',
    'GL_OBJECT_PLANE',
    'GL_EYE_PLANE'
]

texparam_names = [
    'GL_TEXTURE_MAG_FILTER', 
    'GL_TEXTURE_MIN_FILTER',
    'GL_TEXTURE_MIN_LOD',
    'GL_TEXTURE_MAX_LOD',
    'GL_TEXTURE_BASE_LEVEL',
    'GL_TEXTURE_MAX_LEVEL',
    'GL_TEXTURE_WRAP_S',
    'GL_TEXTURE_WRAP_T',
    'GL_TEXTURE_WRAP_R',
    'GL_TEXTURE_BORDER_COLOR',
    'GL_TEXTURE_PRIORITY',
    'GL_TEXTURE_RESIDENT',
    'GL_TEXTURE_COMPARE_MODE',
    'GL_TEXTURE_COMPARE_FUNC',
    'GL_DEPTH_TEXTURE_MODE',
    'GL_GENERATE_MIPMAP'
]

print("""
void crRecDumpTexParam(CR_RECORDER *pRec, CRContext *ctx, GLenum enmTarget)
{
    GLfloat afBuf[4];
    char acBuf[1024];
    unsigned int cComponents;
    crDmpStrF(pRec->pDumper, "==TEX_PARAM for target(0x%x)==", enmTarget);
""")
for pname in texparam_names:
    print("\tcComponents = crStateHlpComponentsCount(%s);" % pname)
    print("\tAssert(cComponents <= RT_ELEMENTS(afBuf));")
    print("\tmemset(afBuf, 0, sizeof (afBuf));")
    print("\tpRec->pDispatch->GetTexParameterfv(enmTarget, %s, afBuf);" % pname)
    print("\tcrDmpFormatArray(acBuf, sizeof (acBuf), \"%f\", sizeof (afBuf[0]), afBuf, cComponents);")
    print("\tcrDmpStrF(pRec->pDumper, \"%s = %%s;\", acBuf);" % pname)
print("""
    crDmpStrF(pRec->pDumper, "==Done TEX_PARAM for target(0x%x)==", enmTarget);
}
""")

print("""
void crRecDumpTexEnv(CR_RECORDER *pRec, CRContext *ctx)
{
    GLfloat afBuf[4];
    char acBuf[1024];
    unsigned int cComponents;
    crDmpStrF(pRec->pDumper, "==TEX_ENV==");
""")

for target in sorted(texenv_mappings.keys()):
    print("\tcrDmpStrF(pRec->pDumper, \"===%s===\");" % target)
    values = texenv_mappings[target]
    for pname in values:
        print("\tcComponents = crStateHlpComponentsCount(%s);" % pname)
        print("\tAssert(cComponents <= RT_ELEMENTS(afBuf));")
        print("\tmemset(afBuf, 0, sizeof (afBuf));")
        print("\tpRec->pDispatch->GetTexEnvfv(%s, %s, afBuf);" % (target, pname))
        print("\tcrDmpFormatArray(acBuf, sizeof (acBuf), \"%f\", sizeof (afBuf[0]), afBuf, cComponents);")
        print("\tcrDmpStrF(pRec->pDumper, \"%s = %%s;\", acBuf);" % pname)
    print("\tcrDmpStrF(pRec->pDumper, \"===Done %s===\");" % target)
print("""
    crDmpStrF(pRec->pDumper, "==Done TEX_ENV==");
}
""")


print("""
void crRecDumpTexGen(CR_RECORDER *pRec, CRContext *ctx)
{
    GLdouble afBuf[4];
    char acBuf[1024];
    unsigned int cComponents;
    crDmpStrF(pRec->pDumper, "==TEX_GEN==");
""")

for coord in texgen_coords:
    print("\tcrDmpStrF(pRec->pDumper, \"===%s===\");" % coord)
    for pname in texgen_names:
        print("\tcComponents = crStateHlpComponentsCount(%s);" % pname)
        print("\tAssert(cComponents <= RT_ELEMENTS(afBuf));")
        print("\tmemset(afBuf, 0, sizeof (afBuf));")
        print("\tpRec->pDispatch->GetTexGendv(%s, %s, afBuf);" % (coord, pname))
        print("\tcrDmpFormatArray(acBuf, sizeof (acBuf), \"%f\", sizeof (afBuf[0]), afBuf, cComponents);")
        print("\tcrDmpStrF(pRec->pDumper, \"%s = %%s;\", acBuf);" % pname)
    print("\tcrDmpStrF(pRec->pDumper, \"===Done %s===\");" % coord)
print("""
    crDmpStrF(pRec->pDumper, "==Done TEX_GEN==");
}
""")
