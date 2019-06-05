#!/usr/bin/python3
#  Copyright (c) 2016-2016 David Anderson.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the example nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS'' AND ANY
#  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#  DISCLAIMED. IN NO EVENT SHALL David Anderson BE LIABLE FOR ANY
#  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
#  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
#  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
#  OF SUCH DAMAGE.

import os
import sys
sys.path.append(os.path.abspath("/home/davea/dwarf/code/bugxml"))
import bugrecord

def ignore_this_line(d,inrecord):
  if len(d) < 1:
    if inrecord == "y":
      return "n"
    else:
      return "y"
  s = str(d)
  if s[0] == '#':
    return "y"
  return "n"

def closeouttext(bugrec,intext,text,linecount):
  if intext == 'd':
    bugrec.setdescription(text)
    return
  elif intext == 'v':
    bugrec.setvulnerability(text)
    return
  elif intext == 'r':
    bugrec.setreferences(text)
    return
  if intext == "":
    return
  print("bogus closeout line at line ",linecount)
  sys.exit(1)


def readbugs(iname):
  name = iname
  if len(name) == 0:
    name = "/home/davea/dwarf/code/bugxml/data.txt"
  try:
      file = open(name,"r")
  except IOError as message:
      print("failed to open ",name, message)

  inrecord = "n"
  linecount = 0
  text = []
  usedid ={}
  intext = ''
  bugrec = ''
  buglist = []
  while 1:
    try:
      rec = file.readline()
    except EOFError:
        break
    if len(rec) < 1:
      # eof
      break
    linecount += 1
    if ignore_this_line(rec,inrecord) == "y":
      continue
    rec = rec.rstrip()
    if inrecord == "n":
      if len(rec) == 0:
        continue
      if  rec.find(":") == -1:
        print("bogus non-blank line at line ",linecount)
        sys.exit(1)
    if inrecord == "y" and len(rec) > 0:
      # A multi line entry may have ":" in it.
      if intext != "" and rec[0] == ' ':
        s3 = ''.join(rec)
        text += [s3]
        continue
    low = rec.find(":")
    fldname = rec[0:low+1]
    fldval = rec[low+1:]
    if fldname == "id:": 
      if inrecord == "y":
        print("bogus id: at line ",linecount)
        sys.exit(1)
      inrecord = "y"
      f = fldval.strip()
      if f in usedid:
        print("Duplicate Key:",f,"Giving up.")
        sys.exit(1)
      usedid[f] = 1
      s4= ''.join(fldval)
      bugrec = bugrecord.bugrecord(s4)
    elif fldname == "cve:":
      closeouttext(bugrec,intext,text,linecount),
      intext = ""
      text = []
      s4= ''.join(fldval)
      bugrec.setcve(s4)
    elif fldname == "datereported:":
      closeouttext(bugrec,intext,text,linecount),
      intext = ""
      text = []
      s4= ''.join(fldval)
      bugrec.setdatereported(s4)
    elif fldname == "reportedby:":
      closeouttext(bugrec,intext,text,linecount),
      intext = ""
      text = []
      s4= ''.join(fldval)
      bugrec.setreportedby(s4)
    elif fldname == "vulnerability:":
      closeouttext(bugrec,intext,text,linecount),
      intext = 'v'
      text = []
      if len(fldval) > 0:
        s4= ''.join(fldval)
        text = [s4]

    elif fldname == "product:":
      closeouttext(bugrec,intext,text,linecount),
      intext = ""
      text = []
      s4= ''.join(fldval)
      bugrec.setproduct(s4)
    elif fldname == "description:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      intext = 'd'
      if len(fldval) > 0:
        s4= ''.join(fldval)
        text = [s4]

    elif fldname == "datefixed:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      intext = ""
      s4= ''.join(fldval)
      bugrec.setdatefixed(s4)
    elif fldname == "references:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      intext = 'r'
      if len(fldval) > 0:
        s4= ''.join(fldval)
        text = [s4]
    elif fldname == "gitfixid:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      intext = ""
      s4= ''.join(fldval)
      bugrec.setgitfixid(s4)
    elif fldname == "tarrelease:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      intext = ""
      s4= ''.join(fldval)
      bugrec.settarrelease(s4)
    elif fldname == "endrec:":
      closeouttext(bugrec,intext,text,linecount),
      text = []
      if inrecord == "n":
         print("bogus endrec: at line ",linecount)
         sys.exit(1)
      buglist += [bugrec]
      inrecord = "n"
      text = []
      intext = ""
      inrecord = "n"
  file.close()
  return buglist







def sort_by_id(myl):
  """ Sort the list of objects by name. """
  auxiliary = [ ( x._id, x) for x in myl ]
  auxiliary.sort()
  return [ x[1] for x in auxiliary ]


def write_line(file,l):
  file.write(l + "\n")

def write_all_lines(file,txt):
  for t in txt:
     write_line(file,t)

def generatehtml(list2,name):
  try:
    file = open(name,"w")
  except IOError as message:
    print("failed to open ",name, message)
    sys.exit(1)
  for b in list2:
    txt=b.generate_html()
    write_all_lines(file,txt)
  write_line(file,"</body>")
  write_line(file,"</html>")
  file.close()

def generatexml(list2,name):
  try:
    file = open(name,"w")
  except IOError as message:
    print("failed to open ",name, message)
    sys.exit(1)
  t = '<?xml version="1.0" encoding="us-ascii"?>'
  write_line(file,t)
  write_line(file,"<dwarfbug>")
  for b in list2:
    txt=b.generate_xml()
    write_all_lines(file,txt)
  write_line(file,"</dwarfbug>")
  file.close()




if __name__ == '__main__':
  list = readbugs("")
  list2  = sort_by_id(list)
  list2.reverse()
  #for b in list2:
  #  b.printbug()

  generatehtml(list2,"./dwarfbugtail")
  generatexml(list2,"./dwarfbuglohi.xml")

  list2.reverse()
  generatehtml(list2,"./dwarfbuglohitail")

