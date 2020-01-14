#! /usr/bin/env python3
#conddef.py.py

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


#Reads  
  #define x y
#lines
#and turns them into
  #ifndef x
  #define x y
  #endif

import os
import sys

def getfirstthree(s):
  out = ""
  if len(s) >= 3:
     out = "".join((s[0],s[1],s[2]))
  return out

def reader(path):
  try:
    file = open(path,"r")
  except IOError as  message:
    print("File could not be opened: ", message,file=sys.stderr)
    sys.exit(1)
  out = []
  iline = 0
  saveword = ""
  for line in file:
    iline = int(iline) + 1
    l2 = line.rstrip()
    wds = l2.split()
    if len(wds) < 3:
       continue
    if wds[0] != "#define":
      continue
    mdef = wds[1]
    val = wds[2]
    out+= [(mdef,val)] 
  file.close()
  return out
if __name__ == '__main__':
  filename = ""
  if len(sys.argv) == 3:
    if sys.argv[1] == "--infile":
      filename = sys.argv[2]
    else:
      print("argument ", sys.argv[1]," ignored")
  else:
    print("Usage: conddef.py --infile <filepath>")
    sys.exit(1)

  full_list = reader(filename)
  maxlen = 16
  for f in full_list:
    (mdef,val) = f
    dlen = len(mdef)
    if int(dlen) > int(maxlen):
       maxlen = int(dlen)
  pstr = "#define %-" + str(maxlen) + "s %s"
  initial = ""
  for f in full_list:
     (mdef,val) = f
     ourthree = getfirstthree(mdef)
     if initial != ourthree:
        print("")
        initial = ourthree
     print("#ifndef",mdef)
     print(pstr % (mdef,val))
     print("#endif")
    
