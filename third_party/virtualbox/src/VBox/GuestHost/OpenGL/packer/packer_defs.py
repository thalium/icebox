# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

# This script generates the packer/packer.def file.

import sys
import cPickle

import apiutil


apiutil.CopyrightDef()

print "DESCRIPTION \"\""
print "EXPORTS"

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")
for func_name in keys:
	if apiutil.CanPack(func_name):
		print "crPack%s" % func_name
		print "crPack%sSWAP" % func_name

functions = [
	'crPackVertexAttrib1dARBBBOX',
	'crPackVertexAttrib1dvARBBBOX',
	'crPackVertexAttrib1fARBBBOX',
	'crPackVertexAttrib1fvARBBBOX',
	'crPackVertexAttrib1sARBBBOX',
	'crPackVertexAttrib1svARBBBOX',
	'crPackVertexAttrib2dARBBBOX',
	'crPackVertexAttrib2dvARBBBOX',
	'crPackVertexAttrib2fARBBBOX',
	'crPackVertexAttrib2fvARBBBOX',
	'crPackVertexAttrib2sARBBBOX',
	'crPackVertexAttrib2svARBBBOX',
	'crPackVertexAttrib3dARBBBOX',
	'crPackVertexAttrib3dvARBBBOX',
	'crPackVertexAttrib3fARBBBOX',
	'crPackVertexAttrib3fvARBBBOX',
	'crPackVertexAttrib3sARBBBOX',
	'crPackVertexAttrib3svARBBBOX',
	'crPackVertexAttrib4dARBBBOX',
	'crPackVertexAttrib4dvARBBBOX',
	'crPackVertexAttrib4fARBBBOX',
	'crPackVertexAttrib4fvARBBBOX',
	'crPackVertexAttrib4sARBBBOX',
	'crPackVertexAttrib4svARBBBOX',
	'crPackVertexAttrib4usvARBBBOX',
	'crPackVertexAttrib4ivARBBBOX',
	'crPackVertexAttrib4uivARBBBOX',
	'crPackVertexAttrib4bvARBBBOX',
	'crPackVertexAttrib4ubvARBBBOX',
	'crPackVertexAttrib4NusvARBBBOX',
	'crPackVertexAttrib4NsvARBBBOX',
	'crPackVertexAttrib4NuivARBBBOX',
	'crPackVertexAttrib4NivARBBBOX',
	'crPackVertexAttrib4NubvARBBBOX',
	'crPackVertexAttrib4NbvARBBBOX',
	'crPackVertexAttrib4NubARBBBOX',
	'crPackVertex2dBBOX',
	'crPackVertex2dvBBOX',
	'crPackVertex2fBBOX',
	'crPackVertex2fvBBOX',
	'crPackVertex2iBBOX',
	'crPackVertex2ivBBOX',
	'crPackVertex2sBBOX',
	'crPackVertex2svBBOX',
	'crPackVertex3dBBOX',
	'crPackVertex3dvBBOX',
	'crPackVertex3fBBOX',
	'crPackVertex3fvBBOX',
	'crPackVertex3iBBOX',
	'crPackVertex3ivBBOX',
	'crPackVertex3sBBOX',
	'crPackVertex3svBBOX',
	'crPackVertex4dBBOX',
	'crPackVertex4dvBBOX',
	'crPackVertex4fBBOX',
	'crPackVertex4fvBBOX',
	'crPackVertex4iBBOX',
	'crPackVertex4ivBBOX',
	'crPackVertex4sBBOX',
	'crPackVertex4svBBOX',
	'crPackVertexAttrib1dARBBBOX_COUNT',
	'crPackVertexAttrib1dvARBBBOX_COUNT',
	'crPackVertexAttrib1fARBBBOX_COUNT',
	'crPackVertexAttrib1fvARBBBOX_COUNT',
	'crPackVertexAttrib1sARBBBOX_COUNT',
	'crPackVertexAttrib1svARBBBOX_COUNT',
	'crPackVertexAttrib2dARBBBOX_COUNT',
	'crPackVertexAttrib2dvARBBBOX_COUNT',
	'crPackVertexAttrib2fARBBBOX_COUNT',
	'crPackVertexAttrib2fvARBBBOX_COUNT',
	'crPackVertexAttrib2sARBBBOX_COUNT',
	'crPackVertexAttrib2svARBBBOX_COUNT',
	'crPackVertexAttrib3dARBBBOX_COUNT',
	'crPackVertexAttrib3dvARBBBOX_COUNT',
	'crPackVertexAttrib3fARBBBOX_COUNT',
	'crPackVertexAttrib3fvARBBBOX_COUNT',
	'crPackVertexAttrib3sARBBBOX_COUNT',
	'crPackVertexAttrib3svARBBBOX_COUNT',
	'crPackVertexAttrib4dARBBBOX_COUNT',
	'crPackVertexAttrib4dvARBBBOX_COUNT',
	'crPackVertexAttrib4fARBBBOX_COUNT',
	'crPackVertexAttrib4fvARBBBOX_COUNT',
	'crPackVertexAttrib4sARBBBOX_COUNT',
	'crPackVertexAttrib4svARBBBOX_COUNT',
	'crPackVertexAttrib4usvARBBBOX_COUNT',
	'crPackVertexAttrib4ivARBBBOX_COUNT',
	'crPackVertexAttrib4uivARBBBOX_COUNT',
	'crPackVertexAttrib4bvARBBBOX_COUNT',
	'crPackVertexAttrib4ubvARBBBOX_COUNT',
	'crPackVertexAttrib4NusvARBBBOX_COUNT',
	'crPackVertexAttrib4NsvARBBBOX_COUNT',
	'crPackVertexAttrib4NuivARBBBOX_COUNT',
	'crPackVertexAttrib4NivARBBBOX_COUNT',
	'crPackVertexAttrib4NubvARBBBOX_COUNT',
	'crPackVertexAttrib4NbvARBBBOX_COUNT',
	'crPackVertexAttrib4NubARBBBOX_COUNT',
	'crPackVertex2dBBOX_COUNT',
	'crPackVertex2dvBBOX_COUNT',
	'crPackVertex2fBBOX_COUNT',
	'crPackVertex2fvBBOX_COUNT',
	'crPackVertex2iBBOX_COUNT',
	'crPackVertex2ivBBOX_COUNT',
	'crPackVertex2sBBOX_COUNT',
	'crPackVertex2svBBOX_COUNT',
	'crPackVertex3dBBOX_COUNT',
	'crPackVertex3dvBBOX_COUNT',
	'crPackVertex3fBBOX_COUNT',
	'crPackVertex3fvBBOX_COUNT',
	'crPackVertex3iBBOX_COUNT',
	'crPackVertex3ivBBOX_COUNT',
	'crPackVertex3sBBOX_COUNT',
	'crPackVertex3svBBOX_COUNT',
	'crPackVertex4dBBOX_COUNT',
	'crPackVertex4dvBBOX_COUNT',
	'crPackVertex4fBBOX_COUNT',
	'crPackVertex4fvBBOX_COUNT',
	'crPackVertex4iBBOX_COUNT',
	'crPackVertex4ivBBOX_COUNT',
	'crPackVertex4sBBOX_COUNT',
	'crPackVertex4svBBOX_COUNT',
	'crPackVertexAttribs1dvNV',
	'crPackVertexAttribs1fvNV',
	'crPackVertexAttribs1svNV',
	'crPackVertexAttribs2dvNV',
	'crPackVertexAttribs2fvNV',
	'crPackVertexAttribs2svNV',
	'crPackVertexAttribs3dvNV',
	'crPackVertexAttribs3fvNV',
	'crPackVertexAttribs3svNV',
	'crPackVertexAttribs4dvNV',
	'crPackVertexAttribs4fvNV',
	'crPackVertexAttribs4svNV',
	'crPackVertexAttribs4ubvNV',
	'crPackExpandDrawArrays',
	'crPackExpandDrawElements',
	'crPackUnrollDrawElements',
	'crPackExpandDrawRangeElements',
	'crPackExpandArrayElement',
	'crPackExpandMultiDrawArraysEXT',
	'crPackMultiDrawArraysEXT',
	'crPackMultiDrawElementsEXT',
	'crPackExpandMultiDrawElementsEXT',
	'crPackMapBufferARB',
	'crPackUnmapBufferARB' ]

for func_name in functions:
    print "%s" % func_name
    print "%sSWAP" % func_name


print """
crPackInitBuffer
crPackResetPointers
crPackAppendBuffer
crPackAppendBoundedBuffer
crPackSetBuffer
crPackSetBufferDEBUG
crPackReleaseBuffer
crPackFlushFunc
crPackFlushArg
crPackSendHugeFunc
crPackBoundsInfoCR
crPackResetBoundingBox
crPackGetBoundingBox
crPackOffsetCurrentPointers
crPackNullCurrentPointers
crPackNewContext
crPackGetContext
crPackSetContext
crPackFree
crNetworkPointerWrite
crPackCanHoldBuffer
crPackCanHoldBoundedBuffer
crPackMaxData
crPackErrorFunction
cr_packer_globals
_PackerTSD
"""
