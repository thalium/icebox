
OPTS= -g -O -std=gnu90 -Wsystem-headers -Wall  -Wsign-compare -Wno-format-y2k -W -Wno-unused-parameter -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wreturn-type -Wcast-qual -Wwrite-strings -Wswitch -Wshadow -Wunused-parameter -Wcast-align -Wchar-subscripts -Winline -Wnested-externs  -Wold-style-definition -Wno-pointer-sign
OPTS= -g  -Wall -O

CC = gcc

HDR=dwarf_tsearch.h config.h
TESTMAIN = tsearch_tester.c 
TS = dwarf_tsearchbin.c 
TSE = dwarf_tsearchepp.c
TSH = dwarf_tsearchhash.c
TSR = dwarf_tsearchred.c
TSB = dwarf_tsearchbal.c
TESTMAINOBJ = tsearch_testerstd.o

all:  binarysearch eppingerdel hashsearch gnusearch redblack balancedsearch


tsearch_testerstd.o: $(TESTMAIN) $(HDR)
	$(CC) $(OPTS) -c  $(TESTMAIN) -o tsearch_testerstd.o

dwarf_tsearchbin.o: $(TS)  $(HDR)
	$(CC) $(OPTS) -c  $(TS) 
binarysearch: dwarf_tsearchbin.o  $(TESTMAINOBJ) $(HDR)
	$(CC) $(OPTS) $(TESTMAINOBJ) dwarf_tsearchbin.o -o binarysearch

dwarf_tsearchbal.o: $(TSB) $(HDR)
	$(CC) $(OPTS) -c  $(TSB)
balancedsearch: dwarf_tsearchbal.o $(TESTMAINOBJ) $(HDR)
	$(CC) $(OPTS) $(TESTMAINOBJ)  dwarf_tsearchbal.o -o balancedsearch

dwarf_tsearchepp.o: $(TSE) $(HDR)
	$(CC) $(OPTS) -c $(TSE) 
eppingerdel: dwarf_tsearchepp.o $(TESTMAINOBJ) $(HDR)
	$(CC) $(OPTS)    $(TESTMAINOBJ) dwarf_tsearchepp.o -o eppingerdel

dwarf_tsearchhash.o: $(TSH) $(HDR)
	$(CC) $(OPTS) -c  $(TSH)
hashsearch: dwarf_tsearchhash.o $(TESTMAIN) $(HDR)
	$(CC) $(OPTS) -DHASHSEARCH -c $(TESTMAIN) -o tsearch_testerhash.o
	$(CC) $(OPTS) tsearch_testerhash.o dwarf_tsearchhash.o -o hashsearch

# Needs a special compile of tsearch_tester.
gnusearch: $(TESTMAINOBJ) $(HDR) $(TESTMAIN)
	$(CC) $(OPTS) -DLIBC_TSEARCH -c $(TESTMAIN) -o tsearch_testergnu.o
	$(CC) $(OPTS) tsearch_testergnu.o -o gnusearch

dwarf_tsearchred.o: $(TSR) $(HDR)
	$(CC) $(OPTS) -c  $(TSR) 
redblack: dwarf_tsearchred.o  $(TESTMAINOBJ) $(HDR)
	$(CC) $(OPTS) $(TESTMAINOBJ)  dwarf_tsearchred.o -o redblack

valgrind:
	valgrind  -v --leak-check=full ./binarysearch
	valgrind  -v --leak-check=full ./eppingerdel
	valgrind  -v --leak-check=full ./hashsearch
	valgrind  -v --leak-check=full ./gnusearch
	valgrind  -v --leak-check=full ./redblack
	valgrind  -v --leak-check=full ./balancedsearch

test: all
	sh RUNTEST

clean:
	rm -f junk*
	rm -f *.o 
	rm -f gnusearch
	rm -f redblack
	rm -f binarysearch
	rm -f simplesearch
	rm -f eppingerdel
	rm -f hashsearch
	rm -f balancedsearch
	rm -f testfail
	rm -f testpass
	rm -f testfailerrs
