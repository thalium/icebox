#!/usr/bin/python3
# Tries to make sense of a group of timeing runs.
# From running a debug dwarfdump.
# David Anderson March 2013.

import sys

def myreadin(name):
  l = []
  try:
    file = open(name,"r");
  except IOError as message:
    print(" File could not be opened:", message,file=sys.stderr)
    sys.exit(1)
  while(1):
    try:
      rec = file.readline()
    except:
      break
    if len(rec) < 1:
      break
    line = rec.strip()
    if len(line) < 1:
       # Ignore empty lines
       continue
    l += [line]
    #print("line ",line)
  return l

def splitup(rec):
  w = rec.split()
  rest =  ""
  ct=2
  while ct < len(w):
    rest = rest + " "  +w[ct]
    ct += 1
  return (len(w),float(w[0]),float(w[1]),rest)

def computebasename(n):
  s = n.split("/")
  v = len(s)
  if v  == 1:
    return n
  return s[v-1]

def absdiff(a,b):
  if float(a) < float(b):
    return float(b) - float(a)
  return float(a) - float(b)
  
ct = 0
print( len(sys.argv))
if len(sys.argv) <= 1:
  print("Nothing to do")
  sys.exit(1)
argn = 1;
filenames=[]
files=[]
filelen=0;
minrtdiff = 1.0
minsizediff = 50
while argn < len(sys.argv):
  name = sys.argv[argn]
  if name == "-drt":
    argn += 1
    minrtdiff = float(sys.argv[argn])
    argn += 1
    continue
  if name == "-dsz":
    argn += 1
    minsizediff = float(sys.argv[argn])
    argn += 1
    continue
  print("argv[",argn,"] = ", name)
  r = myreadin(name)
  if filelen == 0:
    filelen = len(r)
  else:
    if filelen != len(r):
       print("file len mismatch",filelen, " vs ",len(r), " on ",name)
  print(len(r))
  basename = computebasename(name)
  filenames += [basename]
  files += [r]
  argn += 1
print(len(files)," files read")
lnum=0
while lnum < filelen:
  fnum = 0;
  basell = 0
  basernt = 0.0
  basesz = 0.0
  basedet = ""
  basename = ""
  linereport="n"
  while fnum < len(filenames):
    name = filenames[fnum]
    #print(" line ",lnum," file ",name)
    lines = files[fnum]
    ll,rnt,sz,det = splitup(lines[lnum])
    if fnum == 0:
       basename = name
       basell = ll
       basernt = rnt
       basesz = sz
       basedet = det
    else:
       if det != basedet:
          print("mismatch line ",lnum, "file ", basename, " vs ",name)
          print basedet , "  vs ", det
          sys.exit(1)
       rntdiff = absdiff(rnt,basernt)
       szdiff = absdiff(basesz,sz)
       if rntdiff < minrtdiff:
         if szdiff < minsizediff:
           fnum = fnum + 1
           continue
       if linereport == "n":
          linereport = "y"
          print " "
       print basename, basernt, basesz, 
       print name, rnt,sz,det
    fnum += 1
  lnum += 1

sys.exit(0)
  
