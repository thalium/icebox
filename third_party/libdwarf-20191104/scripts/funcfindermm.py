#! /usr/bin/env python3
#funcfindermm.py

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

#Reads libdwarf2p.1.mm and produces a concise list
#of the documented functions, sorted by function name.

# Run as
# funcfindermm.py 
# or
# funcfindermm.py --nonumbers


# \f(CWint dwarf_producer_init(

def looks_like_func(s):
  fwds = s.split("(")
  if len(fwds) < 2:
    #print("Not func",fwds)
    return "n","",""
  if fwds[1].startswith(")"):
    return "n","",""
  fwds2 = fwds[0].split(" ")
  if len(fwds2) < 2:
    #print("Not func",fwds)
    return "n","",""
  return "y",fwds2[1],fwds2[0]

def reader(path):
  try:
    file = open(path,"r")
  except IOError as  message:
    print("File could not be opened: ", message,file=sys.stderr)
    sys.exit(1)
  out = []
  iline = 0
  for line in file:
    iline = int(iline) + 1
    l2 = line.rstrip()
    wds = l2.split('W');
    #if wds[0] != ".H 3 ":
    if len(wds) < 2:
      continue
    if not wds[0].startswith("\\"):
      continue
    #print(wds[0])
    if wds[0] != "\\f(C":
      continue
    isf,func,ret = looks_like_func(wds[1])
    if isf == "y":
      out += [(func,ret,iline)] 
  file.close()
  return out
if __name__ == '__main__':
  showlinenum = "y"
  if len(sys.argv) > 1:
    if sys.argv[1] == "--nonumbers":
      showlinenum = "n"
    else:
      print("argument ", sys.argv[1],"ignored") 
  funcl = reader("libdwarf2p.1.mm");

  funcl.sort()
  for f in funcl:
    if showlinenum == "y":
      print(f)
    else:
      print(f[0],f[1])
    
