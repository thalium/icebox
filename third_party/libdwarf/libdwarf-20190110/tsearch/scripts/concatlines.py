#!/usr/bin/python3
# extracts a couple preferred numbers from the /bin/time output
# interspersed with script output...
# From running a debug dwarfdump.
# David Anderson March 2013.


import sys

def extractnum(w):
  r = ""
  dot="n"
  for c in w:
    if c.isdigit():
       r = r + c
       continue
    if c == '.':
       dot = "y"
       r = r + c
       continue
    return r,dot
  return r,dot
    

def istime(w):
  """ Return "y" if the number looks like a time: d*.d*  """
  t,d = extractnum(w)
  if len(t) < 4:
    return "n"
  if  d == "n":
    return "n"
  if w[0].isdigit():
    if w[1].isdigit:
      return "y"
    elif w[1] == '.':
      return "y"
  return "n"

def findfinal(d):
  beforenums = "y"
  wds = d.split()
  o= ""
  o2 = ""
  for w in wds: 
    if istime(w) == "y":
      beforenums = "n"
    if beforenums == "y":
      o += " "
      o += w
      continue
    else:
      if w.endswith("user"):
          t = ""
          for c in w:
             if not c == "u": 
               t += c
             else:
               o2 += " "
               o2 += t
      elif w.endswith("maxresident)k"):
          t = ""
          for c in w:
             if not c == "m": 
               t += c
             else:
               o2 += " "
               o2 += t
      else:
          ign = ""
  return (o,o2)

final = ""
myfile = sys.stdin
ct = 0
#if len(sys.argv) > 1:
#  print("argv[1] = ", sys.argv[1])
#  v = sys.argv[1]
#  if v == "-t":
#    testonly = "y"
#  elif v == "-v":
#    testonly = "v"
#  else:
#    print("Argument: ", v, " unknown, exiting.")
#    sys.exit(1)

while 1:
  #if int(ct) > 5:
  #    break;
  ct = ct + 1
  try:
    rec = myfile.readline()
  except:
    print(final)
    break

  if len(rec)  < 1:
    f,v = findfinal(final)
    print(v,f)
    break;
  if rec.startswith("===") == 1:
    f,v = findfinal(final)
    print(v,f)
    final = rec.strip()
  else:
    if rec.startswith("Command") == 1:
      disc = rec.strip() 
    else:
      final += " "
      final += rec.strip()

