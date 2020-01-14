#! /usr/bin/env python3
#funcfindersrcs.py

#Copyright (c) 2018, David Anderson
#All rights reserved.
#
#Redistribution and use in source and binary forms, with
#or without modification, are permitted provided that the
#following conditions are met:
#
#    Redistributions of source code must retain the above
#    copyright notice, this list of conditions and the following
#    disclaimer.
#
#    Redistributions in binary form must reproduce the above
#    copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials
#    provided with the distribution.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
#CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
#INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
#OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
#CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
#NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
#OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys

#Reads the producer code source and produces a concise list
#of the implemented functions, sorted by function name.

# Run as
# funcfindersrcs.py 
# or
# funcfindersrcs.py --nonumbers


filelist =  [
"pro_alloc.c",
"pro_alloc.c",
"pro_arange.c",
"pro_die.c",
"pro_dnames.c",
"pro_encode_nm.c",
"pro_error.c",
"pro_expr.c",
"pro_finish.c",
"pro_forms.c",
"pro_frame.c",
"pro_funcs.c",
"pro_init.c",
"pro_line.c",
"pro_macinfo.c",
"pro_pubnames.c",
"pro_reloc.c",
"pro_reloc_stream.c",
"pro_reloc_symbolic.c",
"pro_section.c",
"pro_types.c",
"pro_vars.c",
"pro_weaks.c",
"dwarf_util.c",
"dwarf_util.c",
]


def looks_like_func(s):
  fwds = s.split("(")
  if len(fwds) < 2:
    #print("Not func",fwds)
    return "n","","",""
  fwds2 = fwds[0].split(" ")
  if len(fwds2) < 2:
    #print("Not func",fwds)
    return "n","","",""
  if len(fwds2) == 2:
    return "y",fwds2[1],fwds2[0],""
  if len(fwds2) == 3:
    return "y",fwds2[2],fwds2[0],fwds2[1]
  return "n","","",""

def reader(path):
  try:
    file = open(path,"r")
  except IOError as  message:
    print("File could not be opened: ", message,file=sys.stderr)
    sys.exit(1)
  out = []
  iline = 0
  prevline = ''
  all_the_lines = file.read().splitlines()
  file.close()
  #print("No. Lines:",len(all_the_lines))
  for line in all_the_lines:
    l2 = line.rstrip()
    iline = int(iline) + 1
    if l2.startswith(" ") == 1:
      continue
    if l2.startswith("/") == 1:
      continue
    if l2.startswith("dwarf_") == 1:
      l3 = l2.split("(");
      if len(l3) < 2:
        printf(" dadebug odd-a ",l3);
        continue
      if l3[1].startswith(")") == 1:
        continue
      out += [(l3[0],prev,iline,path)] 
    else:
       prev=l2.strip()
  return out
if __name__ == '__main__':
  showlinenum = "y"
  if len(sys.argv) > 1:
    if sys.argv[1] == "--nonumbers":
      showlinenum = "n"
    else:
      print("argument ", sys.argv[1],"ignored") 
  funcl = []
  for path in filelist:
    funcl += reader(path)
  funcl.sort()
  for f in funcl:
    if showlinenum == "y":
      print(f)
    else:
      print(f[0],f[1])
    
