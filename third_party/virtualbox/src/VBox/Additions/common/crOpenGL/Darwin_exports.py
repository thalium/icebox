# Copyright (c) 2001, Stanford University
# All rights reserved.
#
# See the file LICENSE.txt for information on redistributing this software.

import entrypoints

hacks = ["TexImage3D", "TexImage2D", "TexImage1D", "MultiDrawArrays",
         "BufferData", "BufferSubData", "GetBufferSubData" ]

entrypoints.GenerateEntrypoints(hacks)

