# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.


import sys

import apiutil

apiutil.CopyrightDef()

print """DESCRIPTION ""
EXPORTS
"""

keys = apiutil.GetDispatchedFunctions(sys.argv[1]+"/APIspec.txt")

for func_name in apiutil.AllSpecials( 'state' ):
    print "crState%s" % func_name

for func_name in apiutil.AllSpecials( 'state_feedback' ):
    print "crStateFeedback%s" % func_name

for func_name in apiutil.AllSpecials( 'state_select' ):
    print "crStateSelect%s" % func_name

print """crStateInit
crStateReadPixels
crStateGetChromiumParametervCR
crStateCreateContext
crStateCreateContextEx
crStateDestroyContext
crStateDiffContext
crStateSwitchContext
crStateMakeCurrent
crStateSetCurrent
crStateFlushFunc
crStateFlushArg
crStateDiffAPI
crStateSetCurrentPointers
crStateResetCurrentPointers
crStateCurrentRecover
crStateTransformUpdateTransform
crStateColorMaterialRecover
crStateError
crStateUpdateColorBits
crStateClientInit
crStateGetCurrent
crStateLimitsInit
crStateMergeExtensions
crStateRasterPosUpdate
crStateTextureCheckDirtyImages
crStateExtensionsInit
crStateSetExtensionString
crStateUseServerArrays
crStateUseServerArrayElements
crStateComputeVersion
crStateTransformXformPointMatrixf
crStateTransformXformPointMatrixd
crStateInitMatrixStack
crStateLoadMatrix
__currentBits
"""
