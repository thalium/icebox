\."
\." the following line may be removed if the ff ligature works on your machine
.lg 0
\." set up heading formats
.ds HF 3 3 3 3 3 2 2
.ds HP +2 +2 +1 +0 +0
.nr Hs 5
.nr Hb 5
\." ==============================================
\." Put current date in the following at each rev
.ds vE rev 2.70, December 20, 2018
\." ==============================================
\." ==============================================
.ds | |
.ds ~ ~
.ds ' '
.if t .ds Cw \&\f(CW
.if n .ds Cw \fB
.de Cf          \" Place every other arg in Cw font, beginning with first
.if \\n(.$=1 \&\*(Cw\\$1\fP
.if \\n(.$=2 \&\*(Cw\\$1\fP\\$2
.if \\n(.$=3 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP
.if \\n(.$=4 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4
.if \\n(.$=5 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP
.if \\n(.$=6 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6
.if \\n(.$=7 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP
.if \\n(.$=8 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP\\$8
.if \\n(.$=9 \&\*(Cw\\$1\fP\\$2\*(Cw\\$3\fP\\$4\*(Cw\\$5\fP\\$6\*(Cw\\$7\fP\\$8\
*(Cw
..
.nr Cl 4
.SA 1
.TL
A Consumer Library Interface to DWARF
.AF ""
.AU "David Anderson"
.PF "'\*(vE'- \\\\nP -''"
.AS 1
This document describes an interface to a library of functions
.FS
UNIX is a registered trademark of UNIX System Laboratories, Inc.
in the United States and other countries.
.FE
to access DWARF debugging information entries and DWARF line number
information (and other DWARF2/3/4/5 information).
It does not make recommendations as to how the functions
described in this document should be implemented nor does it
suggest possible optimizations.
.P
The document is oriented to reading DWARF version 2 and later.
There are certain sections which are SGI-specific (those
are clearly identified in the document).
.P
\*(vE

.AE
.MT 4

.H 1 "INTRODUCTION"
This document describes an interface to \fIlibdwarf\fP, a
library of functions to provide access to DWARF debugging information
records, DWARF line number information, DWARF address range and global
names information, weak names information, DWARF frame description
information, DWARF static function names, DWARF static variables, and
DWARF type information.
.P
The document has long mentioned the
"Unix International Programming Languages Special Interest Group"
(PLSIG), under whose auspices the
DWARF committee was formed around 1991.
"Unix International"
was disbanded in the 1990s and no longer exists.
.P
The DWARF committee published DWARF2 July 27, 1993.
.P
In the mid 1990s this document and the library it describes
(which the committee never endorsed, having decided
not to endorse or approve any particular library interface)
was made available on the internet by Silicon Graphics, Inc.
.P
In 2005 the DWARF committee began an affiliation with FreeStandards.org.
In 2007 FreeStandards.org merged with The Linux Foundation.
The DWARF committee dropped its affiliation with FreeStandards.org
in 2007 and established the dwarfstd.org website.
See "http://www.dwarfstd.org" for current
information on standardization activities
and a copy of the standard.
.H 2 "Copyright"
Copyright 1993-2006 Silicon Graphics, Inc.

Copyright 2007-2018 David Anderson.

Permission is hereby granted to
copy or republish or use any or all of this document without
restriction except that when publishing more than a small amount
of the document
please acknowledge Silicon Graphics, Inc and David Anderson.

This document is distributed in the hope that it would be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

.H 2 "Purpose and Scope"
The purpose of this document is to document a library of functions
to access DWARF debugging information. There is no effort made in
this document to address the creation of these records as those
issues are addressed separately
(see "A Producer Library Interface to DWARF").

.P
Additionally, the focus of this document is the functional interface,
and as such, implementation as well as optimization issues are
intentionally ignored.


.H 2 "Document History"
.P
A document was written about 1991 which had similar
layout and interfaces.
Written by people from Hal Corporation,
That document described a library for reading DWARF1.
The authors distributed paper copies to the committee
with the clearly expressed intent to propose the document as
a supported interface definition.
The committee decided not to pursue a library definition.
.P
SGI wrote the document you are now reading in 1993
with a similar layout and content and organization, but
it was complete document rewrite with the intent to read DWARF2
(the DWARF version then in existence).
The intent was (and is) to also cover
future revisions of DWARF.
All the function interfaces were changed
in 1994 to uniformly
return a simple integer success-code (see DW_DLV_OK etc),
generally following
the recommendations in the chapter titled "Candy Machine Interfaces"
of "Writing Solid Code", a book by
Steve Maguire (published by Microsoft Press).
.H 2 "Definitions"
DWARF debugging information entries (DIEs) are the segments of information
placed in the \f(CW.debug_*\fP sections by compilers, assemblers, and
linkage editors that, in conjunction with line number entries, are
necessary for symbolic source-level debugging.
Refer to the latest
"\fIDWARF Debugging Information Format\fP" from www.dwarfstd.org for a more
complete description of these entries.

.P
This document adopts all the terms and definitions in "\fIDWARF Debugging
Information Format\fP" versions 2,3,4, and 5.
It originally focused on the implementation at
Silicon Graphics, Inc., but now
attempts to be more generally useful.

.H 2 "Overview"
The remaining sections of this document describe the proposed interface
to \f(CWlibdwarf\fP, first by describing the purpose of additional types
defined by the interface, followed by descriptions of the available
operations.  This document assumes you are thoroughly familiar with the
information contained in the \fIDWARF Debugging Information Format\fP
document.
.P
We separate the functions into several categories to emphasize that not
all consumers want to use all the functions.  We call the categories
Debugger, Internal-level, High-level, and Miscellaneous not because one is more
important than another but as a way of making the rather large set of
function calls easier to understand.
.P
Unless otherwise specified, all functions and structures should be
taken as being designed for Debugger consumers.
.P
The Debugger Interface of this library is intended to be used by debuggers.
The interface is low-level (close to dwarf) but suppresses irrelevant detail.
A debugger will want to absorb all of some sections at startup and will
want to see little or nothing of some sections except at need.  And even
then will probably want to absorb only the information in a single compilation
unit at a time.  A debugger does not care about
implementation details of the library.
.P
The Internal-level Interface is for a DWARF prettyprinter and checker.
A
thorough prettyprinter will want to know all kinds of internal things
(like actual FORM numbers and actual offsets) so it can check for
appropriate structure in the DWARF data and print (on request) all
that internal information for human users and libdwarf authors and
compiler-writers.
Calls in this interface provide data a debugger
does not normally care about.
.P
The High-level Interface is for higher level access
(it is not really a high level interface!).
Programs such as
disassemblers will want to be able to display relevant information
about functions and line numbers without having to invest too much
effort in looking at DWARF.
.P
The miscellaneous interface is just what is left over: the error handler
functions.
.P
The following is a brief mention of the changes in this libdwarf from
the libdwarf draft for DWARF Version 1 and recent changes.

.H 2 "Items Changed"
.P
Added a libdwarf interface dwarf_errmsg_by_number()
so that places in the code that can have errors but
do not want the Dwarf_Error complexities
can report more details than just an error number.
(December 19, 2018)
.P
Now Mach-o dSYM files containing dwarf
are readable by libdwarf and their DWARF
dumped by dwarfdump.
There are no new options or choices,
libdwarf and dwarfdump notice which
kind of object they are processing.
New functions added to libdwarf.h.in:
dwarf_init_path(),dwarf_object_detector_path(),
and dwarf_object_detector_fd().
(October 24, 2018)
.P
All references to Dwarf_Frame_Op3 have been removed
as that struct was never created or available.
The new function dwarf_get_fde_info_for_reg3_b()
is documented.
(May 12, 2018)
.P
With DWARF5 it became harder to use
dwarf_srclines_data_b() as DWARF5 changed
each line table header file table to zero-based
indexing from one-based (and made the primary
file index zero).
So a new function dwarf_srclines_file_indexes()
returns values that make it easy to step through
and call dwarf_srclines_data_b() sensibly whether
the line table is DWARF2,3,4, or 5.
(March 23, 2018)
.P
Added COMDAT support.
Recent compilers generate COMDAT sections (for some DWARF
information) routinely so this became important
recently. The new libdwarf COMDAT support
extends the groupnumber
idea as suggested just below.
(May 17, 2017)
.P
Adding dwarf_init_b() and dwarf_elf_init_b()
and dwarf_object_init_b() with a groupnumber
option added. DWARF5 adds split-dwarf and
we call original sections like .debug_info
group one and new sections like .debug_info.dwo
group two.
It has not escaped our attention that this
numbering can be extended to deal with
Elf COMDAT
section groups of DWARF information, though
COMDAT groups are not currently supported.
(April 02, 2017)
.P
Adding support for DWARF5 .debug_loc.dwo
and split dwarf range tables.
Added dwarf_get_offset_size().
(November 08, 2015)
.P
Adding support for reading DWARF5 line tables
and GNU two-level line tables.
The function dwarf_srclines() still works
but those using DWARF4 or DWARF5 are
advised to switch to dwarf_srclines_b().
dwarf_srclines()
cannot handle skeleton line tables sensibly
and a new interface was needed for two-level
line tables so the new approach satisfies both.
(October 5,2015)
.P
Adding support for Package Files (DWARF5)
to enable access of address data using DW_FORM_addrx.
See dwarf_set_tied_dbg().
(September 13, 2015)
.P
Adding some DWARF5 support and improved DWP Package File
support, using dwarf_next_cu_header_d().
.P
Added a note about dwarf_errmsg(): the string pointer
returned should be considered ephemeral, not a
string which remains valid permanently.
User code should print it or copy it before calling
other libdwarf functions on the specific Dwarf_Debug
instance.
(May 15, 2014)
.P
Added a printf-callback so libdwarf will not actually print
to stdout.  Added dwarf_highpc_b()
so return of a DWARF4 DW_AT_high_pc of class constant
can be returned properly.
(August 15 2013)
.P
Defined how the new operator DW_OP_GNU_const_type is handled.
(January 26 2013)
.P
Added dwarf_loclist_from_expr_b()
function which adds arguments of the DWARF version
(2 for DWARF2, etc) and the offset size
to the dwarf_loclist_from_expr_a()
function.  Because the DW_OP_GNU_implicit_pointer
opcode is defined differently for DWARF2 than for
later versions.
(November 2012)
.P
Added new functions (some for libdwarf client code)
and internal logic support for the
DWARF4 .debug_types section.
The new functions are
dwarf_next_cu_header_c(),
dwarf_siblingof_b(), dwarf_offdie_b(),
dwarf_get_cu_die_offset_given_cu_header_offset_b(),
dwarf_get_die_infotypes_flag(),
dwarf_get_section_max_offsets_b().
.P
New functions and logic support additional detailed error reporting
so that more compiler bugs can be reported sensibly
by consumer code (as opposed
to having libdwarf just assume
things are ok and blindly continuing on
with erroneous data).
November 20, 2010
.P
It seems impossible to default to both DW_FRAME_CFA_COL
and DW_FRAME_CFA_COL3 in a single build of libdwarf,
so the default is now unambiguously DW_FRAME_CFA_COL3
unless the configure option --enable-oldframecol
is specified at configure time.
The function dwarf_set_frame_cfa_value()
may be used to override the default : using that function gives
consumer applications full control (its use is highly
recommended).
(January 17,2010)
.P
Added dwarf_set_reloc_application() and the default
automatic application of Elf 'rela' relocations
to DWARF sections (such rela sections appear in .o files, not
in executables or shared objects, in general).
The  dwarf_set_reloc_application() routine lets a consumer
turn off the automatic application of 'rela' relocations
if desired (it is not clear why anyone would really want to do that,
but possibly a consumer could write its own relocation application).
An example application that traverses a set of DIEs
was added to the new dwarfexample directory (not
in this libdwarf directory, but in parallel to it).
(July 10, 2009)
.P
Added dwarf_get_TAG_name() (and the FORM AT and so on)
interface functions so applications can get the string
of the TAG, Attribute, etc as needed. (June 2009)
.P
Added dwarf_get_ranges_a() and dwarf_loclist_from_expr_a()
functions which add arguments allowing a correct address_size
when the address_size varies by compilation unit (a varying
address_size is quite rare as of May 2009).
(May 2009)
.P
Added dwarf_set_frame_same_value(), and
dwarf_set_frame_undefined_value() to complete
the set of frame-information functions needed to allow
an application get all frame information
returned correctly (meaning that it
can be correctly interpreted) for all ABIs.
Documented dwarf_set_frame_cfa_value().
Corrected spelling to dwarf_set_frame_rule_initial_value().
(April 2009).
.P
Added support for various DWARF3 features, but primarily
a new frame-information interface tailorable at run-time
to more than a single ABI.
See dwarf_set_frame_rule_initial_value(), dwarf_set_frame_rule_table_size(),
dwarf_set_frame_cfa_value().
See also dwarf_get_fde_info_for_reg3() and
dwarf_get_fde_info_for_cfa_reg3().  (April 2006)
.P
Added support for DWARF3 .debug_pubtypes section.
Corrected various leaks (revising dealloc() calls, adding
new functions) and corrected dwarf_formstring() documentation.
.P
Added dwarf_srclines_dealloc() as the previous deallocation
method documented for data returned by
dwarf_srclines() was incapable of freeing
all the allocated storage (14 July 2005).
.P
dwarf_nextglob(), dwarf_globname(), and dwarf_globdie() were all changed
to operate on the items in the .debug_pubnames section.
.P
All functions were modified to return solely an error code.
Data is returned through pointer arguments.
This makes writing safe and correct library-using-code far easier.
For justification for this approach, see
the chapter titled "Candy Machine Interfaces"
in the book "Writing Solid Code" by
Steve Maguire.

.H 2 "Items Removed"
.P
Dwarf_Type
was removed since types are no longer special.
.P
dwarf_typeof()
was removed since types are no longer special.
.P
Dwarf_Ellist
was removed since element lists no longer are a special format.
.P
Dwarf_Bounds
was removed since bounds have been generalized.
.P
dwarf_nextdie()
was replaced by dwarf_next_cu_header() to reflect the
real way DWARF is organized.
The dwarf_nextdie() was only useful for getting to compilation
unit beginnings, so it does not seem harmful to remove it in favor
of a more direct function.
.P
dwarf_childcnt() is removed on grounds
that no good use was apparent.
.P
dwarf_prevline() and dwarf_nextline() were removed on grounds this
is better left to a debugger to do.
Similarly, dwarf_dieline() was removed.
.P
dwarf_is1stline() was removed as it was not meaningful for the
revised DWARF line operations.
.P
Any libdwarf implementation might well decide to support all the
removed functionality and to retain the DWARF Version 1 meanings
of that functionality.
This would be difficult because the
original libdwarf draft
specification used traditional C library interfaces which
confuse the values returned by successful calls with
exceptional conditions like failures and 'no more data' indications.

.H 2 "Revision History"
.VL 15
.LI "October 2018"
Added support for reading MacOS dSYM object files.
.LI "2017"
Added support for nearly all of DWARF5.
.LI "July 2014"
Added support for the .gdb_index section and
started support for the .debug_cu_index and .debug_tu_index
sections.
.LI "October 2011"
DWARF4 support for reading .debug_types added.
.LI "March 93"
Work on DWARF2 SGI draft begins
.LI "June 94"
The function returns are changed to return an error/success code
only.
.LI "April 2006:
Support for DWARF3 consumer operations is close to completion.
.LI "November 2010:
Added various new functions and improved error checking.
.LI "March 2017:
Adding support for DWARF5 split dwarf.
.LE

.H 1 "Types Definitions"

.H 2 "General Description"
The \fIlibdwarf.h\fP header file contains typedefs and preprocessor
definitions of types and symbolic names used to reference objects
of \fIlibdwarf\fP. The types defined by typedefs contained in
\fIlibdwarf.h\fP all use the convention of adding \f(CWDwarf_\fP
as a prefix and can be placed in three categories:

.BL
.LI
Scalar types : The scalar types defined in \fIlibdwarf.h\fP are
defined primarily for notational convenience and identification.
Depending on the individual definition, they are interpreted as a
value, a pointer, or as a flag.
.LI
Aggregate types : Some values can not be represented by a single
scalar type; they must be represented by a collection of, or as a
union of, scalar and/or aggregate types.
.LI
Opaque types : The complete definition of these types is intentionally
omitted; their use is as handles for query operations, which will yield
either an instance of another opaque type to be used in another query, or
an instance of a scalar or aggregate type, which is the actual result.
.P

.H 2 "Scalar Types"
The following are the defined by \fIlibdwarf.h\fP:

.DS
\f(CW
typedef int                Dwarf_Bool;
typedef unsigned long long Dwarf_Off;
typedef unsigned long long Dwarf_Unsigned;
typedef unsigned short     Dwarf_Half;
typedef unsigned char      Dwarf_Small;
typedef signed long long   Dwarf_Signed;
typedef unsigned long long Dwarf_Addr;
typedef void 		  *Dwarf_Ptr;
typedef void   (*Dwarf_Handler)(Dwarf_Error error, Dwarf_Ptr errarg);
.DE

.nr aX \n(Fg+1
Dwarf_Ptr is an address for use by the host program calling the library,
not for representing pc-values/addresses within the target object file.
Dwarf_Addr is for pc-values within the target object file.  The sample
scalar type assignments above are for a \fIlibdwarf.h\fP that can read
and write
32-bit or 64-bit binaries on a 32-bit or 64-bit host machine.
The types must be  defined appropriately
for each implementation of libdwarf.
A description of these scalar types in the SGI/MIPS
environment is given in Figure \n(aX.

.DS
.TS
center box, tab(:);
lfB lfB lfB lfB
l c c l.
NAME:SIZE:ALIGNMENT:PURPOSE
_
Dwarf_Bool:4:4:Boolean states
Dwarf_Off:8:8:Unsigned file offset
Dwarf_Unsigned:8:8:Unsigned large integer
Dwarf_Half:2:2:Unsigned medium integer
Dwarf_Small:1:1:Unsigned small integer
Dwarf_Signed:8:8:Signed large integer
Dwarf_Addr:8:8:Program address
:::(target program)
Dwarf_Ptr:4|8:4|8:Dwarf section pointer
:::(host program)
Dwarf_Handler:4|8:4|8:Pointer to
:::error handler function
.TE
.FG "Scalar Types"
.DE

.H 2 "Aggregate Types"
The following aggregate types are defined by
\fIlibdwarf.h\fP:
\f(CWDwarf_Loc\fP,
\f(CWDwarf_Locdesc\fP,
\f(CWDwarf_Block\fP,
\f(CWDwarf_Frame_Op\fP.
\f(CWDwarf_Regtable\fP.
\f(CWDwarf_Regtable3\fP.
While most of \f(CWlibdwarf\fP acts on or returns simple values or
opaque pointer types, this small set of structures seems useful.
Yet, at the same time, these public structures are inflexible
as any change in format or content
breaks binary (and possibly source in some cases)
compatibility.

.H 3 "Location Record"
The \f(CWDwarf_Loc\fP type identifies a single atom of a location description
or a location expression.

.DS
\f(CWtypedef struct {
    Dwarf_Small        lr_atom;
    Dwarf_Unsigned     lr_number;
    Dwarf_Unsigned     lr_number2;
    Dwarf_Unsigned     lr_offset;
} Dwarf_Loc;\fP
.DE

The \f(CWlr_atom\fP identifies the atom corresponding to the \f(CWDW_OP_*\fP
definition in \fIdwarf.h\fP and it represents the operation to be performed
in order to locate the item in question.

.P
The \f(CWlr_number\fP field is the operand to be used in the calculation
specified by the \f(CWlr_atom\fP field; not all atoms use this field.
Some atom operations imply signed numbers so it is necessary to cast
this to a \f(CWDwarf_Signed\fP type for those operations.

.P
The \f(CWlr_number2\fP field is the second operand specified by the
\f(CWlr_atom\fP field; only \f(CWDW_OP_BREGX\fP has this field.  Some
atom operations imply signed numbers so it may be necessary to cast
this to a \f(CWDwarf_Signed\fP type for those operations.
.P
For a \f(CWDW_OP_implicit_value\fP operator the \f(CWlr_number2\fP
field is a pointer to the bytes of the value. The field pointed to
is \f(CWlr_number\fP bytes long.  There is no explicit terminator.
Do not attempt to \f(CWfree\fP the bytes which \f(CWlr_number2\fP
points at and do not alter those bytes. The pointer value
remains valid till the open Dwarf_Debug is closed.
This is a rather ugly use of a host integer to hold a pointer.
You will normally have to do a 'cast' operation to use the value.
.P
For a \f(CWDW_OP_GNU_const_type\fP operator the \f(CWlr_number2\fP
field is a pointer to a block with an initial
unsigned byte giving the number of bytes
following, followed immediately that number of const
value bytes.
There is no explicit terminator.
Do not attempt to \f(CWfree\fP the bytes which \f(CWlr_number2\fP
points at and do not alter those bytes. The pointer value
remains valid till the open Dwarf_Debug is closed.
This is a rather ugly use of a host integer to hold a pointer.
You will normally have to do a 'cast' operation to use the value.
.P
The \f(CWlr_offset\fP field is the byte offset (within the block the
location record came from) of the atom specified by the \f(CWlr_atom\fP
field.  This is set on all atoms.  This is useful for operations
\f(CWDW_OP_SKIP\fP and \f(CWDW_OP_BRA\fP.

.H 3 "Location Description"
The \f(CWDwarf_Locdesc\fP type represents an ordered list of
\f(CWDwarf_Loc\fP records used in the calculation to locate
an item.  Note that in many cases, the location can only be
calculated at runtime of the associated program.

.DS
\f(CWtypedef struct {
    Dwarf_Addr        ld_lopc;
    Dwarf_Addr        ld_hipc;
    Dwarf_Unsigned    ld_cents;
    Dwarf_Loc*        ld_s;
} Dwarf_Locdesc;\fP
.DE

The \f(CWld_lopc\fP and \f(CWld_hipc\fP fields provide an address range for
which this location descriptor is valid.  Both of these fields are set to
\fIzero\fP if the location descriptor is valid throughout the scope of the
item it is associated with.  These addresses are virtual memory addresses,
not offsets-from-something.  The virtual memory addresses do not account
for dso movement (none of the pc values from libdwarf do that, it is up to
the consumer to do that).

.P
The \f(CWld_cents\fP field contains a count of the number of \f(CWDwarf_Loc\fP
entries pointed to by the \f(CWld_s\fP field.

.P
The \f(CWld_s\fP field points to an array of \f(CWDwarf_Loc\fP records.

.H 3 "Data Block"
.SP
The \f(CWDwarf_Block\fP type is used to contain the value of an attribute
whose form is either \f(CWDW_FORM_block1\fP, \f(CWDW_FORM_block2\fP,
\f(CWDW_FORM_block4\fP, \f(CWDW_FORM_block8\fP, or \f(CWDW_FORM_block\fP.
Its intended use is to deliver the value for an attribute of any of these
forms.

.DS
\f(CWtypedef struct {
    Dwarf_Unsigned     bl_len;
    Dwarf_Ptr          bl_data;
} Dwarf_Block;\fP
.DE

.P
The \f(CWbl_len\fP field contains the length in
bytes of the data pointed
to by the \f(CWbl_data\fP field.

.P
The \f(CWbl_data\fP field contains a pointer
to the uninterpreted data.
Since we use  a
\f(CWDwarf_Ptr\fP here one must copy the pointer to
some other type (typically an \f(CWunsigned char
*\fP) so one can add increments to index through
the data.
The data pointed to by \f(CWbl_data\fP
is not necessarily at any useful alignment.

.H 3 "Frame Operation Codes: DWARF 2"
This interface is adequate for DWARF2 but
not entirely suitable for DWARF3 or later.
A new (functional) interface is needed.
This DWARF2 interface is not sufficient
but at present is the only available interface.
.P
See also the section "Low Level Frame Operations" below.
.P
The DWARF2 \f(CWDwarf_Frame_Op\fP type is
used to contain the data of a single
instruction of an instruction-sequence of
low-level information from the
section containing frame information.
This is ordinarily used by
Internal-level Consumers trying to print everything in detail.

.DS
\f(CWtypedef struct {
    Dwarf_Small  fp_base_op;
    Dwarf_Small  fp_extended_op;
    Dwarf_Half   fp_register;
    Dwarf_Signed fp_offset;
    Dwarf_Offset fp_instr_offset;
} Dwarf_Frame_Op;
.DE

\f(CWfp_base_op\fP is the 2-bit basic op code.
\f(CWfp_extended_op\fP is
the 6-bit extended opcode (if
\f(CWfp_base_op\fP indicated there was an
extended op code) and is zero otherwise.
.P
\f(CWfp_register\fP
is any (or the first) register value as defined
in the \f(CWCall frame instruction encodings\fP
in the \f(CWdwarf\fP document
(in DWARF3 see Figure 40,in DWARF5 see table 7.29).
If not used with the operation it is 0.
.P
\f(CWfp_offset\fP
is the address, delta, offset, or second register as defined
in the
\f(CWCall frame instruction encodings\fP
documentation.
If this is an \f(CWaddress\fP then the value should be cast to
\f(CW(Dwarf_Addr)\fP before being used.

In any implementation this field *must* be as large as the
largest of Dwarf_Ptr, Dwarf_Signed, and Dwarf_Addr
for this to work properly.
If not used with the op it is 0.
If the fp_extended_op is
\f(CWDW_CFA_def_cfa\fP
or
\f(CWDW_CFA_val_expression\fP
or
\f(CWDW_CFA_expression\fP
then
\f(CWfp_offset\fP
is a pointer to an expression block in the in-memory
copy of the frame section.
.P
\f(CWfp_instr_offset\fP is the byte_offset (within the instruction
stream of the frame instructions) of this operation.  It starts at 0
for a given frame descriptor.

.H 3 "Frame Regtable: DWARF 2"
This interface is adequate for DWARF2
and MIPS but not for DWARF3 or later.
A separate and preferred interface usable for DWARF3 and for DWARF2
is described below.
See also the section "Low Level Frame Operations" below.
.P
The \f(CWDwarf_Regtable\fP type is used to contain the
register-restore information for all registers at a given
PC value.
Normally used by debuggers.
If you wish to default to this interface and to the use
of DW_FRAME_CFA_COL, specify --enable_oldframecol
at libdwarf configure time.
Or add a call dwarf_set_frame_cfa_value(dbg,DW_FRAME_CFA_COL)
after your dwarf_init_b() call, this call replaces the
default libdwarf-compile-time value with DW_FRAME_CFA_COL.
.DS
/* DW_REG_TABLE_SIZE must reflect the number of registers
 *(DW_FRAME_LAST_REG_NUM) as defined in dwarf.h
 */
#define DW_REG_TABLE_SIZE  <fill in size here, 66 for MIPS/IRIX>
\f(CWtypedef struct {
    struct {
        Dwarf_Small         dw_offset_relevant;
        Dwarf_Half          dw_regnum;
        Dwarf_Addr          dw_offset;
    }                       rules[DW_REG_TABLE_SIZE];
} Dwarf_Regtable;\fP
.DE
.P
The array is indexed by register number.
The field values for each index are described next.
For clarity we describe the field values for index rules[M]
(M being any legal array element index).
.P
\f(CWdw_offset_relevant\fP is non-zero to indicate the
\f(CWdw_offset\fP
field is meaningful.
If zero then the
\f(CWdw_offset\fP is zero
and should be ignored.
.P
\f(CWdw_regnum \fPis the register number applicable.
If \f(CWdw_offset_relevant\fP is zero, then this is the register
number of the register containing the value for register M.
If \f(CWdw_offset_relevant\fP is non-zero, then this is
the register number of the register to use as a base (M may be
DW_FRAME_CFA_COL, for example) and the \f(CWdw_offset\fP
value applies.
The value of register M is therefore
the value of register \f(CWdw_regnum\fP.
.P
\f(CWdw_offset\fP should be ignored if
\f(CWdw_offset_relevant\fP is zero.
If \f(CWdw_offset_relevant\fP is non-zero, then
the consumer code should add the value to
the value of the register \f(CWdw_regnum\fP to produce the
value.

.H 3 "Frame Operation Codes: DWARF 3 (for DWARF2 and later )
This interface was intended
to be  adequate for DWARF3 and for DWARF2 (and DWARF4)
but was never implemented.


.H 3 "Frame Regtable: DWARF 3 (for DWARF2 and later)"
This interface is adequate for DWARF2 and later versions.
It is new in libdwarf as of April 2006.
The default configure of libdwarf
inserts DW_FRAME_CFA_COL3 as the default CFA column.
Or add a call dwarf_set_frame_cfa_value(dbg,DW_FRAME_CFA_COL3)
after your dwarf_init_b() call, this call replaces the
default libdwarf-compile-time value with DW_FRAME_CFA_COL3.
.P
The \f(CWDwarf_Regtable3\fP type is used to contain the
register-restore information for all registers at a given
PC value.
Normally used by debuggers.
.DS
\f(CWtypedef struct Dwarf_Regtable_Entry3_s {
    Dwarf_Small         dw_offset_relevant;
    Dwarf_Small         dw_value_type;
    Dwarf_Half          dw_regnum;
    Dwarf_Unsigned      dw_offset_or_block_len;
    Dwarf_Ptr           dw_block_ptr;
}Dwarf_Regtable_Entry3;

typedef struct Dwarf_Regtable3_s {
    struct Dwarf_Regtable_Entry3_s   rt3_cfa_rule;
    Dwarf_Half                       rt3_reg_table_size;
    struct Dwarf_Regtable_Entry3_s * rt3_rules;
} Dwarf_Regtable3;\fP

.DE
.P
The array is indexed by register number.
The field values for each index are described next.
For clarity we describe the field values for index rules[M]
(M being any legal array element index).
(DW_FRAME_CFA_COL3  DW_FRAME_SAME_VAL, DW_FRAME_UNDEFINED_VAL
are not legal array indexes, nor is any index < 0 or >=
rt3_reg_table_size);
The caller  of routines using this
struct must create data space for rt3_reg_table_size entries
of struct Dwarf_Regtable_Entry3_s and arrange that
rt3_rules points to that space and that rt3_reg_table_size
is set correctly.  The caller need not (but may)
initialize the contents of the rt3_cfa_rule or the rt3_rules array.
The following applies to each rt3_rules rule M:
.P
.in +4
\f(CWdw_regnum\fP is the register number applicable.
If \f(CWdw_regnum\fP is DW_FRAME_UNDEFINED_VAL, then the
register I has undefined value.
If \f(CWdw_regnum\fP is DW_FRAME_SAME_VAL, then the
register I has the same value as in the previous frame.
.P
If \f(CWdw_regnum\fP is neither of these two,
then the following apply:
.P
.P
\f(CWdw_value_type\fP determines the meaning of the other fields.
It is one of DW_EXPR_OFFSET (0),
DW_EXPR_VAL_OFFSET(1), DW_EXPR_EXPRESSION(2) or
DW_EXPR_VAL_EXPRESSION(3).

.P
If \f(CWdw_value_type\fP is DW_EXPR_OFFSET (0) then
this is as in DWARF2 and the offset(N) rule  or the register(R)
rule
of the DWARF3 and DWARF2 document applies.
The value is either:
.in +4
If \f(CWdw_offset_relevant\fP is non-zero, then \f(CWdw_regnum\fP
is effectively ignored but must be identical to
DW_FRAME_CFA_COL3 (and the \f(CWdw_offset\fP value applies.
The value of register M is therefore
the value of CFA plus the value
of \f(CWdw_offset\fP.   The result of the calculation
is the address in memory where the value of register M resides.
This is the offset(N) rule of the DWARF2 and DWARF3 documents.
.P
\f(CWdw_offset_relevant\fP is zero it indicates the \f(CWdw_offset\fP
field is not meaningful.
The value of register M is
the value currently in register \f(CWdw_regnum\fP (the
value DW_FRAME_CFA_COL3 must not appear, only real registers).
This is the register(R) rule of the DWARF3 spec.
.in -4

.P
If \f(CWdw_value_type\fP is DW_EXPR_OFFSET (1) then
this is the the val_offset(N) rule of the DWARF3 spec applies.
The calculation is identical to that of DW_EXPR_OFFSET (0)
but the value is interpreted as the value of register M
(rather than the address where register M's value is stored).
.P
If \f(CWdw_value_type\fP is DW_EXPR_EXPRESSION (2) then
this is the the expression(E) rule of the DWARF3 document.
.P
.in +4
\f(CWdw_offset_or_block_len\fP is the length in bytes of
the in-memory block  pointed at by \f(CWdw_block_ptr\fP.
\f(CWdw_block_ptr\fP is a DWARF expression.
Evaluate that expression and the result is the address
where the previous value of register M is found.
.in -4
.P
If \f(CWdw_value_type\fP is DW_EXPR_VAL_EXPRESSION (3) then
this is the the val_expression(E) rule of the DWARF3 spec.
.P
.in +4
\f(CWdw_offset_or_block_len\fP is the length in bytes of
the in-memory block  pointed at by \f(CWdw_block_ptr\fP.
\f(CWdw_block_ptr\fP is a DWARF expression.
Evaluate that expression and the result is the
previous value of register M.
.in -4
.P
The rule \f(CWrt3_cfa_rule\fP is the current value of
the CFA. It is interpreted exactly like
any register M rule (as described just above) except that
\f(CWdw_regnum\fP cannot be CW_FRAME_CFA_REG3 or
DW_FRAME_UNDEFINED_VAL or DW_FRAME_SAME_VAL but must
be a real register number.
.in -4



.H 3 "Macro Details Record"
The \f(CWDwarf_Macro_Details\fP type gives information about
a single entry in the .debug.macinfo section (DWARF2,
DWARF3, and DWARF4).
It is not useful for DWARF 5 .debug_macro section data.
.DS
\f(CWstruct Dwarf_Macro_Details_s {
  Dwarf_Off    dmd_offset;
  Dwarf_Small  dmd_type;
  Dwarf_Signed dmd_lineno;
  Dwarf_Signed dmd_fileindex;
  char *       dmd_macro;
};
typedef struct Dwarf_Macro_Details_s Dwarf_Macro_Details;
.DE
.P
\f(CWdmd_offset\fP is the byte offset, within the .debug_macinfo
section, of this macro information.
.P
\f(CWdmd_type\fP is the type code of this macro info entry
(or 0, the type code indicating that this is the end of
macro information entries for a compilation unit.
See \f(CWDW_MACINFO_define\fP, etc in the DWARF document.
.P
\f(CWdmd_lineno\fP is the line number where this entry was found,
or 0 if there is no applicable line number.
.P
\f(CWdmd_fileindex\fP is the file index of the file involved.
This is only guaranteed meaningful on a \f(CWDW_MACINFO_start_file\fP
\f(CWdmd_type\fP.  Set to -1 if unknown (see the functional
interface for more details).
.P
\f(CWdmd_macro\fP is the applicable string.
For a \f(CWDW_MACINFO_define\fP
this is the macro name and value.
For a
\f(CWDW_MACINFO_undef\fP, or
this is the macro name.
For a
\f(CWDW_MACINFO_vendor_ext\fP
this is the vendor-defined string value.
For other \f(CWdmd_type\fPs this is 0.

.H 2 "Opaque Types"
The opaque types declared in \fIlibdwarf.h\fP are used as descriptors
for queries against DWARF information stored in various debugging
sections.  Each time an instance of an opaque type is returned as a
result of a \fIlibdwarf\fP operation (\f(CWDwarf_Debug\fP excepted),
it should be freed, using \f(CWdwarf_dealloc()\fP when it is no longer
of use (read the following documentation for details, as in at least
one case there is a special routine provided for deallocation
and \f(CWdwarf_dealloc()\fP is not directly called:
see \f(CWdwarf_srclines()\fP).
Some functions return a number of instances of an opaque type
in a block, by means of a pointer to the block
and a count of the number
of opaque descriptors in the block:
see the function description for deallocation
rules for such functions.
The list of opaque types defined in
\fIlibdwarf.h\fP that are pertinent to the
Consumer Library, and their
intended use is described below.

.DS
\f(CWtypedef struct Dwarf_Debug_s* Dwarf_Debug;\fP
.DE
An instance of the
\f(CWDwarf_Debug\fP type is created as a result of a
successful call to
\f(CWdwarf_init_b()\fP, or
\f(CWdwarf_elf_init_b()\fP,
and is used as a descriptor for subsequent access to most
\f(CWlibdwarf\fP
functions on that object.
The storage pointed to by this descriptor
should be not be freed, using the
\f(CWdwarf_dealloc()\fP function.
Instead free it with
\f(CWdwarf_finish()\fP.
.P

.DS
\f(CWtypedef struct Dwarf_Die_s* Dwarf_Die;\fP
.DE
An instance of a \f(CWDwarf_Die\fP type is returned from a successful
call to the \f(CWdwarf_siblingof()\fP, \f(CWdwarf_child\fP, or
\f(CWdwarf_offdie_b()\fP function,
and is used as a descriptor for queries
about information related to that DIE.  The storage pointed to by this
descriptor should be freed, using
\f(CWdwarf_dealloc()\fP with the allocation
type \f(CWDW_DLA_DIE\fP when no longer needed.

.DS
\f(CWtypedef struct Dwarf_Line_s* Dwarf_Line;\fP
.DE
Instances of
\f(CWDwarf_Line\fP type are returned from a successful call
to the
\f(CWdwarf_srclines()\fP function, and are used as descriptors for
queries about source lines.
The storage pointed to by these descriptors
should be individually freed, using
\f(CWdwarf_dealloc()\fP with the
allocation type
\f(CWDW_DLA_LINE\fP when no longer needed.

.DS
\f(CWtypedef struct Dwarf_Global_s* Dwarf_Global;\fP
.DE
Instances of \f(CWDwarf_Global\fP type are returned from a successful
call to the \f(CWdwarf_get_globals()\fP function, and are used as
descriptors for queries about global names (pubnames).

.DS
\f(CWtypedef struct Dwarf_Weak_s* Dwarf_Weak;\fP
.DE
Instances of
\f(CWDwarf_Weak\fP type are returned from a successful call
to the
SGI-specific \f(CWdwarf_get_weaks()\fP
function, and are used as descriptors for
queries about weak names.  The storage pointed to by these descriptors
should be individually freed, using \f(CWdwarf_dealloc()\fP with the
allocation type
\f(CWDW_DLA_WEAK_CONTEXT\fP
(or
\f(CWDW_DLA_WEAK\fP, an older name, supported for compatibility)
when no longer needed.

.DS
\f(CWtypedef struct Dwarf_Func_s* Dwarf_Func;\fP
.DE
Instances of \f(CWDwarf_Func\fP type are returned from a successful
call to the
SGI-specific \f(CWdwarf_get_funcs()\fP
function, and are used as
descriptors for queries about static function names.

.DS
\f(CWtypedef struct Dwarf_Type_s* Dwarf_Type;\fP
.DE
Instances of
\f(CWDwarf_Type\fP type are returned from a successful call
to the
SGI-specific \f(CWdwarf_get_types()\fP
function, and are used as descriptors for
queries about user defined types.

.DS
\f(CWtypedef struct Dwarf_Var_s* Dwarf_Var;\fP
.DE
Instances of
\f(CWDwarf_Var\fP type are returned from a successful call
to the SGI-specific \f(CWdwarf_get_vars()\fP
function, and are used as descriptors for
queries about static variables.

.DS
\f(CWtypedef struct Dwarf_Error_s* Dwarf_Error;\fP
.DE
This descriptor points to a structure that provides
detailed information about errors detected by
\f(CWlibdwarf\fP.
Users typically provide a location
for \f(CWlibdwarf\fP to store this descriptor for
the user to obtain more information about the error.
The storage pointed to by this descriptor should
be freed, using
\f(CWdwarf_dealloc()\fP with the
allocation type
\f(CWDW_DLA_ERROR\fP when no longer
needed.

.DS
\f(CWtypedef struct Dwarf_Attribute_s* Dwarf_Attribute;\fP
.DE
Instances of
\f(CWDwarf_Attribute\fP type are returned from a successful
call to the
\f(CWdwarf_attrlist()\fP, or \f(CWdwarf_attr()\fP functions,
and are used as descriptors for queries about attribute values.
The storage
pointed to by this descriptor should be individually freed, using
\f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_ATTR\fP when
no longer needed.

.DS
\f(CWtypedef struct Dwarf_Abbrev_s* Dwarf_Abbrev;\fP
.DE
An instance of a \f(CWDwarf_Abbrev\fP
type is returned from a successful call to
\f(CWdwarf_get_abbrev()\fP, and is used as a
descriptor for queries about abbreviations in
the .debug_abbrev section.
The storage pointed
to by this descriptor should be freed, using
\f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_ABBREV\fP when no longer needed.

.DS
\f(CWtypedef struct Dwarf_Fde_s* Dwarf_Fde;\fP
.DE
Instances of \f(CWDwarf_Fde\fP type
are returned from a successful call
to the \f(CWdwarf_get_fde_list()\fP,
\f(CWdwarf_get_fde_for_die()\fP, or
\f(CWdwarf_get_fde_at_pc()\fP functions, and are used
as descriptors for queries about frames descriptors.

.DS
\f(CWtypedef struct Dwarf_Cie_s* Dwarf_Cie;\fP
.DE
Instances of \f(CWDwarf_Cie\fP type are returned from
a successful call to the \f(CWdwarf_get_fde_list()\fP
function, and are used as descriptors for queries
about information that is common to several frames.

.DS
\f(CWtypedef struct Dwarf_Arange_s* Dwarf_Arange;\fP
.DE
Instances of \f(CWDwarf_Arange\fP type
are returned from successful calls
to the \f(CWdwarf_get_aranges()\fP, or
\f(CWdwarf_get_arange()\fP functions, and are used
as descriptors for queries about address ranges.
The storage pointed to by this descriptor should be
individually freed, using \f(CWdwarf_dealloc()\fP
with the allocation type \f(CWDW_DLA_ARANGE\fP when
no longer needed.

.DS
\f(CWtypedef struct Dwarf_Gdbindex_s* Dwarf_Gdbindex;\fP
.DE
Instances of \f(CWDwarf_Gdbindex\fP type
are returned from successful calls to the
\f(CWdwarf_gdbindex_header()\fP function and are
used to extract information from a .gdb_index
section.  This section is a gcc/gdb extension
and is designed to allow a debugger fast access
to data in .debug_info.  The storage pointed to
by this descriptor should be freed using a call
to \f(CWdwarf_gdbindex_free()\fP with a valid
\f(CWDwarf_Gdbindex\fP pointer as the argument.

.DS
\f(CWtypedef struct Dwarf_Xu_Index_Header_s* Dwarf_Xu_Index_header;\fP
.DE
Instances of \f(CWDwarf_Xu_Index_Header_s\fP
type are returned from successful calls to the
\f(CWdwarf_get_xu_index_header()\fP function and are
used to extract information from a .debug_cu_index
or .debug_tu_index section.
These sections are used
to make possible access to .dwo sections gathered
into a .dwp object as part of the DebugFission
project allowing separation of an executable from
most of its DWARF debugging information.
As of May
2015 these sections are accepted into DWARF5 but
the standard has not been released.
The storage
pointed to by this descriptor should be freed using
a call to
\f(CWdwarf_xh_header_free()\fP with a valid
\f(CWDwarf_XuIndexHeader\fP pointer as the argument.

.DS
\f(CWtypedef struct Dwarf_Line_Context_s * Dwarf_Line_Context;\fP
.DE
\f(CWdwarf_srclines_b()\fP returns a
Dwarf_Line_Context through an argument
and the new structure pointer lets us access line header information
conveniently.
.DS
\f(CWtypedef struct Dwarf_Loc_c_s * Dwarf_Loc_c;\fP
\f(CWtypedef struct Dwarf_Locdesc_c_s * Dwarf_Locdesc_c;\fP
\f(CWtypedef struct Dwarf_Loc_Head_c_s * Dwarf_Loc_Head_c;\fP
.DE
\f(CWDwarf_Loc*\fP are involved in the DWARF5
interfaces to location lists.
The new interfaces are all functional and contents
of the above types
are not exposed.

.DS
\f(CWtypedef struct Dwarf_Macro_Context_s * Dwarf_Macro_Context;\fP
.DE
\f(CWdwarf_get_macro_context()\fP
and
\f(CWdwarf_get_macro_context_by_offset()\fP
return a Dwarf_Line_Context through an argument
and the new structure pointer lets us access
macro data from the .debug_macro section.

.DS
\f(CWtypedef struct Dwarf_Dsc_Head_s * Dwarf_Dsc_Head;\fP
.DE
\f(CWdwarf_discr_list()\fP
returns a Dwarf_Dsc_Head through an argument
and the new structure pointer lets us access
macro data from a
\f(CWDW_AT_discr_list\fP
attribute.


.H 1 "UTF-8 strings"
\fIlibdwarf\fP
is defined, at various points, to return
string pointers or to copy strings into
string areas you define.
DWARF allows the use of
\f(CWDW_AT_use_UTF8\fP
(DWARF3 and later)
\f(CWDW_ATE_UTF\fP
(DWARF4 and later)
to specify that the strings returned are actually
in UTF-8 format.
What this means is that if UTF-8 is specfied on
a particular object it is up to callers that wish
to print all the characters properly to use language-appropriate
functions to print Unicode strings appropriately.
All ASCII characters in the strings will print properly
whether printed as wide characters or not.
The methods to convert UTF-8 strings so they will print
correctly for all  such strings
is beyond the scope of this document.
.P
If UTF-8 is not specified then one is probably safe
in assuming the strings are iso_8859-15 and normal
C printf() will work fine..
.P
In either case  one should be wary of corrupted
(accidentally or intentionally)
strings
with ASCII control characters in the text.
Such can cause bad effects if simply printed to a device
(such as a terminal).

.H 1 "Error Handling"
The method for detection and disposition of error conditions that arise
during access of debugging information via \fIlibdwarf\fP is consistent
across all
\fIlibdwarf\fP
functions that are capable of producing an
error.  This section describes the method used by \fIlibdwarf\fP in
notifying client programs of error conditions.
.P
Most functions within
\fIlibdwarf\fP accept as an argument a pointer to
a
\f(CWDwarf_Error\fP descriptor where a
\f(CWDwarf_Error\fP descriptor
is stored if an error is detected by the function.
Routines in the client
program that provide this argument can query the
\f(CWDwarf_Error\fP
descriptor to determine the nature of the error
and perform appropriate
processing.
The intent is that clients do the appropriate processing
immediately on encountering an error and then the client
calls \f(CWdwarf_dealloc\fP to free the descriptor.
.P
In the rare case where the malloc arena is exhausted when
trying to create a Dwarf_Error descriptor a
pointer to a statically allocated
descriptor will be returned.
This static descriptor is new in December 2014.
A call to
\f(CWdwarf_dealloc()\fP
to free the statically
allocated descriptor is harmless (it sets the error value
in the descriptor to  DW_DLE_FAILSAFE_ERRVAL).
The possible conflation of errors when the arena
is exhausted
(and a dwarf_error descriptor is saved past
the next reader call in any thread)
is considered better than having
\fIlibdwarf\fP
call
\f(CWabort()\fP (as earlier
\fIlibdwarf\fP
did).
.P
A client program can also specify a function to be
invoked upon detection of an error at the time the
library is initialized (see
\f(CWdwarf_init_b()\fP).
When a \fIlibdwarf\fP routine detects an error,
this function is called with two arguments: a code
indicating the nature of the error and a pointer
provided by the client at initialization (again
see
\f(CWdwarf_init_b()\fP).
This pointer argument
can be used to relay information between the error
handler and other routines of the client program.
A client program can specify or change both the
error handling function and the pointer argument
after initialization using
\f(CWdwarf_seterrhand()\fP
and
\f(CWdwarf_seterrarg()\fP.
.P
In the case where
\fIlibdwarf\fP
functions are not provided a pointer
to a \f(CWDwarf_Error\fP descriptor,
and no error handling function was
provided at initialization,
\fIlibdwarf\fP functions
print a short message to stdout
and terminate exectution with
\f(CWabort()\fP.
.P
Before March 2016
\fIlibdwarf\fP
gave up when there was no error handling
by
emitting a short message on
\f(CWstderr\fP
calling \f(CWabort(3C)\fP.

.P
The following lists the processing steps taken upon detection of an
error:
.AL 1
.LI
Check the \f(CWerror\fP argument; if not a
\fINULL\fP pointer, allocate and initialize a
\f(CWDwarf_Error\fP descriptor with information
describing the error, place this descriptor in the
area pointed to by \f(CWerror\fP, and return a value
indicating an error condition.
.LI
If an \f(CWerrhand\fP argument was provided to
\f(CWdwarf_init_b()\fP at initialization, call
\f(CWerrhand()\fP passing it the error descriptor and
the value of the \f(CWerrarg\fP argument provided
to \f(CWdwarf_init_b()\fP.  If the error handling
function returns, return \f(CWDW_DLV_ERROR\fP
indicating an error condition.

.LI
If neither  the \f(CWerror\fP argument nor
an \f(CWerrhand\fP argument was provided
Terminate program execution by calling
\f(CWabort(3C)\fP.
.LE
.SP

In all cases, it is clear from the value returned
from a function that an error occurred in executing
the function, since DW_DLV_ERROR is returned.
.P
As can be seen from the above steps, the
client program can provide an error handler at
initialization, and still provide an \f(CWerror\fP
argument to \fIlibdwarf\fP functions when it is not
desired to have the error handler invoked.

.P
If a \f(CWlibdwarf\fP function is called with
invalid arguments, the behavior is undefined.
In particular, supplying a \f(CWNULL\fP pointer to a
\f(CWlibdwarf\fP function (except where explicitly
permitted), or pointers to invalid addresses or
uninitialized data causes undefined behavior;
the return value in such cases is undefined, and
the function may fail to invoke the caller supplied
error handler or to return a meaningful error number.
Implementations also may abort execution for such
cases.

.P
Some errors are so inconsequential that it does not
warrant rejecting an object or returning an error.
An example would be a frame length not being a
multiple of an address-size (right now this is
the only such inconsequential error).  To make it
possible for a client  to report such errors the
function \f(CWdwarf_get_harmless_error_list\fP
returns strings with error text in them.
This function may be ignored if client code does
not want to bother with such error reporting. See
\f(CWDW_DLE_DEBUG_FRAME_LENGTH_NOT_MULTIPLE\fP in
the libdwarf source code.

.P
.H 2 "Returned values in the functional interface"
Values returned by \f(CWlibdwarf\fP functions to indicate
success and errors
.nr aX \n(Fg+1
are enumerated in Figure \n(aX.
The \f(CWDW_DLV_NO_ENTRY\fP
case is useful for functions
need to indicate that while there was no data to return
there was no error either.
For example, \f(CWdwarf_siblingof()\fP
may return \f(CWDW_DLV_NO_ENTRY\fP to indicate that that there was
no sibling to return.
.DS
.TS
center box, tab(:);
lfB cfB lfB
l c l.
SYMBOLIC NAME:VALUE:MEANING
_
DW_DLV_ERROR:1:Error
DW_DLV_OK:0:Successful call
DW_DLV_NO_ENTRY:-1:No applicable value
.TE
.FG "Error Indications"
.DE
.P
Each function in the interface that returns a value returns one
of the integers in the above figure.
.P
If \f(CWDW_DLV_ERROR\fP is returned and a pointer
to a \f(CWDwarf_Error\fP pointer is passed to the
function, then a Dwarf_Error handle is returned
through the pointer. No other pointer value
in the interface returns a value.  After the
\f(CWDwarf_Error\fP is no longer of interest, a
\f(CWdwarf_dealloc(dbg,dw_err,DW_DLA_ERROR)\fP on
the error pointer is appropriate to free any space
used by the error information.
.P
If \f(CWDW_DLV_NO_ENTRY\fP is returned no pointer value in the
interface returns a value.
.P
If \f(CWDW_DLV_OK\fP is returned, the
\f(CWDwarf_Error\fP pointer, if supplied, is not
touched, but any other values to be returned through
pointers are returned.  In this case calls (depending
on the exact function returning the error) to
\f(CWdwarf_dealloc()\fP may be appropriate once the
particular pointer returned is no longer of interest.
.P
Pointers passed to allow values to be returned through them are
uniformly the last pointers
in each argument list.
.P
All the interface functions are defined from the
point of view of the writer-of-the-library (as is
traditional for UN*X library documentation), not
from the point of view of the user of the library.
The caller might code:
.P
.DS
\f(CWDwarf_Line line;
Dwarf_Signed ret_loff;
Dwarf_Error  err;
int retval = dwarf_lineoff(line,&ret_loff,&err);\fP
.DE
for the function defined as
.P
.DS
\f(CWint dwarf_lineoff(Dwarf_Line line,Dwarf_Signed *return_lineoff,
  Dwarf_Error* err);\fP
.DE
and this document refers to the function as
returning the value through *err or *return_lineoff or
uses the phrase "returns in
the location pointed to by err".
Sometimes other similar phrases are used.

.H 1 "Memory Management"
Several of the functions that comprise \fIlibdwarf\fP
return pointers (opaque descriptors) to structures
that have been dynamically allocated by the
library.  To manage dynamic memory the function
\f(CWdwarf_dealloc()\fP is provided to free storage
allocated as a result of a call to a \fIlibdwarf\fP
function.  Some additional functions (described
later) are provided to free storage in particular
circumstances.  This section describes the general
strategy that should be taken by a client program
in managing dynamic storage.

.H 2 "Read-only Properties"
All pointers (opaque descriptors) returned by or as a result of a
\fIlibdwarf Consumer Library\fP
call should be assumed to point to read-only memory.
The results are undefined for \fIlibdwarf\fP  clients that attempt
to write to a region pointed to by a value returned by a
\fIlibdwarf Consumer Library\fP
call.

.H 2 "Storage Deallocation"
See the section "Returned values in the functional interface",
above, for the general rules where
calls to \f(CWdwarf_dealloc()\fP
is appropriate.
.P
In some cases the pointers returned by a
\fIlibdwarf\fP call are pointers to data which is
not freeable.
The library knows from the allocation
type provided to it whether the space is freeable
or not and will not free inappropriately when
\f(CWdwarf_dealloc()\fP is called.
So it is vital
that \f(CWdwarf_dealloc()\fP be called with the
proper allocation type.
.P
For most storage allocated by \fIlibdwarf\fP, the
client can free the storage for reuse by calling
\f(CWdwarf_dealloc()\fP, providing it with the
\f(CWDwarf_Debug\fP descriptor specifying the
object for which the storage was allocated,
a pointer to the area to be free-ed, and an
identifier that specifies what the pointer points
to (the allocation type).
For example, to free
a \f(CWDwarf_Die die\fP belonging the the object
represented by \f(CWDwarf_Debug dbg\fP, allocated
by a call to \f(CWdwarf_siblingof()\fP, the call to
\f(CWdwarf_dealloc()\fP would be:
.DS
    \f(CWdwarf_dealloc(dbg, die, DW_DLA_DIE);\fP
.DE

To free storage allocated in the form of a list of pointers (opaque
descriptors), each member of the list should be deallocated, followed
by deallocation of the actual list itself.  The following code fragment
uses an invocation of
\f(CWdwarf_attrlist()\fP as an example to illustrate
a technique that can be used to free storage from any \fIlibdwarf\fP
routine that returns a list:
.DS
.FG "Example1 dwarf_attrlist()"
\f(CW
void example1(Dwarf_Die somedie)
{
    Dwarf_Debug dbg = 0;
    Dwarf_Signed atcount;
    Dwarf_Attribute *atlist;
    Dwarf_Error error = 0;
    Dwarf_Signed i = 0;
    int errv;

    errv = dwarf_attrlist(somedie, &atlist,&atcount, &error);
    if (errv == DW_DLV_OK) {
        for (i = 0; i < atcount; ++i) {
            /* use atlist[i] */
            dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
        }
        dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    }
}
\fP
.DE

The \f(CWDwarf_Debug\fP returned from \f(CWdwarf_init_b()\fP
or \f(CWdwarf_elf_init_b()\fP
cannot be freed using \f(CWdwarf_dealloc()\fP.
The function
\f(CWdwarf_finish()\fP will deallocate all dynamic storage
associated with an instance of a
\f(CWDwarf_Debug\fP type.  In particular,
it will deallocate all dynamically allocated space associated with the
\f(CWDwarf_Debug\fP descriptor, and finally make the descriptor invalid.

An \f(CWDwarf_Error\fP returned from \f(CWdwarf_init_b()\fP
or \f(CWdwarf_elf_init_b()\fP
in case of a failure cannot be freed
using \f(CWdwarf_dealloc()\fP.
The only way to free the \f(CWDwarf_Error\fP from either of those
calls is to use \f2free(3)\fP directly.
Every \f(CWDwarf_Error\fP must be freed
by \f(CWdwarf_dealloc()\fP except those
returned by \f(CWdwarf_init_b()\fP
or \f(CWdwarf_elf_init_b()\fP.

.P
The codes that identify the storage pointed to in calls to
.nr aX \n(Fg+1
\f(CWdwarf_dealloc()\fP are described in figure \n(aX.
.DS
.TS
center box, tab(:);
lfB lfB
l l.
IDENTIFIER:USED TO FREE
_
DW_DLA_STRING           :     char*
DW_DLA_LOC              :     Dwarf_Loc
DW_DLA_LOCDESC          :     Dwarf_Locdesc
DW_DLA_ELLIST           :     Dwarf_Ellist (not used)
DW_DLA_BOUNDS           :     Dwarf_Bounds (not used)
DW_DLA_BLOCK            :     Dwarf_Block
DW_DLA_DEBUG            :     Dwarf_Debug (do not use)
DW_DLA_DIE              :     Dwarf_Die
DW_DLA_LINE             :     Dwarf_Line
DW_DLA_ATTR             :     Dwarf_Attribute
DW_DLA_TYPE             :     Dwarf_Type  (not used)
DW_DLA_SUBSCR           :     Dwarf_Subscr (not used)
DW_DLA_GLOBAL_CONTEXT   :     Dwarf_Global
DW_DLA_ERROR            :     Dwarf_Error
DW_DLA_LIST             :     a list of opaque descriptors
DW_DLA_LINEBUF          :     Dwarf_Line* (not used)
DW_DLA_ARANGE           :     Dwarf_Arange
DW_DLA_ABBREV           :     Dwarf_Abbrev
DW_DLA_FRAME_OP         :     Dwarf_Frame_Op
DW_DLA_CIE              :     Dwarf_Cie
DW_DLA_FDE              :     Dwarf_Fde
DW_DLA_LOC_BLOCK        :     Dwarf_Loc Block
DW_DLA_FRAME_BLOCK      :     Dwarf_Frame Block (not used)
DW_DLA_FUNC_CONTEXT     :     Dwarf_Func
DW_DLA_TYPENAME_CONTEXT :     Dwarf_Type
DW_DLA_VAR_CONTEXT      :     Dwarf_Var
DW_DLA_WEAK_CONTEXT     :     Dwarf_Weak
DW_DLA_PUBTYPES_CONTEXT :     Dwarf_Type
.TE
.FG "Allocation/Deallocation Identifiers"
.DE

.P
.H 1 "Functional Interface"
This section describes the functions available in the \fIlibdwarf\fP
library.  Each function description includes its definition, followed
by one or more paragraph describing the function's operation.

.P
The following sections describe these functions.

.H 2 "Initialization Operations"
These functions are concerned with preparing an object file for subsequent
access by the functions in \fIlibdwarf\fP and with releasing allocated
resources when access is complete.

.H 3 "dwarf_init_path()"
.DS
\f(CWint dwarf_init_path(
    const char *      path,
    char *            true_path_out_buffer,
    unsigned          true_path_bufferlen,
    Dwarf_Unsigned    access,
    unsigned          groupnumber,
    Dwarf_Handler     errhand,
    Dwarf_Ptr         errarg,
    Dwarf_Debug*      dbg,
    const char *      reserved1,
    Dwarf_Unsigned  * reserved2,
    Dwarf_Unsigned  * reserved3,
    Dwarf_Error*    *  error);
.DE
On success the function returns
\f(CWDW_DLV_OK\fP,
and returns a pointer to an initialized
Dwarf_Debug through the dbg argument.
All this work identically across all supported
object file types.
.P
If
\f(CWDW_DLV_NO_ENTRY\fP
is returned there is no such
file and nothing else is done or returned.
.P
If
\f(CWDW_DLV_ERROR\fP is returned
a Dwarf_Error is returned through the error pointer.
and nothing else is done or returned.
.P
Now we turn to the arguments.
.P
Pass in the name of the object file via
the
\f(CWpath\fP
argument.
.P
To
\f(CWtrue_path_out_buffer\fP
pass in a pointer big enough to hold the passed-in
path if that were doubled plus adding 100 characters.
Then pass that length in the
\f(CWtrue_path_bufferlen\fP argument.
The path will be copied to true_path_out_buffer.
In the case of certain MacOS dSYM
object files the final true path
of the dSYM file (with MacOS conventional
directories added) is copied into
\f(CWtrue_path_out_buffer\fP.

To entirely skip the MacOS special treatment
pass 0 as arguments to
\f(CWtrue_path_out_buffer\fP
and
\f(CWtrue_path_bufferlen\fP.
Or use
\f(CWdwarf_init_b()\fP
instead  of
\f(CWdwarf_init_path()\fP
.P
Pass in the usual DW_DLC_READ
(which only ever applied to libelf, really)
to
\f(CWaccess\fP.
Currently no other value is allowed.
Non-elf objects currently ignore this field.
.P
The
\f(CWgroupnumber\fP argument indicates which group is to
be accessed Group one is normal dwarf sections such as
\f(CW.debug_info\fP.
Group two is DWARF5 dwo split-dwarf dwarf
sections such as .debug_info.dwo.

Groups three and higher are for COMDAT groups.  If an object
file has only sections from one of the groups then passing
zero will access that group.  Otherwise passing zero will
access only group one.

See
\f(CWdwarf_sec_group_sizes()\fP
and
\f(CWdwarf_sec_group_map()\fP
for more group information.

Typically
pass in DW_GROUPNUMBER_ANY
to
\f(CWgroupnumber\fP.
Non-elf objects do not use
this field.
.P
The \f(CWerrhand\fP
argument is a pointer to a function that will be
invoked whenever an error is detected as a result of
a \fIlibdwarf\fP operation.  The \f(CWerrarg\fP
argument is passed as an argument to the
\f(CWerrhand\fP function.
.P
\f(CWdbg\fP.
.P
\f(CWreserved1\fP,
\f(CWreserved2\fP,
and
\f(CWreserved3\fP
are currently unused, pass 0
in to all three.
.P
Pass in 0 (null) or a pointer
to a Dwarf_Error to the
\f(CWerror\fP argument
if you wish libdwarf to return an
error code.

.H 3 "dwarf_init_b()"
.DS
\f(CWint dwarf_init_b(
int fd,
    Dwarf_Unsigned access,
    unsigned group_number,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug * dbg,
    Dwarf_Error *error)\fP
.DE
When it returns
\f(CWDW_DLV_OK\fP,
the function \f(CWdwarf_init_b()\fP returns through
\f(CWdbg\fP a \f(CWDwarf_Debug\fP descriptor
that represents a handle for accessing
debugging records associated with
the open file descriptor \f(CWfd\fP.
\f(CWDW_DLV_NO_ENTRY\fP is returned if the object
does not contain DWARF debugging information.
\f(CWDW_DLV_ERROR\fP is returned if
an error occurred.
.P
The
\f(CWaccess\fP argument indicates what access is
allowed for the section.  The \f(CWDW_DLC_READ\fP
parameter is valid for read access (only read access
is defined or discussed in this document).
.P
The
\f(CWgroupnumber\fP argument indicates which group is to
be accessed Group one is normal dwarf sections such as
\f(CW.debug_info\fP.  Group two is DWARF5 dwo split-dwarf dwarf
sections such as .debug_info.dwo.

Groups three and higher are for COMDAT groups.  If an object
file has only sections from one of the groups then passing
zero will access that group.  Otherwise passing zero will
access only group one.

See
\f(CWdwarf_sec_group_sizes()\fP
and
\f(CWdwarf_sec_group_map()\fP
for more group information.

.P
The \f(CWerrhand\fP
argument is a pointer to a function that will be
invoked whenever an error is detected as a result of
a \fIlibdwarf\fP operation.  The \f(CWerrarg\fP
argument is passed as an argument to the
\f(CWerrhand\fP function.
.P
The file descriptor associated with the \f(CWfd\fP
argument must refer to an ordinary file (i.e. not a
pipe, socket, device, /proc entry, etc.), be opened
with the at least as much permission as specified by
the \f(CWaccess\fP argument, and cannot be closed or
used as an argument to any system calls by the client
until after \f(CWdwarf_finish()\fP is called.  The
seek position of the file associated with \f(CWfd\fP
is undefined upon return of \f(CWdwarf_init()\fP.
.P
Historical Note:
With SGI IRIX, by default it was allowed that
the app \f(CWclose()\fP \f(CWfd\fP immediately
after calling \f(CWdwarf_init()\fP, but that
is not  a portable approach (that it worked was an
accidental side effect of the fact that SGI IRIX used
\f(CWELF_C_READ_MMAP\fP in its hidden internal call
to \f(CWelf_begin()\fP).
The portable approach is to
consider that \f(CWfd\fP must be left open till after
the corresponding dwarf_finish() call has returned.
.P
Since \f(CWdwarf_init()\fP uses the same error
handling processing as other \fIlibdwarf\fP functions
(see \fIError Handling\fP above), client programs
will generally supply an \f(CWerror\fP parameter
to bypass the default actions during initialization
unless the default actions are appropriate.

.H 3 "dwarf_init()"

.DS
\f(CWint dwarf_init(
    int fd,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug * dbg,
    Dwarf_Error *error)\fP
.DE
This identical to dwarf_init()
\f(CWdwarf_init_b()\fP
except that it is missing the groupnumber
argument so access to an object file containing
both dwo and non-dwo DWARF5 object sections will
access only group one (and will ignore the dwo sections).

.H 3 "Dwarf_Handler function"
This is an example of a valid error handler function.
A pointer to this  (or another like it)
may be passed to
\f(CWdwarf_elf_init_b()\fP
or
\f(CWdwarf_init_b()\fP.
.DS
\f(CWstatic void
simple_error_handler(Dwarf_Error error, Dwarf_Ptr errarg)
{
    printf("libdwarf error: %d  %s\n",
        dwarf_errno(error), dwarf_errmsg(error));
    exit(1);
}\fP
.DE
.P
This will only be called if an error is detected inside libdwarf
and the
Dwarf_Error
argument passed to libdwarf is NULL.
A Dwarf_Error will be created with the error number
assigned by the library and passed to the error handler.
.P
The second argument is a copy of the value passed in to
\f(CWdwarf_elf_init_b()\fP
or
\f(CWdwarf_init()\fP
as the
\f(CWerrarg()\fP argument.
Typically the init function would be passed
a pointer to an application-created struct
containing the data the application needs to
do what it wants to do in the error handler.
.P
In a language with exceptions or exception-like features
an exception could be thrown here.
Or the application could simply give up and call
\f(CWexit()\fP
as in the sample given above.

.H 3 "dwarf_elf_init_b() [deprecated]"
.DS
\f(CWint dwarf_elf_init_b(
    Elf * elf_file_pointer,
    Dwarf_Unsigned access,
    unsigned groupnumber,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug * dbg,
    Dwarf_Error *error)\fP
.DE
We recommend you change to calling
\f(CWdwarf_init_b()\fP
or
\f(CWdwarf_init_path()\fP
instead.
The
\f(CW
dwarf_elf_init_b()
\fP
and
\f(CW
dwarf_elf_init_b()
\fP
interfaces give no benefit
over the other interfaces.
.P
The function \f(CWdwarf_elf_init_b()\fP is identical to \f(CWdwarf_init_b()\fP
except that an open \f(CWElf *\fP pointer is passed instead of a file
descriptor.
.P
In systems supporting \f(CWElf\fP object files this may be
more space or time-efficient than using \f(CWdwarf_init_b()\fP,
see that function for more detailed description of the
arguments here..
.P
The client is allowed to use the \f(CWElf *\fP pointer
for its own purposes without restriction during the time the
\f(CWDwarf_Debug\fP
is open, except that the client should not  \f(CWelf_end()\fP the
pointer till after  \f(CWdwarf_finish\fP is called.

.H 3 "dwarf_elf_init()"
.DS
\f(CWint dwarf_elf_init(
    Elf * elf_file_pointer,
    Dwarf_Unsigned access,
    Dwarf_Handler errhand,
    Dwarf_Ptr errarg,
    Dwarf_Debug * dbg,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_elf_init()\fP is identical to \f(CWdwarf_init()\fP
except that an open \f(CWElf *\fP pointer is passed instead of a file
descriptor.


.H 3 "dwarf_get_elf()"
.DS
\f(CWint dwarf_get_elf(
    Dwarf_Debug dbg,
    Elf **      elf,
    Dwarf_Error *error)\fP
.DE
When it returns \f(CWDW_DLV_OK\fP,
the function \f(CWdwarf_get_elf()\fP returns through the
pointer \f(CWelf\fP the \f(CWElf *\fP handle
used to access the object represented by the \f(CWDwarf_Debug\fP
descriptor \f(CWdbg\fP.  It returns \f(CWDW_DLV_ERROR\fP on error.
.P
Because \f(CWint dwarf_init()\fP opens an Elf descriptor
on its fd and \f(CWdwarf_finish()\fP does not close that
descriptor, an app should use \f(CWdwarf_get_elf\fP
and should call \f(CWelf_end\fP with the pointer returned
through the \f(CWElf**\fP handle created by \f(CWint dwarf_init()\fP.
.P
This function is not meaningful for a system that does not use the
Elf format for objects.

.H 3 "dwarf_set_tied_dbg()"
.DS
\f(CWint dwarf_set_tied_dbg(
    Dwarf_Debug dbg,
    Dwarf_Debug tieddbg,
    Dwarf_Error *error)\fP
.DE
The function
\f(CWdwarf_set_tied_dbg()\fP
enables cross-object
access of DWARF data.
If a DWARF5 Package object
has
\f(CWDW_FORM_addrx\fP
or
\f(CWDW_FORM_GNU_addr_index\fP
in an address attribute
one needs both the Package file
and the executable to extract the actual address with
\f(CWdwarf_formaddr()\fP.
So one does a normal
\f(CWdwarf_elf_init_b()\fP
or
\f(CWdwarf_init()_b\fP
on each object and then tie the two together with
a call  such as:
.in +2
.DS
.FG "Example2 dwarf_set_died_dbg()"
\f(CW
void example2(Dwarf_Debug dbg, Dwarf_Debug tieddbg)
{
    Dwarf_Error error = 0;
    int res = 0;

    /*  Do the dwarf_init_b() or dwarf_elf_init_b()
        calls to set
        dbg, tieddbg at this point. Then: */
    res = dwarf_set_tied_dbg(dbg,tieddbg,&error);
    if (res != DW_DLV_OK) {
        /* Something went wrong*/
    }
}
\fP
.DE
.in -2

When done with both dbg and tieddbg
do the normal finishing operations on both
in any order.

It is possible to undo the tieing operation with
.in +2
.FG "Example3 dwarf_set_tied_dbg() obsolete"
.DS
\f(CW
void example3(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    int res = 0;
    res = dwarf_set_tied_dbg(dbg,NULL,&error);
    if (res != DW_DLV_OK) {
        /* Something went wrong*/
    }
}
\fP
.DE
.in -2
.P
It is not necessary to undo the tieing operation
before finishing on the dbg and tieddbg.
.H 3 "dwarf_get_tied_dbg()"
.DS
\f(CWint dwarf_get_tied_dbg(
    Dwarf_Debug   /*dbg*/,
    Dwarf_Debug * /*tieddbg_out*/,
    Dwarf_Error * /*error*/)\fP
.DE
\f(CWdwarf_get_tied_dbg\fP returns
\f(CWDW_DLV_OK\fP and sets
\f(CWtieddbg_out\fP to the pointer to
the 'tied' Dwarf_Debug.
If there is no 'tied' object
\f(CWtieddbg_out\fP is set to NULL.
.P
On error it returns
\f(CWDW_DLV_ERROR\fP.
.P
It never returns
\f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_finish()"
.DS
\f(CWint dwarf_finish(
    Dwarf_Debug dbg,
    Dwarf_Error *error)\fP
.DE
The function
\f(CWdwarf_finish()\fP releases all \fILibdwarf\fP internal resources
associated with the descriptor \f(CWdbg\fP, and invalidates \f(CWdbg\fP.
It returns \f(CWDW_DLV_ERROR\fP if there is an error during the
finishing operation.  It returns \f(CWDW_DLV_OK\fP
for a successful operation.

Because \f(CWint dwarf_init()\fP opens an Elf descriptor
on its fd and \f(CWdwarf_finish()\fP does not close that
descriptor, an app should use \f(CWdwarf_get_elf\fP
and should call \f(CWelf_end\fP with the pointer returned
through the \f(CWElf**\fP handle created by \f(CWint dwarf_init()\fP.

.H 3 "dwarf_set_stringcheck()"
.DS
\f(CWint dwarf_set_stringcheck(
        int stringcheck)\fP
.DE
The function
\f(CWint dwarf_set_stringcheck()\fP sets a global flag
and returns the previous value of the global flag.

If the stringcheck global flag is zero (the default)
libdwarf does string length validity checks
(the checks do slow libdwarf down very slightly).
If the stringcheck global flag is non-zero
libdwarf does not do string length validity
checks.

The global flag is really just 8 bits long, upperbits are not noticed
or recorded.

.H 3 "dwarf_set_reloc_application()"
.DS
\f(CWint dwarf_set_reloc_application(
        int apply)\fP
.DE
The function
\f(CWint dwarf_set_reloc_application()\fP sets a global flag
and returns the previous value of the global flag.

If the reloc_application global flag is non-zero (the default)
then the applicable .rela section (if one exists) will be
processed and applied to any DWARF section when it is read in.
If the reloc_application global flag is zero no such
relocation-application is attempted.

Not all
machine types (elf header e_machine)
or all relocations are supported, but then very few
relocation types apply to DWARF debug sections.

The global flag is really just 8 bits long, upperbits are not noticed
or recorded.

It seems unlikely anyone will need to call this function.

.H 3 "dwarf_record_cmdline_options()"
.DS
\f(CWint dwarf_record_cmdline_options(
    Dwarf_Cmdline_Options options)\fP
.DE
The function
\f(CWint dwarf_record_cmdline_options()\fP
copies a Dwarf_Cmdline_Options structure from
consumer code to libdwarf.

The structure is defined in \f(CWlibdwarf.h\fP.

The initial version of this structure has a single field
\f(CWcheck_verbose_mode\fP which, if non-zero, tells
libdwarf to print some detailed messages to stdout in case
certain errors are detected.

The default for this value is FALSE (0) so the extra messages
are off by default.

.H 3 "dwarf_object_init_b()"
.DS
\f(CWint dwarf_object_init_b(
    Dwarf_Obj_Access_Interface* obj,
    Dwarf_Handler errhand,
    Dwarf_Ptr     errarg,
    unsigned      groupnumber,
    Dwarf_Debug*  dbg,
    Dwarf_Error*  error)\fP
.DE
The function
\f(CWint dwarf_object_init_b()\fP
enables access to non-Elf object files
by allowing the caller to
then provide
function pointers to code (user-written,
not part of libdwarf)  that will
look, to libdwarf, as if libdwarf was
reading Elf.
.P
See
\f(CWint dwarf_init_b()\fP
for additional information on the arguments
passed in (the
\f(CWobj\fP argument here
is a set of function pointers
and describing how to access non-Elf
files is beyond the scope of this document.
.P
As a hint, note that the source files with
dwarf_elf_init_file_ownership() (dwarf_original_elf_init.c)
and
dwarf_elf_object_access_init() (dwarf_elf_access.c)
are the only sources that would need replacement
for a different object format.
The replacement would need to emulate certain
conventions of Elf objects, (mainly that
section index
0 is an empty section) but the rest of
libdwarf uses what these two source files set up
without knowing how to operate on Elf.

.P
Writing the functions needed to support
non-Elf will require study of Elf
and of the object format involved.
The topic is beyond the scope of
this document.

.H 3 "dwarf_object_init()"
.DS
\f(CWint dwarf_object_init(
    Dwarf_Obj_Access_Interface* obj,
    Dwarf_Handler errhand,
    Dwarf_Ptr     errarg,
    Dwarf_Debug*  dbg,
    Dwarf_Error*  error)\fP
.DE
The function
\f(CWint dwarf_object_init()\fP
is the same as
\f(CWint dwarf_object_init_b()\fP
except
\f(CWint dwarf_object_init()\fP
is missing the groupnumber argument so
DWARF5 split dwarf objects cannot be
fully handled.

.H 3 "dwarf_get_real_section_name()"
.DS
\f(CWint dwarf_get_real_section_name( Dwarf_Debug dbg,
    const char  * std_section_name,
    const char ** actual_sec_name_out,
    Dwarf_Small * marked_compressed,
    Dwarf_Small * marked_zlib_compressed,
    Dwarf_Small * marked_shf_compressed,
    Dwarf_Unsigned * compressed_length,
    Dwarf_Unsigned * uncompressed_length,
    Dwarf_Error * error);
.DE
FIXME
Elf sections are sometimes compressed to reduce the disk
footprint of the sections.
It's sometimes interesting to library users
what the real name was in the object file and whether it
was compressed.  Libdwarf uncompresses such sections
automatically.
It's not usually necessary to know the true name or
anything about compression.
.P
\f(CW
\fP
The caller passes in a
\f(CWDwarf_Debug\fP
pointer
and a standard section name such as ".debug_info" .
On success the function returns (through the
other arguments) the true section name and a
flag which, if non-zero means the section was compressed
and a flag which, if non-zero means the section had
the Elf section flag SHF_COMPRESSED set.
The caller must ensure that the memory pointed to
by
\f(CWactual_sec_name_out\fP,
\f(CWmarked_zcompressed\fP, and
\f(CWmarked_zlib_compressed\fP,
\f(CWmarked_shf_compressed\fP,
\f(CWcompressed_length\fP,
\f(CWuncompressed_length\fP,
is zero at the point of call.
.P
The flag
\f(CW*marked_compressed\fP,
if non-zero,
means the section name started
with .zdebug (indicating compression
was done).
.P
The flag
\f(CWmarked_zlib_compressed\fP,
if non-zero means the
initial bytes of the section starte
with the ASCII characters ZLIB
and the section was compressed.
.P
The flag
\f(CWmarked_shf_compressed\fP
if non-zero means the Elf section
sh_flag SHF_COMPRESSED is set
and the section was compressed..
The flag value in an elf section
header is (1<<11) (0x800).
.P
The value
\f(CWcompressed_length\fP
is passed back through the pointer
if and only if the section is compressed
and the pointer is non-null.
.P
The value
\f(CWuncompressed_length\fP
is passed back through the pointer
if and only if the section is compressed
and the pointer is non-null.
.P
If the section name passed in is not used by libdwarf
for this object file
the function returns
\f(CWDW_DLV_NO_ENTRY\fP
.P
On error the function returns
\f(CWDW_DLV_ERROR\fP.
.P
The string pointed to by
\f(CW*actual_sec_name_out\fP
must not be free()d.

.H 2 "Object Type Detectors"
These are used by
\f(CWlibdwarf\fP
and may be of use generally.
They have no connection to
any Dwarf_Debug data as you see
from the arguments passed in.
.H 3 "dwarf_object_detector_path()"
.DS
\f(CWint dwarf_object_detector_path(const char  *path,
    char *outpath,
    unsigned long outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    Dwarf_Unsigned  *filesize,
    int * errcode);\fP
.DE
On success the function returns
\f(CWDW_DLV_OK\fP,
and returns various data through the
arguments (described just below).
This works identically across all supported
object file types.
.P
If
\f(CWDW_DLV_NO_ENTRY\fP
is returned there is no such
file and nothing else is done or returned.
.P
If
\f(CWDW_DLV_ERROR\fP is returned
a Dwarf_Error is returned through the error pointer.
and nothing else is done or returned.
.P
Now we turn to the arguments.
Pass in the name of the object file via
the
\f(CWpath\fP
argument.
.P
To
\f(CWoutpath\fP
pass in a pointer big enough to hold the passed-in
path if that were doubled plus adding 100 characters.
Then pass that length in the
\f(CWoutpath_len\fP argument.
The path will be copied to outpath.
In the case of certain MacOS dSYM
object files the final outpath
of the dSYM file (with MacOS conventional
directories added) is copied into
\f(CWoutpath\fP.

To entirely skip the MacOS special treatment
pass 0 as arguments to
\f(CWoutpath\fP
and
\f(CWoutpath_len\fP.
.P
The \f(CWftype\fP
pointer argument returns
\f(CWDW_FTYPE_ELF\fP,
\f(CWDW_FTYPE_MACH_O\fP
,
\f(CWDW_FTYPE_PE\fP
,
\f(CWDW_FTYPE_ARCHIVE\fP
or
\f(CWDW_FTYPE_UNKNOWN\fP
to the caller.
The
\f(CWDW_FTYPE_ARCHIVE\fP
value says nothing whatever
about the contents of the
archive.
.P
The \f(CWendian\fP
pointer argument returns
\f(CWDW_ENDIAN_BIG\fP,
\f(CWDW_ENDIAN_LITTLE\fP
,
\f(CWDW_ENDIAN_SAME\fP
,
\f(CWDW_ENDIAN_OPPOSITE\fP
or
\f(CWDW_ENDIAN_UNKNOWN\fP
to the caller.
.P
The \f(CWoffsetsize\fP
pointer argument returns
a size value from the object file.
If the object file uses 32-bit
offsets it returns 32, and if
64-bit offsets it returns 64.
Each object type uses such values
but the ways the value is used
varies.
.P
The \f(CWfilesize\fP
pointer argument returns
the size, in bytes, of the object file.
This is essentially useless for
\f(CWDW_FTYPE_ARCHIVE\fP
files, one thinks.
.P
The \f(CWerrcode\fP
pointer argument returns
(if and only if DW_DLV_ERROR is returned
by the function) an integer error code.
At this time there is no handy function
to turn that error code into a string.
In the libdwarf source you will find that code in
the DW_DLE_* error list.

.H 3 "dwarf_object_detector_fd()"
.DS
\f(CWint dwarf_object_detector_fd(int fd,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    Dwarf_Unsigned  *filesize,
    int * errcode);\fP
.DE
\f(CWdwarf_object_detector_fd()\fP
is the same as
\f(CWdwarf_object_detector_path()\fP
except that no path strings apply to
\f(CWdwarf_object_detector_fd()\fP.


.H 2 "Section Group Operations"
The section group data is essential information
when processing an object with COMDAT section group DWARF sections
or with both split-dwarf (.dwo sections)
and non-split dwarf sections.
.P
It relies on Elf section groups, whereas some compilers
rely instead on relocation information to identify
section groups.
These relocation-specified groupings are not understood
at this time.
.P
A standard DWARF2 or DWARF3 or DWARF4 object
(Old Standard Object, or OSO) will
not contain any of those new sections.
The DWARF4 standard, Appendix E.1 "Using Compilation Units"
offers an overview of COMDAT section groups.
\f(CWlibdwarf\fP
assigns  the group number one(1) to
OSO DWARF.
Any sections that are split dwarf (section name ending in .dwo
or one of the two special DWP index sections)
are assigned group number two(2)
by libdwarf.
COMDAT section groups are assigned groups numbers 3 and higher as needed.
.P
The COMDAT section group uses are not well defined, but
popular compilations systems are using such sections.
There is no meaningful documentation
that we can find (so far) on how the COMDAT
section groups are used,
so
\f(CWlibdwarf\fP
is based on observations
of what compilers generate.

.H 3 "dwarf_sec_group_sizes()"
.DS
\f(CW int dwarf_dwarf_sec_group_sizes(
    Dwarf_Debug dbg,
    Dwarf_Unsigned * section_count_out,
    Dwarf_Unsigned * group_count_out,
    Dwarf_Unsigned * selected_group_out,
    Dwarf_Unsigned * map_entry_count_out,
    Dwarf_Error    * error)
\fP
.DE
The function
\f(CWdwarf_sec_group_sizes()\fP
may be called on any open
\f(CWDwarf_Debug\fP.
It returns
\f(CWDW_DLV_OK\fP
on success and returns
values via the pointer arguments.
.P
Once the
\f(CWDwarf_Debug\fP
is open the group
information is set and it will not change
for the life of this
\f(CWDwarf_Debug\fP.
.P
The
\f(CW*section_count_out\fP
is set to the number of sections
in the object. Many of the sections
will be irrelevant to
\f(CWlibdwarf\fP.
.P
The
\f(CW*group_count_out\fP
is set to the number of groups
in the object (as
\f(CWlibdwarf\fP
counts them).
An OSO will have exactly one group.
A DWP object will have exactly one group.
If is more than one group consumer code
will likely want to open additional
\f(CWDwarf_Debug\fP
objects and request
relevant information to process the
DWARF contents.
An executable or a DWP object will
always have a
\f(CW*group_count_out\fP
of one(1).
An executable or a shared library
cannot have any COMDAT section groups
as the linker will have dealt with them.
.P
The
\f(CW*selected_group_out\fP
is set to the group number that
this
\f(CWDwarf_Debug\fP
will focus on.
See
\f(CWdwarf_sec_group_map()\fP
for additional details on how
\f(CW*selected_group_out\fP
is interpreted.
.P
The
\f(CW*map_entry_count_out\fP
is set to the number of
entries in the map.
See
\f(CWdwarf_sec_group_map()\fP.

.P
On failure it returns
\f(CWDW_DLV_ERROR\fP
and sets \f(CW*error\fP
.P
The initial implementation never returns
\f(CWDW_DLV_ERROR\fP
or
\f(CWDW_DLV_NO_ENTRY\fP
but callers should allow for that possibility.

.H 3 " dwarf_sec_group_map()"
.DS
\f(CW int dwarf_sec_group_map(
    Dwarf_Debug      dbg,
    Dwarf_Unsigned   map_entry_count,
    Dwarf_Unsigned * group_numbers_array,
    Dwarf_Unsigned * section_numbers_array,
    const char     ** sec_names_array,
    Dwarf_Error    * error)
\fP
.DE
The function
\f(CWdwarf_sec_group_map()\fP
may be called on any open
\f(CWDwarf_Debug\fP.
.P
The caller must allocate
\f(CWmap_entry_count\fP
arrays used in the
following three arguments
the and pass the appropriate
pointer into the function as well
as passing in
\f(CWmap_entry_count\fP
itself.
.P
The map entries returned cover all the
DWARF related sections in the object
though the
\f(CWselected_group\fP
value will dictate which of the sections
in the
\f(CWDwarf_Debug\fP
will actually be accessed via the usual
\f(CWlibdwarf\fP
functions.
That is, only sections in the selected group
may be directly accessed though libdwarf may
indirectly access sections in section group
one(1) so relevant details can be accessed,
such as abbreviation tables etc.
Describing the details of this access outside
the current
\f(CWselected_group\fP
goes beyond what this document covers (as of this
writing).

.P
It returns
\f(CWDW_DLV_OK\fP
on success and sets
values into the user-allocated
array elements (sorted by section number):
.in +2
.DS
\f(CW
group_numbers_array[0]...  group_numbers_array[map_entry_count-1]
section_numbers_array[0]...  section_numbers_array[map_entry_count-1]
sec_names_array[0]...  sec_names_array[map_entry_count-1]
\fP
.DE
.in -2
.P
\f(CWgroup_numbers_array[0]\fP
for example
is set to a group number. One(1), or two(2) or if there are
COMDAT groups it will be three(3) or higher.
.P
\f(CWsection_numbers_array[0]\fP
for example
is set to a valid Elf section number
relevant to
\f(CWDWARF\fP
(each section number shown will be greater than zero).
.P
\f(CWsec_names_array[0]\fP
for example
is set to a pointer to a string containing
the Elf section name of the Elf section number in
\f(CWsections_number_array[0]\fP.
.P
On error the function will return
\f(CWDW_DLV_ERROR\fP
or
\f(CWDW_DLV_NO_ENTRY\fP
which indicates a serious problem with this object.


.P
Here is an example of use of these functions.
.in +2
.DS
\f(CW
void examplesecgroup(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Unsigned  section_count = 0;
    Dwarf_Unsigned  group_count;
    Dwarf_Unsigned  selected_group = 0;
    Dwarf_Unsigned  group_map_entry_count = 0;
    Dwarf_Unsigned *sec_nums = 0;
    Dwarf_Unsigned *group_nums = 0;
    const char **   sec_names = 0;
    Dwarf_Error     error = 0;
    Dwarf_Unsigned  i = 0;


    res = dwarf_sec_group_sizes(dbg,&section_count,
        &group_count,&selected_group, &group_map_entry_count,
        &error);
    if(res != DW_DLV_OK) {
        /* Something is badly wrong*/
        return;
    }
    /*  In an object without split-dwarf sections
        or COMDAT sections we now have
        selected_group == 1. */
    sec_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if(!sec_nums) {
        /* FAIL. out of memory */
        return;
    }
    group_nums = calloc(group_map_entry_count,sizeof(Dwarf_Unsigned));
    if(!group_nums) {
        free(group_nums);
        /* FAIL. out of memory */
        return;
    }
    sec_names = calloc(group_map_entry_count,sizeof(char*));
    if(!sec_names) {
        free(group_nums);
        free(sec_nums);
        /* FAIL. out of memory */
        return;
    }

    res = dwarf_sec_group_map(dbg,group_map_entry_count,
        group_nums,sec_nums,sec_names,&error);
    if(res != DW_DLV_OK) {
        /* FAIL. Something badly wrong. */
    }
    for( i = 0; i < group_map_entry_count; ++i) {
        /*  Now do something with
            group_nums[i],sec_nums[i],sec_names[i] */
    }
    free(group_nums);
    free(sec_nums);
    /*  The strings are in Elf data.
        Do not free() the strings themselves.*/
    free(sec_names);
}
\fP
.DE
.in -2

.H 2 "Section size operations"
.P
These operations are informative but not normally needed.
.H 3 "dwarf_get_section_max_offsets_b()"
.DS
\f(CWint dwarf_get_section_max_offsets_b(Dwarf_debug dbg,
    Dwarf_Unsigned * /*debug_info_size*/,
    Dwarf_Unsigned * /*debug_abbrev_size*/,
    Dwarf_Unsigned * /*debug_line_size*/,
    Dwarf_Unsigned * /*debug_loc_size*/,
    Dwarf_Unsigned * /*debug_aranges_size*/,
    Dwarf_Unsigned * /*debug_macinfo_size*/,
    Dwarf_Unsigned * /*debug_pubnames_size*/,
    Dwarf_Unsigned * /*debug_str_size*/,
    Dwarf_Unsigned * /*debug_frame_size*/,
    Dwarf_Unsigned * /*debug_ranges_size*/,
    Dwarf_Unsigned * /*debug_pubtypes_size*/,
    Dwarf_Unsigned * /*debug_types_size*/);
.DE
.P
The function
\f(CWdwarf_get_section_max_offsets_b()\fP an open
Dwarf_Dbg and reports on the section sizes by pushing
section size values  back through the pointers.

Created in October 2011.

.H 3 "dwarf_get_section_max_offsets()"
.DS
\f(CWint dwarf_get_section_max_offsets(Dwarf_debug dbg,
    Dwarf_Unsigned * /*debug_info_size*/,
    Dwarf_Unsigned * /*debug_abbrev_size*/,
    Dwarf_Unsigned * /*debug_line_size*/,
    Dwarf_Unsigned * /*debug_loc_size*/,
    Dwarf_Unsigned * /*debug_aranges_size*/,
    Dwarf_Unsigned * /*debug_macinfo_size*/,
    Dwarf_Unsigned * /*debug_pubnames_size*/,
    Dwarf_Unsigned * /*debug_str_size*/,
    Dwarf_Unsigned * /*debug_frame_size*/,
    Dwarf_Unsigned * /*debug_ranges_size*/,
    Dwarf_Unsigned * /*debug_pubtypes_size*/);
.DE
.P
The function is the same as \f(CWdwarf_get_section_max_offsets_b()\fP
except it is missing the \f(CWdebug_types_size()\fP argument.
Though obsolete it is still supported.

.H 2 "Printf Callbacks"
.P
This is new in August 2013.
.P
The \f(CWdwarf_print_lines()\fP function
is intended as a helper to programs like \f(CWdwarfdump\fP
and show some line internal details in a way only the interals
of libdwarf can show these details.
But using printf directly in libdwarf means the caller
has limited control of where the output appears.
So now the 'printf' output is passed back to the
caller through a callback function whose implementation
is provided by the caller.
.P
Any code calling libdwarf can ignore
the functions described in this section completely.
If the functions are ignored the messages
(if any) from libdwarf will simply not appear anywhere.
.P
The \f(CWlibdwarf.h\fP header file defines
\f(CWstruct Dwarf_Printf_Callback_Info_s\fP
and
\f(CWdwarf_register_printf_callback\fP
for those libdwarf callers wishing to implement the callback.
In this section we describe how one uses that interface.
The applications \f(CWdwarfdump\fP  and
\f(CWdwarfdump2\fP are examples of how these may be used.



.H 3 "dwarf_register_printf_callback"
.DS
\f(CWstruct  Dwarf_Printf_Callback_Info_s
    dwarf_register_printf_callback(Dwarf_Debug dbg,
    struct  Dwarf_Printf_Callback_Info_s * newvalues);
.DE
.P
The  \f(CWdwarf_register_printf_callback()\fP function
can only be called after the Dwarf_Debug instance
has been initialized, the call makes no sense at other times.
The function returns the current value of the structure.
If \f(CWnewvalues\fP is non-null then the passed-in
values are used to initialize the libdwarf internal
callback data (the values returned are the values
before the \f(CWnewvalues\fP are recorded).
If \f(CWnewvalues\fP is null no change is made to
the libdwarf internal callback data.




.H 3 "Dwarf_Printf_Callback_Info_s"
.DS
\f(CWstruct Dwarf_Printf_Callback_Info_s {
    void *                        dp_user_pointer;
    dwarf_printf_callback_function_type dp_fptr;
    char *                        dp_buffer;
    unsigned int                  dp_buffer_len;
    int                           dp_buffer_user_provided;
    void *                        dp_reserved;
};
.DE
.P
First we describe the fields as applicable in setting up
for a call to \f(CWdwarf_register_printf_callback()\fP.
.P
The field \f(CWdp_user_pointer\fP is remembered by libdwarf
and passed back in any call libdwarf makes to the
user's callback function.
It is otherwise ignored by libdwarf.
.P
The field \f(CWdp_fptr\fP is either NULL or a pointer to
a user-implemented function.
.P
If the field \f(CWdp_buffer_user_provided\fP is non-zero
then \f(CWdp_buffer_len\fP and \f(CWdp_buffer\fP
must be set by the user and libdwarf will use that buffer
without doing any malloc of space.
If the field \f(CWdp_buffer_user_provided\fP is zero
then the input fields \f(CWdp_buffer_len\fP and \f(CWdp_buffer\fP
are ignored by libdwarf and space is malloc'd as needed.
.P
The field \f(CWdp_reserved\fP is ignored, it is reserved for
future use.
.P
When the structure is returned by \f(CWdwarf_register_printf_callback()\fP
the values of the fields before the
\f(CWdwarf_register_printf_callback()\fP call are returned.


.H 3 "dwarf_printf_callback_function_type"
.DS
\f(CWtypedef void (* dwarf_printf_callback_function_type)(void * user_pointer,
    const char * linecontent);
.DE
.P
Any application using the callbacks needs to use the function
\f(CWdwarf_register_printf_callback()\fP and supply a function matching
the above function prototype from libdwarf.h.

.H 3 "Example of printf callback use in a C++ application using libdwarf"
.DS
\f(CWstruct Dwarf_Printf_Callback_Info_s printfcallbackdata;
    memset(&printfcallbackdata,0,sizeof(printfcallbackdata));
    printfcallbackdata.dp_fptr = printf_callback_for_libdwarf;
    dwarf_register_printf_callback(dbg,&printfcallbackdata);

Assuming the user implements something
like the following function in her application:

void
printf_callback_for_libdwarf(void *userdata,const char *data)
{
     cout << data;
}
.DE
.P
It is crucial that the user's callback function copies or
prints the data immediately. Once the user callback
function returns the \f(CWdata\fP
pointer may change or become stale without warning.


.H 2 "Debugging Information Entry Delivery Operations"
These functions are concerned with accessing debugging information
entries, whether from a .debug_info, .debug_types, .debug_info.dwo,
or .debug_types.dwo .
.P
Since all such sections use similar formats, one
set of functions suffices.

.H 3 "dwarf_get_die_section_name()"
.DS
int
dwarf_get_die_section_name(Dwarf_Debug dbg,
    Dwarf_Bool    is_info,
    const char ** sec_name,
    Dwarf_Error * error);
.DE
\f(CWdwarf_get_die_section_name()\fP lets consumers
access the object section name when no specific
DIE is at hand.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
See  also \f(CWdwarf_get_die_section_name_b()\fP.
.P
The function
\f(CWdwarf_get_die_section_name()\fP operates on
the either the .debug_info[.dwo] section
(if \f(CWis_info\fP is non-zero)
or .debug_types[.dwo]
section
(if \f(CWis_info\fP is zero).
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.
.H 3 "dwarf_get_die_section_name_b()"
.DS
int
dwarf_get_die_section_name_b(Dwarf_Die die,
    const char ** sec_name,
    Dwarf_Error * error);
.DE
\f(CWdwarf_get_die_section_name_b()\fP lets consumers
access the object section name when one has a DIE.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
See  also \f(CWdwarf_get_die_section_name()\fP.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.




.H 3 "dwarf_next_cu_header_d()"
.DS
\f(CWint dwarf_next_cu_header_d(
    Dwarf_debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Unsigned *cu_header_length,
    Dwarf_Half     *version_stamp,
    Dwarf_Unsigned *abbrev_offset,
    Dwarf_Half     *address_size,
    Dwarf_Half     *offset_size,
    Dwarf_Half     *extension_size,
    Dwarf_Sig8     *signature,
    Dwarf_Unsigned *typeoffset
    Dwarf_Unsigned *next_cu_header,
    Dwarf_Half     *header_cu_type,
    Dwarf_Error    *error);
.DE
The function
\f(CWdwarf_next_cu_header_d()\fP operates on
the either the .debug_info   section
(if \f(CWis_info\fP is non-zero) or .debug_types
section
(if \f(CWis_info\fP is zero).
It returns \f(CWDW_DLV_ERROR\fP
if it fails, and
\f(CWDW_DLV_OK\fP if it succeeds.
.P
If it succeeds, \f(CW*next_cu_header\fP is set to
the offset in the .debug_info section of the next
compilation-unit header if it succeeds.  On reading the last
compilation-unit header in the .debug_info section it contains
the size of the .debug_info or debug_types section.
The next call to
\f(CWdwarf_next_cu_header_b()\fP returns \f(CWDW_DLV_NO_ENTRY\fP
without reading a
compilation-unit or setting \f(CW*next_cu_header\fP.
Subsequent calls to \f(CWdwarf_next_cu_header()\fP
repeat the cycle by reading the first compilation-unit and so on.
.P
The other
values returned through pointers are the values in the compilation-unit
header.  If any of \f(CWcu_header_length\fP, \f(CWversion_stamp\fP,
\f(CWabbrev_offset\fP, \f(CWaddress_size\fP,
\f(CWoffset_size\fP, \f(CWextension_size\fP,
\f(CWsignature\fP, or \f(CWtypeoffset\fP,
is \f(CWNULL\fP, the
argument is ignored (meaning it is not an error to provide a
\f(CWNULL\fP pointer for any or all of these arguments).
.P
\f(CWcu_header_length\fP returns the length in bytes of the compilation
unit header.
.P
\f(CWversion_stamp\fP returns the section version, which
would be (for .debug_info) 2 for DWARF2, 3 for DWARF4, or
4 for DWARF4.
.P
\f(CWabbrev_offset\fP returns the .debug_abbrev
section offset of the abbreviations
for this compilation unit.
.P
\f(CWaddress_size\fP returns the size of an address in this
compilation unit.  Which is usually 4 or 8.
.P
\f(CWoffset_size\fP returns the size in bytes of
an offset for the compilation unit.  The offset size
is 4 for 32bit dwarf
and 8 for 64bit dwarf.
This is the offset size in dwarf data, not
the address size inside the executable code.
The offset size can be 4 even
if embedded in a 64bit elf file (which
is normal for 64bit elf), and can be 8 even in
a 32bit elf file (which probably will never be seen
in practice).
.P
The
\f(CWextension_size\fP pointer is only relevant if
the \f(CWoffset_size\fP pointer returns 8.
The value is not normally useful but is returned
through the pointer for completeness.
The pointer \f(CWextension_size\fP returns 0
if the CU is MIPS/IRIX non-standard 64bit dwarf
(MIPS/IRIX 64bit dwarf was created years before DWARF3
defined 64bit dwarf)
and returns 4 if the dwarf uses the standard 64bit
extension (the 4 is the size in bytes of the 0xffffffff
in the initial length field
which indicates the following 8 bytes in the .debug_info section
are the real length).
See the DWARF3 or DWARF4 standard, section 7.4.
.P
The
\f(CWsignature\fP pointer is only relevant if
 the CU has a type signature, and if relevant the 8 byte type
signature of the .debug_types CU header is assigned through
the pointer.
.P
The
\f(CWtypeoffset\fP pointer is only relevant
the CU has a type signature
if relevant the local offset
within the CU of the the type offset the .debug_types entry
represents is assigned through the pointer.
The
\f(CWtypeoffset\fP matters because a
DW_AT_type referencing the type unit may reference an inner type,
such as a  C++ class in a C++ namespace, but the type itself
has the enclosing namespace in the .debug_type type_unit.
.P
The
\f(CWheader_cu_type\fP pointer is applicable to
all  CU headers.
The value returned through the pointer is either
\f(CWDW_UT_compile\fP
\f(CWDW_UT_partial\fP
\f(CWDW_UT_type\fP
and identifies the header type of this CU.
In \f(CWDWARF4\fP a \f(CWDW_UT_type\fP
will be in \f(CW.debug_types\fP, but in
\f(CWDWARF5\fP these compilation units are in \f(CW.debug_info\fP
and the Debug Fission \f(CW.debug_info.dwo\fP .

.H 3 "dwarf_next_cu_header_c()"
.DS
\f(CWint dwarf_next_cu_header_c(
    Dwarf_debug dbg,
    Dwarf_Bool is_info,
    Dwarf_Unsigned *cu_header_length,
    Dwarf_Half     *version_stamp,
    Dwarf_Unsigned *abbrev_offset,
    Dwarf_Half     *address_size,
    Dwarf_Half     *offset_size,
    Dwarf_Half     *extension_size,
    Dwarf_Sig8     *signature,
    Dwarf_Unsigned *typeoffset
    Dwarf_Unsigned *next_cu_header,
    Dwarf_Error    *error);
.DE
The function
\f(CWdwarf_next_cu_header_c()\fP operates on
the either the .debug_info   section
(if \f(CWis_info\fP is non-zero) or .debug_types
section
(if \f(CWis_info\fP is zero).
.P
It operates exactly like
\f(CWdwarf_next_cu_header_d()\fP but
is missing the
\f(CWheader_type\fP field.
This is kept for compatibility.
All code using this should be changed to use
\f(CWdwarf_next_cu_header_d()\fP


.H 3 "dwarf_next_cu_header_b()"
.DS
\f(CWint dwarf_next_cu_header_b(
    Dwarf_debug dbg,
    Dwarf_Unsigned *cu_header_length,
    Dwarf_Half     *version_stamp,
    Dwarf_Unsigned *abbrev_offset,
    Dwarf_Half     *address_size,
    Dwarf_Half     *offset_size,
    Dwarf_Half     *extension_size,
    Dwarf_Unsigned *next_cu_header,
    Dwarf_Error    *error);
.DE
.P
This is obsolete as of October 2011 though supported.
.P
The function
\f(CWdwarf_next_cu_header_b()\fP  operates on
the .debug_info section.  It operates exactly like
\f(CWdwarf_next_cu_header_c()\fP but
is missing the
\f(CWsignature\fP, and \f(CWtypeoffset\fP
fields.
This is kept for compatibility.
All code using this should be changed to use
\f(CWdwarf_next_cu_header_c()\fP

.H 3 "dwarf_next_cu_header()"
.P
The following is the original form, missing the
\f(CWoffset_size\fP, \f(CWextension_size\fP,
\f(CWsignature\fP, and \f(CWtypeoffset\fP
fields in
\f(CWdwarf_next_cu_header_c()\fP.
This is kept for compatibility.
All code using this should be changed to use
\f(CWdwarf_next_cu_header_c()\fP
.DS
\f(CWint dwarf_next_cu_header(
    Dwarf_debug dbg,
    Dwarf_Unsigned *cu_header_length,
    Dwarf_Half     *version_stamp,
    Dwarf_Unsigned *abbrev_offset,
    Dwarf_Half     *address_size,
    Dwarf_Unsigned *next_cu_header,
    Dwarf_Error    *error);
.DE

.H 3 "dwarf_siblingof_b()"
.DS
\f(CWint dwarf_siblingof_b(
    Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Bool is_info,
    Dwarf_Die *return_sib,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_siblingof_b()\fP
returns \f(CWDW_DLV_ERROR\fP and sets the \f(CWerror\fP pointer on error.
If there is no sibling it returns \f(CWDW_DLV_NO_ENTRY\fP.
When it succeeds,
\f(CWdwarf_siblingof_b()\fP returns
\f(CWDW_DLV_OK\fP  and sets \f(CW*return_sib\fP to the \f(CWDwarf_Die\fP
descriptor of the sibling of \f(CWdie\fP.

If \f(CWis_info\fP is non-zero then the  \f(CWdie\fP
is assumed to refer to a .debug_info DIE.
If \f(CWis_info\fP is zero then the  \f(CWdie\fP
is assumed to refer to a .debug_types DIE.
Note that the first call (the call that gets the compilation-unit
DIE in a compilation unit) passes in a NULL \f(CWdie\fP
so having the caller pass in \f(CWis_info\fP is essential.
And if \f(CWdie\fP is non-NULL it is still essential for the
call to pass in  \f(CWis_info\fP set properly to reflect the
section the DIE came from.
The function
\f(CWdwarf_get_die_infotypes_flag()\fP  is of interest as
it returns the proper is_info value from any non-NULL \f(CWdie\fP
pointer.


If \f(CWdie\fP is \fINULL\fP, the \f(CWDwarf_Die\fP descriptor of the
first die in the compilation-unit is returned.
This die has the
\f(CWDW_TAG_compile_unit\fP,
\f(CWDW_TAG_partial_unit\fP,
or \f(CWDW_TAG_type_unit\fP
tag.

.in +2
.FG "Example4 dwarf_siblingof()"
.DS
\f(CW
void example4(Dwarf_Debug dbg,Dwarf_Die in_die,Dwarf_Bool is_info)
{
    Dwarf_Die return_sib = 0;
    Dwarf_Error error = 0;
    int res = 0;

    /* in_die might be NULL or a valid Dwarf_Die */
    res = dwarf_siblingof_b(dbg,in_die,is_info,&return_sib, &error);
    if (res == DW_DLV_OK) {
        /* Use return_sib here. */
        dwarf_dealloc(dbg, return_sib, DW_DLA_DIE);
        /*  return_sib is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_sib = 0;
    }
}
\fP
.DE
.in -2

.H 3 "dwarf_siblingof()"
.DS
\f(CWint dwarf_siblingof(
    Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Die *return_sib,
    Dwarf_Error *error)\fP
.DE
.P
\f(CWint dwarf_siblingof()\fP operates exactly the same as
\f(CWint dwarf_siblingof_b()\fP, but
\f(CWint dwarf_siblingof()\fP refers only to .debug_info
DIEs.


.H 3 "dwarf_child()"
.DS
\f(CWint dwarf_child(
    Dwarf_Die die,
    Dwarf_Die *return_kid,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_child()\fP
returns \f(CWDW_DLV_ERROR\fP and sets the \f(CWerror\fP die on error.
If there is no child it returns \f(CWDW_DLV_NO_ENTRY\fP.
When it succeeds,
\f(CWdwarf_child()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_kid\fP
to the \f(CWDwarf_Die\fP descriptor
of the first child of \f(CWdie\fP.
The function
\f(CWdwarf_siblingof()\fP can be used with the return value of
\f(CWdwarf_child()\fP to access the other children of \f(CWdie\fP.

.in +2
.FG "Example5 dwarf_child()"
.DS
\f(CW
void example5(Dwarf_Debug dbg,Dwarf_Die in_die)
{
    Dwarf_Die return_kid = 0;
    Dwarf_Error error = 0;
    int res = 0;

    res = dwarf_child(in_die,&return_kid, &error);
    if (res == DW_DLV_OK) {
        /* Use return_kid here. */
        dwarf_dealloc(dbg, return_kid, DW_DLA_DIE);
        /*  return_die is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_kid = 0;
    }
}
\fP
.DE
.in -2

.H 3 "dwarf_offdie_b()"
.DS
\f(CWint dwarf_offdie_b(
    Dwarf_Debug dbg,
    Dwarf_Off offset,
    Dwarf_Bool is_info,
    Dwarf_Die *return_die,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_offdie_b()\fP
returns \f(CWDW_DLV_ERROR\fP and sets the \f(CWerror\fP die on error.
When it succeeds,
\f(CWdwarf_offdie_b()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_die\fP
to the
the \f(CWDwarf_Die\fP
descriptor of the debugging information entry at \f(CWoffset\fP in
the section containing debugging information entries i.e the .debug_info
section.
A return of \f(CWDW_DLV_NO_ENTRY\fP
means that the \f(CWoffset\fP in the section is of a byte containing
all 0 bits, indicating that there
is no abbreviation code. Meaning this 'die offset' is not
the offset of a real die, but is instead an offset of a null die,
a padding die, or of some random zero byte: this should
not be returned in normal use.
.P
It is the user's
responsibility to make sure that \f(CWoffset\fP is the start of a valid
debugging information entry.  The result of passing it an invalid
offset could be chaos.
.P
If \f(CWis_info\fP is non-zero the \f(CWoffset\fP must refer
to a .debug_info section offset.
If \f(CWis_info\fP zero the \f(CWoffset\fP must refer
to a .debug_types section offset.
Error returns or misleading
values may result if the
\f(CWis_info\fP flag
or the \f(CWoffset\fP value
are incorrect.

.in +2
.FG "Example6 dwarf_offdie_b()"
.DS
\f(CW
void example6(Dwarf_Debug dbg,Dwarf_Off die_offset,Dwarf_Bool is_info)
{
    Dwarf_Error error = 0;
    Dwarf_Die return_die = 0;
    int res = 0;

    res = dwarf_offdie_b(dbg,die_offset,is_info,&return_die, &error);
    if (res == DW_DLV_OK) {
        /* Use return_die here. */
        dwarf_dealloc(dbg, return_die, DW_DLA_DIE);
        /*  return_die is no longer usable for anything, we
            ensure we do not use it accidentally with: */
        return_die = 0;
    } else {
        /*  res could be NO ENTRY or ERROR, so no
            dealloc necessary.  */
    }
}
\fP
.DE
.in -2

.H 3 "dwarf_offdie()"
.DS
\f(CWint dwarf_offdie(
    Dwarf_Debug dbg,
    Dwarf_Off offset,
    Dwarf_Die *return_die,
    Dwarf_Error *error)\fP
.DE
.P
The function \f(CWdwarf_offdie()\fP is obsolete, use
\f(CWdwarf_offdie_b()\fP instead.
The function is still supported in the library, but only
references the .debug_info section.


.H 3 "dwarf_validate_die_sibling()"
.DS
\f(CWint validate_die_sibling(
    Dwarf_Die sibling,
    Dwarf_Off *offset)\fP
.DE
When used correctly in a depth-first walk of a DIE tree this
function validates that any DW_AT_sibling attribute gives
the same offset as the direct tree walk.
That is the only purpose of this function.

The function \f(CWdwarf_validate_die_sibling()\fP
returns \f(CWDW_DLV_OK\fP  if the last die processed
in a depth-first DIE tree walk was the same offset as
generated by a call to \f(CWdwarf_siblingof()\fP.
Meaning that the DW_AT_sibling attribute value, if any, was correct.

If the conditions are not met then DW_DLV_ERROR is returned
and  \f(CW*offset\fP is set to the offset
in the .debug_info section of the last DIE processed.
If the application prints the offset a knowledgeable
user may be able to figure out what the compiler did wrong.

.H 2 "Debugging Information Entry Query Operations"
These queries return specific information about debugging information
entries or a descriptor that can be used on subsequent queries when
given a \f(CWDwarf_Die\fP descriptor.  Note that some operations are
specific to debugging information entries that are represented by a
\f(CWDwarf_Die\fP descriptor of a specific type.
For example, not all
debugging information entries contain an attribute having a name, so
consequently, a call to \f(CWdwarf_diename()\fP using a \f(CWDwarf_Die\fP
descriptor that does not have a name attribute will return
\f(CWDW_DLV_NO_ENTRY\fP.
This is not an error, i.e. calling a function that needs a specific
attribute is not an error for a die that does not contain that specific
attribute.
.P
There are several methods that can be used to obtain the value of an
attribute in a given die:
.AL 1
.LI
Call \f(CWdwarf_hasattr()\fP to determine if the debugging information
entry has the attribute of interest prior to issuing the query for
information about the attribute.

.LI
Supply an \f(CWerror\fP argument, and check its value after the call to
a query indicates an unsuccessful return, to determine the nature of the
problem.  The \f(CWerror\fP argument will indicate whether an error occurred,
or the specific attribute needed was missing in that die.

.LI
Arrange to have an error handling function invoked upon detection of an
error (see \f(CWdwarf_init()\fP).

.LI
Call \f(CWdwarf_attrlist()\fP and iterate through the returned list of
attributes, dealing with each one as appropriate.
.LE
.P

.H 3 "dwarf_get_die_infotypes_flag()"
.DS
\f(CWDwarf_Bool dwarf_get_die_infotypes_flag(Dwarf_Die die)\fP
.DE
.P
The function \f(CWdwarf_tag()\fP returns the section flag
indicating which section the DIE originates from.
If the returned value is non-zero the DIE
originates from the .debug_info section.
If the returned value is zero the DIE
originates from the .debug_types section.

.H 3 "dwarf_tag()"
.DS
\f(CWint dwarf_tag(
    Dwarf_Die die,
    Dwarf_Half *tagval,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_tag()\fP returns the \f(CWtag\fP of \f(CWdie\fP
through the pointer  \f(CWtagval\fP if it succeeds.
It returns \f(CWDW_DLV_OK\fP if it succeeds.
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_dieoffset()"
.DS
\f(CWint dwarf_dieoffset(
    Dwarf_Die die,
    Dwarf_Off * return_offset,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
the function \f(CWdwarf_dieoffset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP
to the position of \f(CWdie\fP
in the section containing debugging information entries
(the \f(CWreturn_offset\fP is a section-relative offset).
In other words,
it sets \f(CWreturn_offset\fP
to the offset of the start of the debugging information entry
described by \f(CWdie\fP in the section containing dies i.e .debug_info.
It returns \f(CWDW_DLV_ERROR\fP on error.


.H 3 "dwarf_debug_addr_index_to_addr()"
.DS
int dwarf_debug_addr_index_to_addr(Dwarf_Die /*die*/,
    Dwarf_Unsigned  index,
    Dwarf_Addr    * return_addr,
    Dwarf_Error   * error);
.DE
Attributes with form DW_FORM_addrx, the
operation DW_OP_addrx, or
certain of the split-dwarf location list entries
give an index value to
a machine address in the .debug_addr section
(which is always in .debug_addr even when
the form/operation are in a split dwarf .dwo section).
.P
On successful return this function turns such an index
into a target address value through the pointer
\f(CWreturn_addr\fP .
.P
If there is an error this may return DW_
\f(CWDW_DLV_ERROR\fP and it will have
returned an error through \f(CW*error\fP.
.P
If there is no available .debug_addr section this
may return \f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_die_CU_offset()"
.DS
\f(CWint dwarf_die_CU_offset(
    Dwarf_Die die,
    Dwarf_Off *return_offset,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_die_CU_offset()\fP is similar to
\f(CWdwarf_dieoffset()\fP, except that it puts the offset of the DIE
represented by the \f(CWDwarf_Die\fP \f(CWdie\fP, from the
start of the compilation-unit that it belongs to rather than the start
of .debug_info (the \f(CWreturn_offset\fP is a CU-relative offset).

.H 3 "dwarf_die_offsets()"
.DS
\f(CWint dwarf_die_offsets(
    Dwarf_Die die,
    Dwarf_Off *global_off,
    Dwarf_Off *cu_off,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_die_offsets()\fP is a combination of
\f(CWdwarf_dieoffset()\fP and \f(CWdwarf_die_cu_offset()\fP
in that it returns both the global .debug_info offset and
the CU-relative offset of the \f(CWdie\fP in a single call.


.H 3 "dwarf_ptr_CU_offset()"
.DS
\f(CWint dwarf_ptr_CU_offset(
    Dwarf_CU_Context cu_context,
    Dwarf_Byte_ptr di_ptr ,
    Dwarf_Off *cu_off)\fP
.DE
Given a valid CU context pointer and a pointer into that CU
context,
the function \f(CWdwarf_ptr_CU_offset()\fP returns DW_DLV_OK
and sets \f(CW*cu_off\fP to the CU-relative (local) offset
in that CU.


.H 3 "dwarf_CU_dieoffset_given_die()"
.DS
\f(CWint dwarf_CU_dieoffset_given_die(
    Dwarf_Die given_die,
    Dwarf_Off *return_offset,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_CU_dieoffset_given_die()\fP is similar to
\f(CWdwarf_die_CU_offset()\fP, except that it puts the
global offset of the CU DIE owning \f(CWgiven_die\fP
of .debug_info (the \f(CWreturn_offset\fP is a global section offset).
.P
This is useful when processing a DIE tree and encountering
an error or other surprise in a DIE, as the \f(CWreturn_offset\fP
can be passed to \f(CWdwarf_offdie_b()\fP to return a pointer
to the CU die of the CU owning the \f(CWgiven_die\fP passed
to \f(CWdwarf_CU_dieoffset_given_die()\fP. The consumer can
extract information from the CU die and the \f(CWgiven_die\fP
(in the normal way) and print it.

An example  (a snippet) of code using this function
follows. It assumes that \f(CWin_die\fP is a DIE
in .debug_info
that, for some reason, you have decided needs CU context
printed (assuming \f(CWprint_die_data\fP
does some reasonable printing).

.in +2
.FG "Example7 dwarf_CU_dieoffset_given_die()"
.DS
\f(CW
void example7(Dwarf_Debug dbg, Dwarf_Die in_die,Dwarf_Bool is_info)
{
    int res = 0;
    Dwarf_Off cudieoff = 0;
    Dwarf_Die cudie = 0;
    Dwarf_Error error = 0;

    res = dwarf_CU_dieoffset_given_die(in_die,&cudieoff,&error);
    if(res != DW_DLV_OK) {
        /*  FAIL */
        return;
    }
    res = dwarf_offdie_b(dbg,cudieoff,is_info,&cudie,&error);
    if(res != DW_DLV_OK) {
        /* FAIL */
        return;
    }
    /* do something with cu_die */
    dwarf_dealloc(dbg,cudie, DW_DLA_DIE);
}
\fPy
.DE
.in -2



.H 3 "dwarf_die_CU_offset_range()"
.DS
\f(CWint dwarf_die_CU_offset_range(
    Dwarf_Die die,
    Dwarf_Off *cu_global_offset,
    Dwarf_Off *cu_length,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_die_CU_offset_range()\fP
returns the offset of the beginning of the CU and the length of the CU.
The offset and length are of the entire CU that this DIE is
a part of.  It is used by dwarfdump (for example) to check
the validity of offsets.
Most applications will have no reason to call this function.


.H 3 "dwarf_diename()"
.DS
\f(CWint dwarf_diename(
    Dwarf_Die die,
    char  ** return_name,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
the function \f(CWdwarf_diename()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP
to
a pointer to a
null-terminated string of characters that represents the
name attribute
(\f(CWDW_AT_name\fP)
of
\f(CWdie\fP.
.P
The storage pointed to by a successful return of
\f(CWdwarf_diename()\fP should be freed using the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest (see
\f(CWdwarf_dealloc()\fP).
.P
It returns \f(CWDW_DLV_NO_ENTRY\fP if
\f(CWdie\fP does not have a name attribute.
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.


.H 3 "dwarf_die_text()"
.DS
\f(CWint dwarf_die_text(
    Dwarf_Die die,
    Dwarf_Half attrnum,
    char  ** return_name,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
the function
\f(CWdwarf_die_text()\fP
returns
\f(CWDW_DLV_OK\fP
and sets
\f(CW*return_name\fP
to
a pointer to a
null-terminated string of characters that represents a
string-value
attribute of
\f(CWdie\fP
if an
attribute
\f(CWattrnum\fP
is present.
.P
The storage pointed to by a successful return of
\f(CWdwarf_die_text()\fP should be freed using the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest (see
\f(CWdwarf_dealloc()\fP).
.P
It returns
\f(CWDW_DLV_NO_ENTRY\fP
if \f(CWdie\fP does not have the attribute
\f(CWattrnum\fP.
It returns
\f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 3 "dwarf_die_abbrev_code()"
.DS
\f(CWint dwarf_die_abbrev_code( Dwarf_Die die)\fP
.DE
The function returns
the abbreviation code of the DIE.
That is, it returns the abbreviation "index"
into the abbreviation table for the compilation unit
of which the DIE is a part.
It cannot fail. No errors are possible.
The pointer \f(CWdie()\fP must not be NULL.

.H 3 "dwarf_die_abbrev_children_flag()"
.DS
\f(CWint dwarf_die_abbrev_children_flag( Dwarf_Die die,
      Dwarf_Half *has_child)\fP
.DE
The function returns the has-children flag of the \f(CWdie\fP
passed in through the \f(CW*has_child\fP passed in and returns
\f(CWDW_DLV_OK\fP on success.
A non-zero value of \f(CW*has_child\fP means the \f(CWdie\fP
has children.

On failure it returns \f(CWDW_DLV_ERROR\fP.

The function was developed to let
consumer code do better error reporting
in some circumstances, it is not generally needed.

.H 3 "dwarf_die_abbrev_global_offset()"
.DS
\f(CWint dwarf_die_abbrev_global_offset(Dwarf_Die die,
    Dwarf_Off       * abbrev_offset,
    Dwarf_Unsigned  * abbrev_count,
    Dwarf_Error*      error);\fP
.DE
The function allows more detailed printing of abbreviation
data.
It is handy for analyzing abbreviations but is not
normally needed by applications.
The function first appears in March 2016.
.P
On success the function returns \f(CWDW_DLV_OK\fP
and sets
\f(CW*abbrev_offset\fP
to the global offset
in the
\f(CW.debug_abbrev\fP
section of the abbreviation.
It also sets
\f(CW*abbrev_count\fP
to the number of attribute/form
pairs in the abbreviation entry.
It is possible, though unusual, for the
count to be zero (meaning there is abbreviation
instance and a TAG instance which have no attributes).
.P
On failure it returns
\f(CWDW_DLV_ERROR\fP
and sets \f(CW*error\fP
.P
It should never return \f(CWDW_DLV_NO_ENTRY\fP,
but callers should allow for that possibility..

.H 3 "dwarf_get_version_of_die()"
.DS
\f(CWint dwarf_get_version_of_die(Dwarf_Die die,
    Dwarf_Half *version,
    Dwarf_Half *offset_size)\fP
.DE
The function returns the CU context version through \f(CW*version\fP
and the CU context offset-size through \f(CW*offset_size\fP and
returns \f(CWDW_DLV_OK\fP on success.

In case of error, the only errors possible involve
an inappropriate NULL \f(CWdie\fP pointer so no Dwarf_Debug
pointer is available.  Therefore setting a Dwarf_Error would not
be very meaningful (there is no Dwarf_Debug to
attach it to).  The function returns DW_DLV_ERROR on error.

The values returned through the pointers are the values
two arguments to  dwarf_get_form_class() requires.

.H 3 "dwarf_attrlist()"
.DS
\f(CWint dwarf_attrlist(
    Dwarf_Die die,
    Dwarf_Attribute** attrbuf,
    Dwarf_Signed *attrcount,
    Dwarf_Error *error)\fP
.DE
When it returns \f(CWDW_DLV_OK\fP,
the function \f(CWdwarf_attrlist()\fP sets \f(CWattrbuf\fP to point
to an array of \f(CWDwarf_Attribute\fP descriptors corresponding to
each of the attributes in die, and returns the number of elements in
the array through \f(CWattrcount\fP.
\f(CWDW_DLV_NO_ENTRY\fP is returned if the count is zero (no
\f(CWattrbuf\fP is allocated in this case).
\f(CWDW_DLV_ERROR\fP is returned on error.
On a successful return from \f(CWdwarf_attrlist()\fP, each of the
\f(CWDwarf_Attribute\fP descriptors should be individually freed using
\f(CWdwarf_dealloc()\fP with the allocation type \f(CWDW_DLA_ATTR\fP,
followed by free-ing the list pointed to by \f(CW*attrbuf\fP using
\f(CWdwarf_dealloc()\fP with the allocation type \f(CWDW_DLA_LIST\fP,
when no longer of interest (see \f(CWdwarf_dealloc()\fP).

Freeing the attrlist:
.in +2
.FG "Example8 dwarf_attrlist() free"
.DS
\f(CW
void example8(Dwarf_Debug dbg, Dwarf_Die somedie)
{
    Dwarf_Signed atcount = 0;
    Dwarf_Attribute *atlist = 0;
    Dwarf_Error error = 0;
    int errv = 0;

    errv = dwarf_attrlist(somedie, &atlist,&atcount, &error);
    if (errv == DW_DLV_OK) {
        Dwarf_Signed i = 0;

        for (i = 0; i < atcount; ++i) {
            /* use atlist[i] */
            dwarf_dealloc(dbg, atlist[i], DW_DLA_ATTR);
        }
        dwarf_dealloc(dbg, atlist, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2
.P
.H 3 "dwarf_hasattr()"
.DS
\f(CWint dwarf_hasattr(
    Dwarf_Die die,
    Dwarf_Half attr,
    Dwarf_Bool *return_bool,
    Dwarf_Error *error)\fP
.DE
When it succeeds, the
function \f(CWdwarf_hasattr()\fP returns \f(CWDW_DLV_OK\fP
and sets \f(CW*return_bool\fP to \fInon-zero\fP if
\f(CWdie\fP has the attribute \f(CWattr\fP and \fIzero\fP otherwise.
If it fails, it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_attr()"
.DS
\f(CWint dwarf_attr(
    Dwarf_Die die,
    Dwarf_Half attr,
    Dwarf_Attribute *return_attr,
    Dwarf_Error *error)\fP
.DE
.P
When it returns \f(CWDW_DLV_OK\fP,
the function \f(CWdwarf_attr()\fP
sets
\f(CW*return_attr\fP to the  \f(CWDwarf_Attribute\fP
descriptor of \f(CWdie\fP having the attribute \f(CWattr\fP.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWattr\fP is not contained
in \f(CWdie\fP.
It returns \f(CWDW_DLV_ERROR\fP if an error occurred.


.H 3 "dwarf_lowpc()"
.DS
\f(CWint dwarf_lowpc(
    Dwarf_Die     die,
    Dwarf_Addr  * return_lowpc,
    Dwarf_Error * error)\fP
.DE
The function \f(CWdwarf_lowpc()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_lowpc\fP
to the low program counter
value associated with the \f(CWdie\fP descriptor if \f(CWdie\fP
represents a debugging information entry with the
\f(CWDW_AT_low_pc\fP attribute.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not have this
attribute.
It returns \f(CWDW_DLV_ERROR\fP if an error occurred.

.H 3 "dwarf_highpc_b()"
.DS
\f(CWint dwarf_highpc_b(
    Dwarf_Die               die,
    Dwarf_Addr  *           return_highpc,
    Dwarf_Half  *           return_form*/,
    enum Dwarf_Form_Class * return_class*/,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_highpc_b()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_highpc\fP
to the value of the \f(CWDW_AT_high_pc\fP attribute.
It also sets \f(CWreturn_form\fP to the FORM
of the attribute.
It also sets \f(CWreturn_class\fP to the form class
of the attribute.

If the form class  returned is \f(CWDW_FORM_CLASS_ADDRESS\fP
the \f(CWreturn_highpc\fP is an actual pc address (1 higher
than the address of the last pc in the address range)..
If the form class  returned is \f(CWDW_FORM_CLASS_CONSTANT\fP
the \f(CWreturn_highpc\fP is an offset from the value of
the the DIE's  low PC address (see DWARF4 section 2.17.2 Contiguous
Address Range).

It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not have
the \f(CWDW_AT_high_pc\fP attribute.

It returns \f(CWDW_DLV_ERROR\fP if an error occurred.

.H 3 "dwarf_highpc()"
.DS
\f(CWint dwarf_highpc(
    Dwarf_Die die,
    Dwarf_Addr  * return_highpc,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_highpc()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_highpc\fP
the high program counter
value associated with the \f(CWdie\fP descriptor if \f(CWdie\fP
represents a debugging information entry with the
\f(CWDW_AT_high_pc attribute\fP and the form is \f(CWDW_FORM_addr\fP
(meaning the form is of class address).
.P
This function is useless for a \f(CWDW_AT_high_pc\fP
which is encoded as a constant (which was first possible in
DWARF4).
.P
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not have this
attribute.
.P
It returns \f(CWDW_DLV_ERROR\fP if an error occurred or if the
form is not of class address.

.H 3 "dwarf_dietype_offset()"
.DS
\f(CWint dwarf_dietype_offset(Dwarf_Die /*die*/,
    Dwarf_Off   * /*return_off*/,
    Dwarf_Error * /*error*/);\fP
.DE
On success the function
\f(CWdwarf_dietype_offset()\fP
returns
the offset referred to by
\f(CWDW_AT_type\fP
attribute of
\f(CWdie\fP.
.P
\f(CWDW_DLV_NO_ENTRY\fP
is returned if the
\f(CWdie\fP
has no
\f(CWDW_AT_type\fP
attribute.
.P
\f(CWDW_DLV_ERROR\fP
is returned if an error is
detected.
.P
This feature was introduced in February 2016.

.H 3 "dwarf_offset_list()"
.DS
\f(CWint dwarf_offset_list(Dwarf_Debug dbg,
    Dwarf_Off         offset,
    Dwarf_Bool        is_info,
    Dwarf_Off      ** offbuf,
    Dwarf_Unsigned *  offcnt,
    Dwarf_Error    *  error);
.DE
On success The function
\f(CWdwarf_offset_list()\fP
returns
\f(CWDW_DLV_OK\fP and
sets
\f(CW*offbuf\fP to point to
an array of the offsets of the direct children
of the die
at
\f(CWoffset\fP.
It sets
\f(CW*offcnt\fP to point to
the count of entries in
the
\f(CWoffset\fP array
.P
In case of error it returns
\f(CWDW_DLV_OK\fP.
.P
It does not return
\f(CWDW_DLV_NO_ENTRY\fP
but callers should allow
for that possibility anyway.
.P
This feature was introduced in March 2016.
.P
Freeing the offset_list is done as follows.:
.in +2
.FG "Exampleoffset_list dwarf_offset_list() free"
.DS
\f(CW
void exampleoffset_list(Dwarf_Debug dbg, Dwarf_Off dieoffset,
    Dwarf_Bool is_info)
{
    Dwarf_Unsigned offcnt = 0;
    Dwarf_Off *offbuf = 0;
    Dwarf_Error error = 0;
    int errv = 0;

    errv = dwarf_offset_list(dbg,dieoffset, is_info,
        &offbuf,&offcnt, &error);
    if (errv == DW_DLV_OK) {
        Dwarf_Unsigned i = 0;

        for (i = 0; i < offcnt; ++i) {
            /* use offbuf[i] */
        }
        dwarf_dealloc(dbg, offbuf, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2
.P



.H 3 "dwarf_bytesize()"
.DS
\f(CWDwarf_Signed dwarf_bytesize(
    Dwarf_Die        die,
    Dwarf_Unsigned  *return_size,
    Dwarf_Error     *error)\fP
.DE
When it succeeds,
\f(CWdwarf_bytesize()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_size\fP
to the number of bytes
needed to contain an instance of the aggregate debugging information
entry represented by \f(CWdie\fP.
It returns \f(CWDW_DLV_NO_ENTRY\fP if
\f(CWdie\fP does not contain the byte size attribute \f(CWDW_AT_byte_size\fP.
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 3 "dwarf_bitsize()"
.DS
\f(CWint dwarf_bitsize(
    Dwarf_Die die,
    Dwarf_Unsigned  *return_size,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_bitsize()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_size\fP
to the number of
bits
occupied by the bit field value that is an attribute of the given
die.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not
contain the bit size attribute \f(CWDW_AT_bit_size\fP.
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 3 "dwarf_bitoffset()"
.DS
\f(CWint dwarf_bitoffset(
    Dwarf_Die die,
    Dwarf_Unsigned  *return_size,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_bitoffset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_size\fP
to the number of bits
to the left of the most significant bit of the bit field value.
This bit offset is not necessarily the net bit offset within the
structure or class , since \f(CWDW_AT_data_member_location\fP
may give a byte offset to this \f(CWDIE\fP and the bit offset
returned through the pointer
does not include the bits in the byte offset.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not contain the
bit offset attribute \f(CWDW_AT_bit_offset\fP.
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 3 "dwarf_srclang()"
.DS
\f(CWint dwarf_srclang(
    Dwarf_Die die,
    Dwarf_Unsigned  *return_lang,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_srclang()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_lang\fP
to
a code indicating the
source language of the compilation unit represented by the descriptor
\f(CWdie\fP.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not
represent a source file debugging information entry (i.e. contain the
attribute \f(CWDW_AT_language\fP).
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 3 "dwarf_arrayorder()"
.DS
\f(CWint dwarf_arrayorder(
    Dwarf_Die die,
    Dwarf_Unsigned  *return_order,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_arrayorder()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_order\fP
a code indicating
the ordering of the array represented by the descriptor \f(CWdie\fP.
It returns \f(CWDW_DLV_NO_ENTRY\fP if \f(CWdie\fP does not contain the
array order attribute \f(CWDW_AT_ordering\fP.
It returns \f(CWDW_DLV_ERROR\fP if
an error occurred.

.H 2 "Attribute Queries"
Based on the attributes form, these operations are concerned with
returning uninterpreted attribute data.  Since it is not always
obvious from the return value of these functions if an error occurred,
one should always supply an
\f(CWerror\fP
parameter or have arranged
to have an error handling function invoked (see
\f(CWdwarf_init()\fP
)
to determine the validity of the returned value and the nature of any
errors that may have occurred.

A \f(CWDwarf_Attribute\fP descriptor describes an attribute of a
specific die.  Thus, each \f(CWDwarf_Attribute\fP descriptor is
implicitly associated with a specific die.

.H 3 "dwarf_hasform()"
.DS
\f(CWint dwarf_hasform(
    Dwarf_Attribute attr,
    Dwarf_Half form,
    Dwarf_Bool  *return_hasform,
    Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_hasform()\fP returns
\f(CWDW_DLV_OK\fP and  and puts a
\fInon-zero\fP
 value in the
\f(CW*return_hasform\fP boolean if the
attribute represented by the \f(CWDwarf_Attribute\fP descriptor
\f(CWattr\fP has the attribute form \f(CWform\fP.
If the attribute does not have that form \fIzero\fP
is put into \f(CW*return_hasform\fP.
\f(CWDW_DLV_ERROR\fP is returned on error.

.H 3 "dwarf_whatform()"
.DS
\f(CWint dwarf_whatform(
    Dwarf_Attribute attr,
    Dwarf_Half     *return_form,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_whatform()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_form\fP
to the attribute form code of
the attribute represented by the \f(CWDwarf_Attribute\fP descriptor
\f(CWattr\fP.
It returns  \f(CWDW_DLV_ERROR\fP  on error.

An attribute using DW_FORM_indirect effectively has two forms.
This function returns the 'final' form for \f(CWDW_FORM_indirect\fP,
not the \f(CWDW_FORM_indirect\fP itself. This function is
what most applications will want to call.

.H 3 "dwarf_whatform_direct()"
.DS
\f(CWint dwarf_whatform_direct(
    Dwarf_Attribute attr,
    Dwarf_Half     *return_form,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_whatform_direct()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_form\fP
to the attribute form code of
the attribute represented by the \f(CWDwarf_Attribute\fP descriptor
\f(CWattr\fP.
It returns  \f(CWDW_DLV_ERROR\fP  on error.
An attribute using \f(CWDW_FORM_indirect\fP effectively has two forms.
This returns the form 'directly' in the initial form field.
That is, it returns the 'initial' form of the attribute.
.P
So when the form field is \f(CWDW_FORM_indirect\fP
this call returns the \f(CWDW_FORM_indirect\fP form,
which is sometimes useful for dump utilities.
.P
It is confusing that the _direct() function returns
DW_FORM_indirect if an indirect form is involved.
Just think of this as returning the initial form the first
form value seen for the attribute, which is also the final
form unless the initial form is \f(CWDW_FORM_indirect\fP.

.H 3 "dwarf_whatattr()"
.DS
\f(CWint dwarf_whatattr(
    Dwarf_Attribute attr,
    Dwarf_Half     *return_attr,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_whatattr()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_attr\fP
to the attribute code
represented by the \f(CWDwarf_Attribute\fP descriptor \f(CWattr\fP.
It returns  \f(CWDW_DLV_ERROR\fP  on error.

.H 3 "dwarf_formref()"
.DS
\f(CWint dwarf_formref(
    Dwarf_Attribute attr,
    Dwarf_Off     *return_offset,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_formref()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP
to the CU-relative offset
represented by the descriptor \f(CWattr\fP if the form of the attribute
belongs to the \f(CWREFERENCE\fP class.
\f(CWattr\fP must be a CU-local reference,
not form \f(CWDW_FORM_ref_addr\fP and not \f(CWDW_FORM_sec_offset\fP .
It is an error for the form to
not belong to the \f(CWREFERENCE\fP class.
It returns \f(CWDW_DLV_ERROR\fP on error.

Beginning November 2010:
All \f(CWDW_DLV_ERROR\fP returns set \f(CW*return_offset\fP. Most
errors set \f(CW*return_offset\fP to zero, but
for error \f(CWDW_DLE_ATTR_FORM_OFFSET_BAD\fP
the function sets \f(CW*return_offset\fP to the invalid
offset (which allows the caller to print a more
detailed error message).

See also \f(CWdwarf_global_formref\fP below.


.H 3 "dwarf_global_formref()"
.DS
\f(CWint dwarf_global_formref(
    Dwarf_Attribute attr,
    Dwarf_Off     *return_offset,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_global_formref()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP
to the section-relative offset
represented by the descriptor \f(CWattr\fP if the form of the attribute
belongs to the \f(CWREFERENCE\fP or other section-references classes.
.P
\f(CWattr\fP can be any legal
\f(CWREFERENCE\fP class form plus \f(CWDW_FORM_ref_addr\fP or
\f(CWDW_FORM_sec_offset\fP.
It is an error for the form to
not belong to one of the reference classes.
It returns \f(CWDW_DLV_ERROR\fP on error.
See also \f(CWdwarf_formref\fP above.
.P
The caller must determine which section the offset returned applies to.
The function \f(CWdwarf_get_form_class()\fP  is useful to determine
the applicable section.
.P
The function converts CU relative offsets from forms
such as DW_FORM_ref4 into
global section offsets.

.H 3 "dwarf_convert_to_global_offset()"
.DS
\f(CWint dwarf_convert_to_global_offset(
        Dwarf_Attribute attr,
        Dwarf_Off     offset,
        Dwarf_Off     *return_offset,
        Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_convert_to_global_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP
to the section-relative offset
represented by the cu-relative offset \f(CWoffset\fP
if the form of the attribute
belongs to the \f(CWREFERENCE\fP class.
\f(CWattr\fP must be a CU-local reference (DWARF class REFERENCE)
or form \f(CWDW_FORM_ref_addr\fP and the \f(CWattr\fP
must be directly relevant for the calculated \f(CW*return_offset\fP
to mean anything.

The function returns \f(CWDW_DLV_ERROR\fP on error.

The function is not strictly necessary but may be a
convenience for attribute printing  in case of error.


.H 3 "dwarf_formaddr()"
.DS
\f(CWint dwarf_formaddr(
        Dwarf_Attribute attr,
        Dwarf_Addr    * return_addr,
        Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_formaddr()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_addr\fP
to
the address
represented by the descriptor \f(CWattr\fP if the form of the attribute
belongs to the \f(CWADDRESS\fP class.
It is an error for the form to
not belong to this class.
It returns \f(CWDW_DLV_ERROR\fP on error.

One possible error that can arise (in a .dwo object file
or a .dwp package file) is
\f(CWDW_DLE_MISSING_NEEDED_DEBUG_ADDR_SECTION\fP.
Such an error means that the  .dwo or .dwp file
is missing the
\f(CW.debug_addr\fP
section.
When opening a .dwo object file or a .dwp package file
one should also open the corresponding executable
and use
\f(CWdwarf_set_tied_dbg()\fP
to associate the objects before calling dwarf_formaddr().



H 3 "dwarf_get_debug_addr_index()"
.DS
\f(CWint dwarf_get_debug_addr_index(
        Dwarf_Attribute attr,
        Dwarf_Unsigned  * return_index,
        Dwarf_Error *error)\fP
.DE
\f(CWdwarf_get_debug_addr_index()\fP
is only valid on attributes with form
\f(CWDW_FORM_GNU_addr_index\fP
or
\f(CWDW_FORM_addrx\fP.

The function makes it possible
to print the
index from a dwarf dumper program.

When it succeeds,
\f(CWdwarf_get_debug_addr_index()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_index\fP
to
the attribute's index (into the
\f(CW.debug_addr\fP section).

It returns \f(CWDW_DLV_ERROR\fP on error.



.H 3 "dwarf_get_debug_str_index()"

.DS
\f(CWint dwarf_get_debug_str_index(
        Dwarf_Attribute  attr,
        Dwarf_Unsigned * return_index,
        Dwarf_Error *    error);\fP
.DE
.P
For an attribute with  form
\f(CWDW_FORM_strx\fP
or
\f(CWDW_FORM_GNU_str_index\fP
this function retrieves the index (which refers
to a .debug_str_offsets section in this .dwo).
.P
If successful, the function
\f(CWdwarf_get_debug_str_index()\fP
returns
\f(CWDW_DLV_OK\fP
and
returns the index through the
\f(CWreturn_index()\fP pointer.
.P
If the passed in attribute does not have
this form or there is no valid compilation
unit context for the attribute the
function returns
\f(CWDW_DLV_ERROR\fP.
.P
\f(CWDW_DLV_NO_ENTRY\fP is not returned.

.H 3 "dwarf_formflag()"
.DS
\f(CWint dwarf_formflag(
    Dwarf_Attribute attr,
    Dwarf_Bool * return_bool,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_formflag()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_bool\fP
to the (one unsigned byte) flag value.
Any non-zero value means true.
A zero value means false.

Before 29 November 2012 this would only return 1 or zero
through the pointer, but that was always a strange thing to do.
The DWARF specification has always been clear that any non-zero
value means true.  The function should report the value
found truthfully, and now it does.

It returns \f(CWDW_DLV_ERROR\fP on error or if the \f(CWattr\fP
does not have form flag.

.H 3 "dwarf_formudata()"
.DS
\f(CWint dwarf_formudata(
    Dwarf_Attribute   attr,
    Dwarf_Unsigned  * return_uvalue,
    Dwarf_Error     * error)\fP
.DE
The function
\f(CWdwarf_formudata()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_uvalue\fP
to
the \f(CWDwarf_Unsigned\fP
value of the attribute represented by the descriptor \f(CWattr\fP if the
form of the attribute belongs to the \f(CWCONSTANT\fP class.
It is an
error for the form to not belong to this class.
It returns \f(CWDW_DLV_ERROR\fP on error.

Never returns \f(CWDW_DLV_NO_ENTRY\fP.

For DWARF2 and DWARF3, \f(CWDW_FORM_data4\fP and \f(CWDW_FORM_data8\fP
are possibly class \f(CWCONSTANT\fP,
and for DWARF4 and later they
are definitely class \f(CWCONSTANT\fP.

.H 3 "dwarf_formsdata()"
.DS
\f(CWint dwarf_formsdata(
        Dwarf_Attribute attr,
	Dwarf_Signed  * return_svalue,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_formsdata()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_svalue\fP
to
the \f(CWDwarf_Signed\fP
value of the attribute represented by the descriptor \f(CWattr\fP if the
form of the attribute belongs to the \f(CWCONSTANT\fP class.
It is an
error for the form to not belong to this class.
If the size of the data
attribute referenced is smaller than the size of the \f(CWDwarf_Signed\fP
type, its value is sign extended.
It returns \f(CWDW_DLV_ERROR\fP on error.

Never returns \f(CWDW_DLV_NO_ENTRY\fP.

For DWARF2 and DWARF3, \f(CWDW_FORM_data4\fP and \f(CWDW_FORM_data8\fP
are possibly class \f(CWCONSTANT\fP,
and for DWARF4 and later they
are definitely class \f(CWCONSTANT\fP.

.H 3 "dwarf_formblock()"
.DS
\f(CWint dwarf_formblock(
        Dwarf_Attribute attr,
	Dwarf_Block  ** return_block,
        Dwarf_Error *   error)\fP
.DE
The function \f(CWdwarf_formblock()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_block\fP
to
a pointer to a
\f(CWDwarf_Block\fP structure containing the value of the attribute
represented by the descriptor \f(CWattr\fP if the form of the
attribute belongs to the \f(CWBLOCK\fP class.
It is an error
for the form to not belong to this class.
The storage pointed
to by a successful return of \f(CWdwarf_formblock()\fP should
be freed using the allocation type \f(CWDW_DLA_BLOCK\fP,  when
no longer of interest (see \f(CWdwarf_dealloc()\fP).
It returns
\f(CWDW_DLV_ERROR\fP on error.


.H 3 "dwarf_formstring()"

.DS
\f(CWint dwarf_formstring(
        Dwarf_Attribute attr,
	char        **  return_string,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_formstring()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_string\fP
to
a pointer to a
null-terminated string containing  the value of the attribute
represented by the descriptor \f(CWattr\fP if the form of the
attribute belongs to the \f(CWSTRING\fP class.
It is an error
for the form to not belong to this class.
The storage pointed
to by a successful return of \f(CWdwarf_formstring()\fP
should not be freed.  The pointer points into
existing DWARF memory and the pointer becomes stale/invalid
after a call to \f(CWdwarf_finish\fP.
\f(CWdwarf_formstring()\fP returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_formsig8()"
.DS
\f(CWint dwarf_formsig8(
        Dwarf_Attribute attr,
        Dwarf_Sig8  * return_sig8,
        Dwarf_Error *   error)\fP
.DE
The function \f(CWdwarf_formsig8()\fP returns
\f(CWDW_DLV_OK\fP and copies the 8 byte signature
to a \f(CWDwarf_Sig8\fP structure provided by the caller
if the form of the
attribute is of form \f(CWDW_FORM_ref_sig8\fP
( a member of the \f(CWREFERENCE\fP class).
It is an error
for the form to be anything but \f(CWDW_FORM_ref_sig8\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
.P
This form is used to refer to a type unit.

.H 3 "dwarf_formexprloc()"
.DS
\f(CWint dwarf_formexprloc(
        Dwarf_Attribute attr,
        Dwarf_Unsigned * return_exprlen,
        Dwarf_Ptr  * block_ptr,
        Dwarf_Error *   error)\fP
.DE
The function \f(CWdwarf_formexprloc()\fP returns
\f(CWDW_DLV_OK\fP and sets the two values thru the pointers
to the length and bytes of the DW_FORM_exprloc entry
if the form of the
attribute is of form \f(CWDW_FORM_experloc\fP.
It is an error
for the form to be anything but \f(CWDW_FORM_exprloc\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
.P
On success the value set through the
\f(CWreturn_exprlen\fP pointer is the length
of the location expression.
On success the value set through the
\f(CWblock_ptr\fP pointer is a pointer to
the bytes of the location expression itself.

.H 3 "dwarf_get_form_class()"
.DS
\f(CWenum Dwarf_Form_Class dwarf_get_form_class(
    Dwarf_Half dwversion,
    Dwarf_Half attrnum,
    Dwarf_Half offset_size,
    Dwarf_Half form)\fP
.DE
.P
The function is just for the convenience
of libdwarf clients that might wish to categorize
the FORM of a particular attribute.
The DWARF specification divides FORMs into classes
in Chapter 7 and this function figures out the correct
class for a form.
.P
The \f(CWdwversion\fP passed in shall be the dwarf version
of the compilation unit involved (2 for DWARF2, 3 for
DWARF3, 4 for DWARF 4).
The \f(CWattrnum\fP passed in shall be the attribute
number of the attribute involved (for example, \f(CWDW_AT_name\fP ).
The \f(CWoffset_size\fP passed in shall be the
length of an offset in the current compilation unit
(4 for 32bit dwarf or 8 for 64bit dwarf).
The \f(CWform\fP passed in shall be the attribute form number.
If \f(CWform\fP
\f(CWDW_FORM_indirect\fP
is passed in \f(CWDW_FORM_CLASS_UNKNOWN\fP will be returned
as this form has no defined 'class'.
.P
When it returns \f(CWDW_FORM_CLASS_UNKNOWN\fP the
function is simply saying it could not determine the
correct class given the arguments
presented.  Some user-defined
attributes might have this problem.

The function \f(CWdwarf_get_version_of_die()\fP may be helpful
in filling out arguments for a call to \f(CWdwarf_get_form_class()\fP.

.H 3 "dwarf_discr_list()"
.DS
\f(CWint dwarf_discr_list(
    Dwarf_Debug dbg,
    Dwarf_Small    * blockpointer,
    Dwarf_Unsigned   blocklen,
    Dwarf_Dsc_Head * dsc_head_out,
    Dwarf_Unsigned * dsc_array_length_out,
    Dwarf_Error    * error)
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_discr_list()\fP
returns
\f(CWDW_DLV_OK\fP and sets
\f(CW*dsc_head_out\fP
to a pointer to the discriminant information
for the discriminant list and sets
\f(CW*dsc_array_length_out\fP
to the count of discriminant entries.
The only current applicability is
the block value of a
\f(CWDW_AT_discr_list\fP
attribute.
.P
Those values are useful for calls to
\f(CWdwarf_discr_entry_u()\fP
or
\f(CWdwarf_discr_entry_s()\fP
to get the actual discriminant values.
See the example below.
It returns \f(CWDW_DLV_NO_ENTRY\fP if
the block is empty.
It returns
\f(CWDW_DLV_ERROR\fP
if
an error occurred.
.P
When the call was successful and
the
\f(CWDwarf_Dsc_Head\fP
is no longer needed, call
\f(CWdwarf_dealloc()\fP
to free all the space
related to this.

.DS
void example_discr_list(Dwarf_Debug dbg,
    Dwarf_Die die,
    Dwarf_Attribute attr,
    Dwarf_Half attrnum,
    Dwarf_Bool isunsigned,
    Dwarf_Half theform,
    Dwarf_Error *err)
{
    /*  The example here assumes that
        attribute attr is a DW_AT_discr_list.
        isunsigned should be set from the signedness
        of the parent of 'die' per DWARF rules for
        DW_AT_discr_list. */
    enum Dwarf_Form_Class fc = DW_FORM_CLASS_UNKNOWN;
    Dwarf_Half version = 0;
    Dwarf_Half offset_size = 0;
    int wres = 0;

    wres = dwarf_get_version_of_die(die,&version,&offset_size);
    if (wres != DW_DLV_OK) {
        /* FAIL */
        return;
    }
    fc = dwarf_get_form_class(version,attrnum,offset_size,theform);
    if (fc == DW_FORM_CLASS_BLOCK) {
        int fres = 0;
        Dwarf_Block *tempb = 0;
        fres = dwarf_formblock(attr, &tempb, err);
        if (fres == DW_DLV_OK) {
            Dwarf_Dsc_Head h = 0;
            Dwarf_Unsigned u = 0;
            Dwarf_Unsigned arraycount = 0;
            int sres = 0;

            sres = dwarf_discr_list(dbg,
                (Dwarf_Small *)tempb->bl_data,
                tempb->bl_len,
                &h,&arraycount,err);
            if (sres == DW_DLV_NO_ENTRY) {
                /* Nothing here. */
                dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                return;
            }
            if (sres == DW_DLV_ERROR) {
                /* FAIL . */
                dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                return;
            }
            for(u = 0; u < arraycount; u++) {
                int u2res = 0;
                Dwarf_Half dtype = 0;
                Dwarf_Signed dlow = 0;
                Dwarf_Signed dhigh = 0;
                Dwarf_Unsigned ulow = 0;
                Dwarf_Unsigned uhigh = 0;

                if (isunsigned) {
                  u2res = dwarf_discr_entry_u(h,u,
                      &dtype,&ulow,&uhigh,err);
                } else {
                  u2res = dwarf_discr_entry_s(h,u,
                      &dtype,&dlow,&dhigh,err);
                }
                if( u2res == DW_DLV_ERROR) {
                    /* Something wrong */
                    dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
                    dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                    return;
                }
                if( u2res == DW_DLV_NO_ENTRY) {
                    /* Impossible. u < arraycount. */
                    dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
                    dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
                    return;
                }
                /*  Do something with dtype, and whichever
                    of ulow, uhigh,dlow,dhigh got set.
                    Probably save the values somewhere.
                    Simple casting of dlow to ulow (or vice versa)
                    will not get the right value due to the nature
                    of LEB values. Similarly for uhigh, dhigh.
                    One must use the right call.

                     */
            }
            dwarf_dealloc(dbg,h,DW_DLA_DSC_HEAD);
            dwarf_dealloc(dbg, tempb, DW_DLA_BLOCK);
        }
    }
}
.DE

.H 3 "dwarf_discr_entry_u()"
.DS
\f(CWint dwarf_discr_entry_u(
    Dwarf_Dsc_Head dsc_head,
    Dwarf_Unsigned dsc_array_index,
    Dwarf_Half *dsc_type,
    Dwarf_Unsigned *dsc_low,
    Dwarf_Unsigned *dsc_high,
    Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_discr_entry_u()\fP
returns
\f(CWDW_DLV_OK\fP and sets
\f(CW*dsc_type\fP,
\f(CW*dsc_low\fP,
and
\f(CW*dsc_high\fP
to the discriminent values for that index.
Valid
\f(CWdsc_array_index\fP
values
are zero to
\f(CW(dsc_array_length_out -1)\fP
from a
\f(CWdwarf_discr_list()\fP
call.
.P
If
\f(CW*dsc_type\fP is
\f(CWDW_DSC_label\fP
\f(CW*dsc_low\fP is
set to the discriminant value
and
\f(CW*dsc_high\fP is set to zero.
.P
If
\f(CW*dsc_type\fP
is
\f(CWDW_DSC_range\fP
\f(CW*dsc_low\fP
is
set to the low end of the discriminant
range and
and
\f(CW*dsc_high\fP is set to
the high end of the discriminant range.
.P
Due to the nature of the LEB numbers in
the discriminant representation in DWARF
one must call the correct one of
\f(CWdwarf_discr_entry_u()\fP
or
\f(CWdwarf_discr_entry_s()\fP
based on whether the discriminant is
signed or unsigned.
Casting an unsigned to signed is
not always going to get the right value.
.P
If
\f(CWdsc_array_index\fP
is outside the range of valid indexes
the function returns
\f(CWDW_DLV_NO_ENTRY\fP.
On error it returns
\f(CWDW_DLV_ERROR\fP
and sets
\f(CW*error\fP
to an error pointer.

.H 3 "dwarf_discr_entry_s()"
.DS
\f(CWint dwarf_discr_entry_s(
    Dwarf_Dsc_Head dsc_head,
    Dwarf_Unsigned dsc_array_index,
    Dwarf_Half *dsc_type,
    Dwarf_Signed *dsc_low,
    Dwarf_Signed *dsc_high,
    Dwarf_Error *error)\fP
.DE
This is identical to
\f(CWdwarf_discr_entry_u()\fP
except that the discriminant values
are signed values in this interface.
Callers must check the discriminant type
and call the correct function.


.H 2 "Location List operations"
.H 3 "dwarf_get_loclist_c()"
.DS
int dwarf_get_loclist_c (Dwarf_Attribute attr,
   Dwarf_Loc_Head_c * loclist_head,
   Dwarf_Unsigned   * locCount,
   Dwarf_Error      * error);
.DE
This function returns a pointer that is, in turn,
used to make possible calls to return the details
of the location list.
.P
The incoming argument
\f(CWattr\fP
should have one of the FORMs of a location
expression or location list.
.P
On success this returns
\f(CWDW_DLV_OK\fP
and sets
\f(CW*loclist_head\fP
to a pointer used in further calls (see the example
and descriptions that follow it).
\f(CWlocCount\fP
is set to the number of entries
in the location list (or if the FORM is of a location
expression the
\f(CWlocCount\fP
will be set to one).
At this point one cannot yet tell if it was a location
list or a location expression (see .
\f(CWdwarf_get_locdesc_entry_c{}\fP).
.P
In case of error
\f(CWDW_DLV_ERROR\fP
is returned and
\f(CW*error\fP
is set to an error designation.
.P
A return of
\f(CWDW_DLV_NO_ENTRY\fP
may be possible but is a bit odd.
.DS
\f(CW
void
example_loclistc(Dwarf_Debug dbg,Dwarf_Attribute someattr)
{
    Dwarf_Unsigned lcount = 0;
    Dwarf_Loc_Head_c loclist_head = 0;
    Dwarf_Error error = 0;
    int lres = 0;

    lres = dwarf_get_loclist_c(someattr,&loclist_head,&lcount,&error);
    if (lres == DW_DLV_OK) {
        Dwarf_Unsigned i = 0;

        /*  Before any return remember to call
            dwarf_loc_head_c_dealloc(loclist_head); */
        for (i = 0; i < lcount; ++i) {
            Dwarf_Small loclist_source = 0;
            Dwarf_Small lle_value = 0; /* DWARF5 */
            Dwarf_Addr lopc = 0;
            Dwarf_Addr hipc = 0;
            Dwarf_Unsigned ulocentry_count = 0;
            Dwarf_Locdesc_c locentry = 0;

            /*  section_offset is the section offset of the expression, not
                the location description prefix. */
            Dwarf_Unsigned section_offset = 0;

            /*  locdesc_offset is the section offset of the
                location description prefix. */
            Dwarf_Unsigned locdesc_offset = 0;

            lres = dwarf_get_locdesc_entry_c(loclist_head,
                i,
                &lle_value,&lopc,&hipc,
                &ulocentry_count,
                &locentry,
                &loclist_source,
                &section_offset,
                &locdesc_offset,
                &error);
            if (lres == DW_DLV_OK) {
                /*  Here, use loclist_source and
                    lle_value to determine what
                    sort of loclist it is and what to do with
                    the values. locentry_count will only be
                    more than zero if there is a set of location
                    operators.
                    One must use lle_value to determine how
                    to interpret lopc,hipc as sometimes they
                    are a target address and sometimes an
                    index into .debug_addr or even a length. */
                Dwarf_Unsigned j = 0;
                int opres = 0;
                 Dwarf_Small op = 0;

                for (j = 0; j < ulocentry_count; ++j) {
                    Dwarf_Unsigned opd1 = 0;
                    Dwarf_Unsigned opd2 = 0;
                    Dwarf_Unsigned opd3 = 0;
                    Dwarf_Unsigned offsetforbranch = 0;

                    opres = dwarf_get_location_op_value_c(locentry,
                        j,&op,&opd1, &opd2,&opd3,&offsetforbranch,
                        &error);
                    if (opres == DW_DLV_OK) {
                        /* Do something with the operators. */
                    } else {
                        /*Something is wrong. */
                    }

                }

            } else {
                /* Something is wrong. Do something. */
            }

        }
        /* In case of error or any other situation where one
            is giving up one can call dwarf_loc_head_c_dealloc()
            to free all the memory associated with loclist_head.  */
        dwarf_loc_head_c_dealloc(loclist_head);
        loclist_head = 0;
    }
}
\fP
.DE
.H 3 "dwarf_get_locdesc_entry_c()"
.DS
int dwarf_get_locdesc_entry_c(Dwarf_Loc_Head_c /*loclist_head*/,
   Dwarf_Unsigned    /*index*/,

   /* identifies type of locdesc entry*/
   Dwarf_Small    *  /*lle_value_out*/,
   Dwarf_Addr     *  /*lowpc_out*/,
   Dwarf_Addr     *  /*hipc_out*/,
   Dwarf_Unsigned *  /*loclist_count_out*/,

   /* Returns pointer to specific Locdesc index refers to */
   Dwarf_Locdesc_c * /*locentry_out*/,
   Dwarf_Small    *  /*loclist_source_out*/, /* 0,1, or 2 */
   Dwarf_Unsigned *  /*expression_offset_out*/,
   Dwarf_Unsigned *  /*locdesc_offset_out*/,
   Dwarf_Error    *  /*error*/);
.DE
This function returns overall information about a
location list or location description.
Details about location operators are retrieved
by a call to
\f(CWdwarf_get_location_op_value_c()\fP
(described
below).
The values returned here have been unified, hiding
irrelevant differences between DWARF2 location expressions/lists
and DWARF5 split-dwarf location expressions/lists.
.P
In case of success
\f(CWDW_DLV_OK\fP
is returned and arguments are set through
the pointers to return values to the caller.
Now we describe each argument.
.P
Return value
\f(CW*loclist_source_out\fP
is critical as it identifies the sort of entry we have.
If its value is zero (0) it identifies the location description
is a location expression.  In that case
\f(CW*lle_value_out\fP,
\f(CW*lowpc_out\fP, and
\f(CW*hipc_out\fP are not really interesting.
And because it is a location expression the
\f(CWindex\fP has to have been zero as there is no
real list, just an expression made to look like a list
entry.
.P
If
\f(CW*loclist_source_out\fP is one (1) then this is
a location list entry in DWARF2,3,4 loclist form.
Here the
\f(CW*lle_value_out\fP
has been created by libdwarf to match the split-dwarf
\f(CWDW_LLE_\fP value that the standard loclist entry
represents
(
\f(CWDW_LLE_end_of_list_entry\fP,
\f(CWDW_LLE_base_address_selection_entry\fP, or
\f(CWDW_LLE_offset_pair_entry\fP
).
.P
If
\f(CW*loclist_source_out\fP is two (2) then this is
a location list entry in DWARF5 split-dwarf (.dwo)
location-entry-form.
\f(CW*lle_value_out\fP
is set to the
\f(CWDW_LLE_\fP value that the split-dwarf loclist
entry contains.
.P
The
\f(CWDW_LLE_\fP value
determines how one is to interpret
\f(CWlopc_out\fP and
\f(CWhipc_out\fP.
See the DWARF5 standard.
.P
The argument
\f(CWloclist_count_out\fP
returns the number of operators in the location expression
involved (which may be zero).
.P
The argument
\f(CWlocentry_out\fP
returns an identifier used in calls to
\f(CWdwarf_get_location_op_value_c()\fP.
.P
The argument
\f(CWexpression_offset_out\fP
returns the offset (in the .debug_loc(.dso) or .debug_info(.dwo)
of the location expression itself
(possibly useful for debugging).
.P
The argument
\f(CWlocdesc_offset_out\fP
returns the offset (in the .debug_loc(.dso)
of the location list entry itself
(possibly useful for debugging).
.P
In case of error
\f(CWDW_DLV_ERROR\fP
is returned and
\f(CW*error\fP
is set to an error designation.
.P
A return of
\f(CWDW_DLV_NO_ENTRY\fP
may be possible but is a bit odd.

.H 3 "dwarf_get_location_op_value_c()"
.DS
int dwarf_get_location_op_value_c(Dwarf_Locdesc_c locdesc,
   Dwarf_Unsigned   index,
   Dwarf_Small    * atom_out,
   Dwarf_Unsigned * operand1,
   Dwarf_Unsigned * operand2,
   Dwarf_Unsigned * operand3,
   Dwarf_Unsigned * offset_for_branch,
   Dwarf_Error*     error);
.DE
On sucess
The function
\f(CWdwarf_get_location_op_value_c()\fP
returns
the information for the single operator
number
\f(CWindex\fP
from the location expression
\f(CWlocdesc\fP.
It sets the following values.
.P
\f(CWatom_out\fP
is set to the applicable operator code, for example
\f(CWDW_OP_reg5\fP.
.P
\f(CWoperand1\fP,
\f(CWoperand2\fP, and
\f(CWoperand3\fP are set to the
operator operands as applicable (see
DWARF documents on the operands
for each operator).
\f(CWoperand3\fP is new as of DWARF5.
.P
When a DWARF operand is not of a size
fixed by dwarf, or is possibly too large for
a dwarf stack entry, libdwarf will
insert a pointer (to memory
in the dwarf data somewhere) as the operand value.
\f(CWDW_OP_implicit_value operand 2\fP,
\f(CWDW_OP_[GNU_]entry_value operand 2\fP,
and
\f(CWDW_OP_[GNU_]const_type operand 3\fP
are instances of this.
.P
\f(CWoffset_for_branch\fP is set to the
offset (in bytes) in this expression of this
operator.
The value makes it possible for callers
to implement the operator branch
operators.
.P
In case of an error, the
function returns
\f(CWDW_DLV_ERROR\fP
and sets
\f(CW*error\fP to an error
value.
.P
\f(CWDW_DLV_NO_ENTRY\fP
is probably not a possible return value, but
please test for it anyway.

.H 3 "dwarf_loclist_from_expr_c()"
.DS
int dwarf_loclist_from_expr_c(Dwarf_Debug dbg,
    Dwarf_Ptr         expression_in,
    Dwarf_Unsigned    expression_length,
    Dwarf_Half        address_size,
    Dwarf_Half        offset_size,
    Dwarf_Small       dwarf_version,
    Dwarf_Loc_Head_c* loc_head,
    Dwarf_Unsigned  * listlen,
    Dwarf_Error     * error);
.DE
Frame operators such as
DW_CFA_def_cfa_expression have a location expression
and the location_expression is accessed with
this function.
.P
On success it returns
\f(CWDW_DLV_OK\fP and sets the two return arguments
(explained a few lines later here).
.P
The
\f(CWexpression_in\fP
argument must contain a valid pointer
to location expression bytes.
The
\f(CWexpression_length\fP
argument must contain the length of
that location expression in bytes.
.P
The
\f(CWaddress_size\fP
argument must contain the size of an address
on the target machine for this expression
(normally 4 or 8).
The
\f(CWoffset_size\fP
argument must contain the size of an offset
in the expression
(normally 4, sometimes 8).
The
\f(CWversion\fP
argument must contain the dwarf_version
of the expression
(2,3,4, or 5).
.P
The returned value
\f(CW*loc_head\fP
is  used to actually access the location expression
details (see the example following).
.P
The returned value
\f(CW*listlen\fP
is  the number of location expressions
(ie 1) in the location list
(for uniformity of access we make it look like
a single-entry location list).
.P
On error the function returns
\f(CWDW_DLV_ERROR\fP
and sets
\f(CW*error\fP to reflect
the error.
.P
A return of
\f(CWDW_DLV_NO_ENTRY\fP
is probably impossible, but
callers should assume it is possible.
No return arguments are set in this case.

.DS
void
example_locexprc(Dwarf_Debug dbg,Dwarf_Ptr expr_bytes,
    Dwarf_Unsigned expr_len,
    Dwarf_Half addr_size,
    Dwarf_Half offset_size,
    Dwarf_Half version)
{
    Dwarf_Loc_Head_c head = 0;
    Dwarf_Locdesc_c locentry = 0;
    int res2 = 0;
    Dwarf_Unsigned lopc = 0;
    Dwarf_Unsigned hipc = 0;
    Dwarf_Unsigned ulistlen = 0;
    Dwarf_Unsigned ulocentry_count = 0;
    Dwarf_Unsigned section_offset = 0;
    Dwarf_Unsigned locdesc_offset = 0;
    Dwarf_Small lle_value = 0;
    Dwarf_Small loclist_source = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Error error = 0;

    res2 = dwarf_loclist_from_expr_c(dbg,
        expr_bytes,expr_len,
        addr_size,
        offset_size,
        version,
        &head,
        &ulistlen,
        &error);
    if(res2 == DW_DLV_NO_ENTRY) {
        return;
    }
    if(res2 == DW_DLV_ERROR) {
        return;
    }
    /*  These are a location expression, not loclist.
        So we just need the 0th entry. */
    res2 = dwarf_get_locdesc_entry_c(head,
        0, /* Data from 0th LocDesc */
        &lle_value,
        &lopc, &hipc,
        &ulocentry_count,
        &locentry,
        &loclist_source,
        &section_offset,
        &locdesc_offset,
        &error);
    if (res2 == DW_DLV_ERROR) {
        dwarf_loc_head_c_dealloc(head);
        return;
    } else if (res2 == DW_DLV_NO_ENTRY) {
        dwarf_loc_head_c_dealloc(head);
        return;
    }
    /*  ASSERT: ulistlen == 1 */
    for (i = 0; i < ulocentry_count;++i) {
        Dwarf_Small op = 0;
        Dwarf_Unsigned opd1 = 0;
        Dwarf_Unsigned opd2 = 0;
        Dwarf_Unsigned opd3 = 0;
        Dwarf_Unsigned offsetforbranch = 0;

        res2 = dwarf_get_location_op_value_c(locentry,
            i, &op,&opd1,&opd2,&opd3,&offsetforbranch,
            &error);
        /* Do something with the expression operator and operands */
        if (res2 != DW_DLV_OK) {
            dwarf_loc_head_c_dealloc(head);
            return;
        }
    }
    dwarf_loc_head_c_dealloc(head);
}
.DE
.H 3 "dwarf_loc_head_c_dealloc()"
.DS
void dwarf_loc_head_c_dealloc(Dwarf_Loc_Head_c loclist_head);
.DE
This function frees all the memory associated with
the
\f(CWloclist_head\fP.
There is no return value.

.H 3 "dwarf_loclist_n()"

.DS
\f(CWint dwarf_loclist_n(
        Dwarf_Attribute attr,
        Dwarf_Locdesc ***llbuf,
        Dwarf_Signed  *listlen,
        Dwarf_Error *error)\fP
.DE
This interface cannot handle DWARF5 or Split Dwarf.
Use  \f(CWdwarf_get_loclist_c()\fP and related functions
instead (as of November 2015).
The function \f(CWdwarf_loclist_n()\fP sets \f(CW*llbuf\fP to point to
an array of \f(CWDwarf_Locdesc\fP pointers corresponding to each of
the location expressions in a location list, and sets
\f(CW*listlen\fP to the number
of elements in the array and
returns \f(CWDW_DLV_OK\fP if the attribute is
appropriate.
.P
This is the preferred function for Dwarf_Locdesc as
it is the interface allowing access to an entire
loclist. (use of \f(CWdwarf_loclist_n()\fP is
suggested as the better interface, though
\f(CWdwarf_loclist()\fP is still
supported.)
.P
If the attribute is a reference to a location list
(DW_FORM_data4 or DW_FORM_data8)
the location list entries are used to fill
in all the fields of the \f(CWDwarf_Locdesc\fP(s) returned.
.P
If the attribute is a location description
(DW_FORM_block2 or DW_FORM_block4)
then some of the \f(CWDwarf_Locdesc\fP values of the single
\f(CWDwarf_Locdesc\fP record are set to 'sensible'
but arbitrary values.  Specifically, ld_lopc is set to 0 and
ld_hipc is set to all-bits-on. And \f(CW*listlen\fP is set to 1.
.P
If the attribute is a reference to a location expression
(DW_FORM_locexper)
then some of the \f(CWDwarf_Locdesc\fP values of the single
\f(CWDwarf_Locdesc\fP record are set to 'sensible'
but arbitrary values.  Specifically, ld_lopc is set to 0 and
ld_hipc is set to all-bits-on. And \f(CW*listlen\fP is set to 1.
.P
It returns \f(CWDW_DLV_ERROR\fP on error.
.P
\f(CWdwarf_loclist_n()\fP works on \f(CWDW_AT_location\fP,
\f(CWDW_AT_data_member_location\fP, \f(CWDW_AT_vtable_elem_location\fP,
\f(CWDW_AT_string_length\fP, \f(CWDW_AT_use_location\fP, and
\f(CWDW_AT_return_addr\fP attributes.
.P
If the attribute is \f(CWDW_AT_data_member_location\fP the value
may be of class CONSTANT.  \f(CWdwarf_loclist_n()\fP is unable
to read class CONSTANT, so you need to first determine the
class using \f(CWdwarf_get_form_class()\fP and if it is
class CONSTANT call
\f(CWdwarf_formsdata()\fP or \f(CWdwarf_formudata()\fP
to get the constant value (you may need to call both as
DWARF4 does not define the signedness of the constant value).
.P
Storage allocated by a successful call of \f(CWdwarf_loclist_n()\fP should
be deallocated when no longer of interest (see \f(CWdwarf_dealloc()\fP).
The block of \f(CWDwarf_Loc\fP structs pointed to by the \f(CWld_s\fP
field of each \f(CWDwarf_Locdesc\fP structure
should be deallocated with the allocation type
\f(CWDW_DLA_LOC_BLOCK\fP.
and  the \f(CWllbuf[]\fP space pointed to should be deallocated with
allocation type \f(CWDW_DLA_LOCDESC\fP.
This should be followed by deallocation of the \f(CWllbuf\fP
using the allocation type \f(CWDW_DLA_LIST\fP.
.in +2
.DS
\f(CW
void example9(Dwarf_Debug dbg,Dwarf_Attribute someattr)
{
    Dwarf_Signed lcount = 0;
    Dwarf_Locdesc **llbuf = 0;
    Dwarf_Error error = 0;
    int lres = 0;

    lres = dwarf_loclist_n(someattr, &llbuf,&lcount,&error);
    if (lres == DW_DLV_OK) {
        Dwarf_Signed i = 0;
        for (i = 0; i < lcount; ++i) {
            /*  Use llbuf[i]. Both Dwarf_Locdesc and the
                array of Dwarf_Loc it points to are
                defined in libdwarf.h: they are
                not opaque structs. */
            dwarf_dealloc(dbg, llbuf[i]->ld_s, DW_DLA_LOC_BLOCK);
            dwarf_dealloc(dbg,llbuf[i], DW_DLA_LOCDESC);
        }
        dwarf_dealloc(dbg, llbuf, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2
.P

.H 3 "dwarf_loclist()"
.DS
\f(CWint dwarf_loclist(
        Dwarf_Attribute attr,
        Dwarf_Locdesc **llbuf,
        Dwarf_Signed  *listlen,
        Dwarf_Error *error)\fP
.DE
Use  \f(CWdwarf_get_loclist_c()\fP and related functions
instead (as of November 2015).
The function \f(CWdwarf_loclist()\fP sets \f(CW*llbuf\fP to point to
a \f(CWDwarf_Locdesc\fP pointer for the single location expression
it can return.
It sets
\f(CW*listlen\fP to 1.
and returns \f(CWDW_DLV_OK\fP
if the attribute is
appropriate.
.P
It is less flexible than \f(CWdwarf_loclist_n()\fP in that
\f(CWdwarf_loclist()\fP can handle a maximum of one
location expression, not a full location list.
If a location-list is present it returns only
the first location-list entry location description.
Use \f(CWdwarf_loclist_n()\fP instead.
.P
It returns \f(CWDW_DLV_ERROR\fP on error.
\f(CWdwarf_loclist()\fP works on \f(CWDW_AT_location\fP,
\f(CWDW_AT_data_member_location\fP, \f(CWDW_AT_vtable_elem_location\fP,
\f(CWDW_AT_string_length\fP, \f(CWDW_AT_use_location\fP, and
\f(CWDW_AT_return_addr\fP attributes.
.P
Storage allocated by a successful call of \f(CWdwarf_loclist()\fP should
be deallocated when no longer of interest (see \f(CWdwarf_dealloc()\fP).
The block of \f(CWDwarf_Loc\fP structs pointed to by the \f(CWld_s\fP
field of each \f(CWDwarf_Locdesc\fP structure
should be deallocated with the allocation type \f(CWDW_DLA_LOC_BLOCK\fP.
This should be followed by deallocation of the \f(CWllbuf\fP
using the allocation type \f(CWDW_DLA_LOCDESC\fP.
.in +2
.FG "Examplea dwarf_loclist()"
.DS
\f(CW
void examplea(Dwarf_Debug dbg,Dwarf_Attribute someattr)
{
    Dwarf_Signed lcount = 0;
    Dwarf_Locdesc *llbuf = 0;
    Dwarf_Error error = 0;
    int lres = 0;

    lres = dwarf_loclist(someattr, &llbuf,&lcount,&error);
    if (lres == DW_DLV_OK) {
        /* lcount is always 1, (and has always been 1) */
        /* Use llbuf here. */

        dwarf_dealloc(dbg, llbuf->ld_s, DW_DLA_LOC_BLOCK);
        dwarf_dealloc(dbg, llbuf, DW_DLA_LOCDESC);
    }
}
\fP
.DE
.in -2
.P

.H 3 "dwarf_loclist_from_expr()"
.DS
\f(CWint dwarf_loclist_from_expr(
        Dwarf_Debug dbg,
        Dwarf_Ptr bytes_in,
        Dwarf_Unsigned bytes_len,
        Dwarf_Locdesc **llbuf,
        Dwarf_Signed  *listlen,
        Dwarf_Error *error)\fP
.DE
Use \f(CWdwarf_loclist_from_expr_b()\fP instead.
This function is obsolete.
.P
The function \f(CWdwarf_loclist_from_expr()\fP
sets \f(CW*llbuf\fP to point to
a \f(CWDwarf_Locdesc\fP pointer for the single location expression
which is pointed to by \f(CW*bytes_in\fP (whose length is
\f(CW*bytes_len\fP).
It sets
\f(CW*listlen\fP to 1.
and returns \f(CWDW_DLV_OK\fP
if decoding is successful.
Some sources of bytes of expressions are dwarf expressions
in frame operations like \f(CWDW_CFA_def_cfa_expression\fP,
\f(CWDW_CFA_expression\fP, and  \f(CWDW_CFA_val_expression\fP.
.P
Any address_size data in the location expression is assumed
to be the same size as the default address_size for the object
being read (normally 4 or 8).
.P
It returns \f(CWDW_DLV_ERROR\fP on error.
.P
Storage allocated by a successful call
of \f(CWdwarf_loclist_from_expr()\fP should
be deallocated when no longer of interest (see \f(CWdwarf_dealloc()\fP).
The block of \f(CWDwarf_Loc\fP structs pointed to by the \f(CWld_s\fP
field of each \f(CWDwarf_Locdesc\fP structure
should be deallocated with the allocation type \f(CWDW_DLA_LOC_BLOCK\fP.
This should be followed by deallocation of the \f(CWllbuf\fP
using the allocation type \f(CWDW_DLA_LOCDESC\fP.
.in +2
.FG "Exampleb dwarf_loclist_from_expr()"
.DS
\f(CW
void exampleb(Dwarf_Debug dbg,Dwarf_Ptr data, Dwarf_Unsigned len)
{
    Dwarf_Signed lcount = 0;
    Dwarf_Locdesc *llbuf = 0;
    Dwarf_Error error = 0;
    int lres = 0;

    lres = dwarf_loclist_from_expr(dbg,data,len, &llbuf,&lcount,
        &error);
    if (lres == DW_DLV_OK) {
        /* lcount is always 1 */
        /* Use llbuf  here.*/

        dwarf_dealloc(dbg, llbuf->ld_s, DW_DLA_LOC_BLOCK);
        dwarf_dealloc(dbg, llbuf, DW_DLA_LOCDESC);
    }
}
\fP
.DE
.in -2
.P
.H 3 "dwarf_loclist_from_expr_b()"
.DS
\f(CWint dwarf_loclist_from_expr_a(
        Dwarf_Ptr bytes_in,
        Dwarf_Unsigned bytes_len,
        Dwarf_Half addr_size,
        Dwarf_Half offset_size,
        Dwarf_Half version_stamp,
        Dwarf_Locdesc **llbuf,
        Dwarf_Signed  *listlen,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_loclist_from_expr_b()\fP
is identical to  \f(CWdwarf_loclist_from_expr_a()\fP
in every way except that the caller passes an additional argument
\f(CWversion_stamp\fP containing the
version stamp (2 for DWARF2, etc) of the CU using
this location expression and an additional argument
of the offset size of the CU using this location expression.
The DW_OP_GNU_implicit_pointer operation requires this version
and offset information to be correctly processed.
.P
The \f(CWaddr_size\fP argument (from 27April2009) is needed
to correctly interpret frame information as different compilation
units can have different address sizes.
DWARF4 adds address_size to the CIE header.

.P
.H 3 "dwarf_loclist_from_expr_a()"
.DS
\f(CWint dwarf_loclist_from_expr_a(
        Dwarf_Ptr bytes_in,
        Dwarf_Unsigned bytes_len,
        Dwarf_Half addr_size,
        Dwarf_Locdesc **llbuf,
        Dwarf_Signed  *listlen,
        Dwarf_Error *error)\fP
.DE
Use \f(CWdwarf_loclist_from_expr_b()\fP instead.
This function is obsolete.
.P
The function \f(CWdwarf_loclist_from_expr_a()\fP
is identical to  \f(CWdwarf_loclist_from_expr()\fP
in every way except that the caller passes the additional argument
\f(CWaddr_size\fP containing the address size (normally 4 or 8)
applying this location expression.
.P
The \f(CWaddr_size\fP argument (added 27April2009) is needed
to correctly interpret frame information as different compilation
units can have different address sizes.
DWARF4 adds address_size to the CIE header.

.P
.H 2 "Line Number Operations"
These functions are concerned with accessing line number entries,
mapping debugging information entry objects to their corresponding
source lines, and providing a mechanism for obtaining information
about line number entries.  Although, the interface talks of "lines"
what is really meant is "statements".  In case there is more than
one statement on the same line, there will be at least one descriptor
per statement, all with the same line number.  If column number is
also being represented they will have the column numbers of the start
of the statements also represented.
.P
There can also be more than one Dwarf_Line per statement.
For example, if a file is preprocessed by a language translator,
this could result in translator output showing 2 or more sets of line
numbers per translated line of output.
.P
\f(CW
\fP
As of October 2015 there are two sets of overall access
and release functions.
The older set of functions is
\f(CWdwarf_srclines()\fP
with
\f(CWdwarf_srclines_dealloc()\fP.
This set does not handle line table
headers with no lines.
.P
A newer set is
\f(CWdwarf_srclines_b()\fP
with
\f(CWdwarf_srclines_from_linecontext()\fP
and
\f(CWdwarf_srclines_dealloc_b()\fP.
These functions provide for handling
both DWARF2 through DWARF5 details
and give access to line header
information even if there are no lines
in a particular compilation unit's line
table.
.P



.H 3 "Get A Set of Lines (including skeleton line tables)"
This set of functions works on any DWARF version.
DWARF2,3,4,5 and the DWARF4 based experimental
two-level line tables are all supported.
What was once done by dwarf_srclines() alone
is now done with two calls as described here.
.P
The interfaces support reading GNU two-level line tables.
The format of such tables is a topic beyond
the scope of this document.
.P

.H 3 "dwarf_srclines_b()"
This is the
.DS
\f(CWint dwarf_srclines_b(
        Dwarf_Die die,
        Dwarf_Unsigned *version_out,
        Dwarf_Bool     *is_single_table,
        Dwarf_Line_Context *context_out,
        Dwarf_Error *error)\fP
.DE
\f(CWdwarf_srclines_b()\fP
takes a single argument as input,
a pointer to a compilation-unit (CU)  DIE.
The other arguments are used to return values
to the caller.
On success
\f(CWDW_DLV_OK\fP
is returned and values
are returned through the pointers.
If there is no line table
\f(CWDW_DLV_NO_ENTRY\fP
is returned
and no values are returned though the pointers.
If
\f(CWDW_DLV_ERROR\fP
is returned the
involved is returned through the
\f(CWerror\fP
pointer.
.P
The values returned on success are:
.P
\f(CW*version_out()\fP
is set to the version number from
the line table header for this CU.
The experimental two-level line table
value is 0xf006.  Standard numbers
are 2,3,4 and 5.
.P
\f(CW*is_single_table()\fP
is set to non-zero if the line table
is an ordinary single line table.
If the line table is anything else
(either a line table header with no lines
or an experimental two-level line table)
it is set to zero.
.P
\f(CW*context_out()\fP
is set to an opaque pointer to a
\f(CWDwarf_Line_Context\fP
record
which in turn is used to get
other data from this line table.
See below.
.P
See
\f(CW*dwarf_srclines_dealloc_b()\fP
for examples showing correct use.


.H 3 "dwarf_get_line_section_name_from_die()"
.DS
\f(CWint dwarf_get_line_section_name_from_die(
        Dwarf_Die die,
        const char ** sec_name,
        Dwarf_Error *error)\fP
.DE
\f(CW*dwarf_get_line_section_name_from_die()\fP
retrieves the object file section name of
the applicable line section.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.

.H 3 "dwarf_srclines_from_linecontext()"
.DS
\f(CWint dwarf_srclines_from_linecontext(
        Dwarf_Line_Context line_context,
        Dwarf_Line ** linebuf,
        Dwarf_Signed *linecount,
        Dwarf_Error *error)\fP
.DE
\f(CW*dwarf_srclines_from_linecontext()\fP
gives access to the line tables.
On success it returns
\f(CWDW_DLV_OK\fP
and passes back line tables
through the pointers.
.P
Though
\f(CWDW_DLV_OK\fP
will not be returned callers should
assume it is possible.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and the error code set through
the
\f(CWerror\fP
pointer.
.P
On success:
.P
\f(CW*linebuf\fP
is set to an array of Dwarf_Line pointers.
.P
\f(CW*linecount\fP is set
to the number of pointers in the array.
.P

.H 3 "dwarf_srclines_two_levelfrom_linecontext()"
.DS
\f(CWint dwarf_srclines_from_linecontext(
        Dwarf_Line_Context line_context,
        Dwarf_Line ** linebuf,
        Dwarf_Signed *linecount,
        Dwarf_Line ** linebuf_actuals,
        Dwarf_Signed *linecount_actuals,
        Dwarf_Error *error)\fP
.DE
\f(CW*dwarf_srclines_two_levelfrom_linecontext()\fP
gives access to the line tables.
On success it returns
\f(CWDW_DLV_OK\fP
and passes back line tables
through the pointers.
.P
Though
\f(CWDW_DLV_OK\fP
will not be returned callers should
assume it is possible.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and the error code set through
the
\f(CWerror\fP
pointer.
.P
On success:
.P
\f(CW*linebuf\fP
is set to an array of Dwarf_Line pointers.
.P
\f(CW*linecount\fP is set
to the number of pointers in the array.
.P
If one is not intending that the experimental two-level
line tables are of interest then pass NULL
for
\f(CW*linebuf_actuals\fP
and
\f(CW*linecount_actuals\fP.
The NULL pointers notify the library
that the second table is not to be passed back.
.P
If a line table is actually a two-level tables
\f(CW*linebuf\fP is set to point to an array of
Logicals lines.
\f(CW*linecount\fP is set to the number of Logicals.
\f(CW*linebuf_actals\fP is set to point to an array of
Actuals lines.
\f(CW*linecount_actuals\fP is set to the number of Actuals.

.H 3 "dwarf_srclines_dealloc_b()"
.DS
\f(CWvoid dwarf_srclines_dealloc_b(
        Dwarf_Line_Context line_context,
        Dwarf_Error *error)\fP
.DE
This does a complete deallocation of
the memory of the
\f(CWDwarf_Line_Context\fP
and the
\f(CWDwarf_Line\fP array (or arrays)
that came from the
\f(CWDwarf_Line_Context\fP.
On return you should set any local pointers
to these buffers to NULL as a reminder
that any use of the local pointers would
be to stale memory.

.in +2
.FG "Examplec dwarf_srclines_b()"
.DS
\f(CW
void examplec(Dwarf_Die cu_die)
{
    /* EXAMPLE: DWARF5 style access.  */
    Dwarf_Line  *linebuf = 0;
    Dwarf_Signed linecount = 0;
    Dwarf_Line  *linebuf_actuals = 0;
    Dwarf_Signed linecount_actuals = 0;
    Dwarf_Line_Context line_context = 0;
    Dwarf_Signed linecount_total = 0;
    Dwarf_Small  table_count = 0;
    Dwarf_Unsigned lineversion = 0;
    Dwarf_Error err = 0;
    int sres = 0;
    /* ... */
    /*  we use 'return' here to signify we can do nothing more
        at this point in the code. */
    sres = dwarf_srclines_b(cu_die,&lineversion,
        &table_count,&line_context,&err);
    if (sres != DW_DLV_OK) {
        /*  Handle the DW_DLV_NO_ENTRY  or DW_DLV_ERROR
            No memory was allocated so there nothing
            to dealloc. */
        return;
    }
    if (table_count == 0) {
        /*  A line table with no actual lines.  */
        /*...do something, see dwarf_srclines_files_count()
            etc below. */

        dwarf_srclines_dealloc_b(line_context);
        /*  All the memory is released, the line_context
            and linebuf zeroed now
            as a reminder they are stale. */
        linebuf = 0;
        line_context = 0;
    } else if (table_count == 1) {
        Dwarf_Signed i = 0;
        Dwarf_Signed baseindex = 0;
        Dwarf_Signed file_count = 0;
        Dwarf_Signed endindex = 0;
        /*  Standard dwarf 2,3,4, or 5 line table */
        /*  Do something. */

        /*  First let us index through all the files listed
            in the line table header. */
        sres = dwarf_srclines_files_indexes(line_context,
            &baseindex,&file_count,&endindex,&err);
        if (sres != DW_DLV_OK) {
            /* Something badly wrong! */
            return;
        }
        /*  Works for DWARF2,3,4 (one-based index)
            and DWARF5 (zero-based index) */
        for (i = baseindex; i < endindex; i++) {
            Dwarf_Unsigned dirindex = 0;
            Dwarf_Unsigned modtime = 0;
            Dwarf_Unsigned flength = 0;
            Dwarf_Form_Data16 *md5data = 0;
            int vres = 0;
            const char *name = 0;

            vres = dwarf_srclines_files_data_b(line_context,i,
                &name,&dirindex, &modtime,&flength,
                &md5data,&err);
            if (vres != DW_DLV_OK) {
                /* something very wrong. */
                return;
            }
            /* Do something. */
        }


        /*  For this case where we have a line table we will likely
            wish to get the line details: */
        sres = dwarf_srclines_from_linecontext(line_context,
            &linebuf,&linecount,
            &err);
        if (sres != DW_DLV_OK) {
            /* Error. Clean up the context information. */
            dwarf_srclines_dealloc_b(line_context);
            return;
        }
        /* The lines are normal line table lines. */
        for (i = 0; i < linecount; ++i) {
            /* use linebuf[i] */
        }
        dwarf_srclines_dealloc_b(line_context);
        /*  All the memory is released, the line_context
            and linebuf zeroed now as a reminder they are stale */
        linebuf = 0;
        line_context = 0;
        linecount = 0;
    } else {
        Dwarf_Signed i = 0;
        /*  ASSERT: table_count == 2,
            Experimental two-level line table. Version 0xf006
            We do not define the meaning of this non-standard
            set of tables here. */

        /*  For 'something C' (two-level line tables)
            one codes something like this
            Note that we do not define the meaning or use of two-level line
            tables as these are experimental, not standard DWARF. */
        sres = dwarf_srclines_two_level_from_linecontext(line_context,
            &linebuf,&linecount,
            &linebuf_actuals,&linecount_actuals,
            &err);
        if (sres == DW_DLV_OK) {
            for (i = 0; i < linecount; ++i) {
                /* use linebuf[i], these are the 'logicals' entries. */
            }
            for (i = 0; i < linecount_actuals; ++i) {
                /* use linebuf_actuals[i], these are the actuals entries */
            }
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        } else if (sres == DW_DLV_NO_ENTRY) {
            /* This should be impossible, but do something.   */
            /* Then Free the line_context */
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        } else {
            /*  ERROR, show the error or something.
                Free the line_context. */
            dwarf_srclines_dealloc_b(line_context);
            line_context = 0;
            linebuf = 0;
            linecount = 0;
            linebuf_actuals = 0;
            linecount_actuals = 0;
        }
    }
}
\fP
.DE
.in -2

.H 2 "Line Context Details (DWARF5 style)"
New in October 2015.
When a
\f(CW
Dwarf_Line_Context
\fP
has been returned by
\f(CWdwarf_srclines_b()\fP
that line context data's details can be retrieved
with the following set of calls.
.H 3 "dwarf_srclines_table_offset()"
.DS
\f(CW
int dwarf_srclines_table_offset(Dwarf_Line_Context line_context,
    Dwarf_Unsigned * offset,
    Dwarf_Error    * error);
\fP
.DE
On success, this function returns the offset (in the object file
line section) of the actual line data (i.e. after the line
header for this compilation unit) through the
\f(CWoffset\fP
pointer.
The offset is probably only of interest when
printing detailed information about a line table header.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.

.H 3 "dwarf_srclines_version()"
.DS
\f(CW
int dwarf_srclines_version(Dwarf_Line_Context line_context,
    Dwarf_Unsigned * version,
    Dwarf_Error    * error);
\fP
.DE
On success
\f(CWDW_DLV_OK\fP
is returned and the line table version number is returned
through  the
\f(CWversion\fP pointer.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.


.H 3 "dwarf_srclines_comp_dir()"
.DS
\f(CW
int dwarf_srclines_comp_dir(Dwarf_Line_Context line_context,
    const char ** compilation_directory,
    Dwarf_Error * error);
\fP
.DE
On success this returns a pointer to
the compilation directory string for this
line table in
\f(CW*compilation_directory\fP.
That compilation string may be NULL or the empty
string.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.

.H 3 "dwarf_srclines_files_indexes()"
.DS
\f(CW
int dwarf_srclines_files_indexes(Dwarf_Line_Context line_context,
    Dwarf_Signed  *  baseindex,
    Dwarf_Signed  *  count,
    Dwarf_Signed  *  endindex,
    Dwarf_Error   *  error);
\fP
.DE
With DWARF5 the base file number index in the
line table changed from zero (DWARF2,3,4)
to one (DWARF5).
Which meant iterating through the valid source file
indexes became messy if one used the older
\f(CWdwarf_srclines_files_count()\fP
function (zero-based and one-based indexing
being incompatible).
See Figure  "Examplec dwarf_srclines_b()"
above
for use of this function in accessing file names.
.P
The base index of  files in the files list of
a line table header will be returned through
\f(CWbaseindex\fP.
.P
The number of files in the files list of
a line table header will be returned through
\f(CWcount\fP.
.P
The end index of  files in the files list of
a line table header will be returned through
\f(CWendindex\fP.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.



.H 3 "dwarf_srclines_files_count()"
.DS
\f(CW
int dwarf_srclines_files_count(Dwarf_Line_Context line_context,
    Dwarf_Signed  *  count,
    Dwarf_Error   *  error);
\fP
.DE
On success, the number of files in the files list of
a line table header will be returned through
\f(CWcount\fP.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.


.H 3 "dwarf_srclines_files_data_b()"
This supplants
\f(CWdwarf_srclines_files_data()\fP
as of March 2018
to allow access to the md5 value in DWARF5.
The function
\f(CWdwarf_srclines_files_data()\fP
continues to be supported.
.DS
\f(CW
int dwarf_srclines_files_data_b(Dwarf_Line_Context line_context,
    Dwarf_Signed     index,
    const char **    name,
    Dwarf_Unsigned * directory_index,
    Dwarf_Unsigned * last_mod_time,
    Dwarf_Unsigned * file_length,
    Dwarf_Form_Data16 ** md5_value,
    Dwarf_Error    * error);
\fP
.DE
On success, data about a single file in
the files list will be returned through the pointers.
See DWARF documentation for the meaning of these
fields.
\f(CWcount\fP.
Valid
\f(CWindex\fP.
values are 1 through
\f(CWcount\fP,
reflecting the way the table is defined by DWARF2,3,4.
For a dwarf5 line table index values 0...count-1 are legal.
This is certainly awkward.
.P
If
\f(CWmd5_value\fP
is non-null it is used to pass a back
a pointer to a
\f(CWDwarf_Form_Data16\fP md5 value if
the md5 value is present. Otherwise
a zero value is passed back to indicate there
was no such field.
The 16-byte value pointed to is inside
the line_context, so if you want to keep
the value you should probably copy it
to storage you control.
.P
This returns the raw files data from the
line table header.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.


.H 3 "dwarf_srclines_files_data()"
This interface was created in October 2015.
It cannot return the DWARF5 MD5 value.
See the newer dwarf_srclines_files_data_b().
.DS
\f(CW
int dwarf_srclines_files_data(Dwarf_Line_Context line_context,
    Dwarf_Signed     index,
    const char **    name,
    Dwarf_Unsigned * directory_index,
    Dwarf_Unsigned * last_mod_time,
    Dwarf_Unsigned * file_length,
    Dwarf_Error    * error);
\fP
.DE
On success, data about a single file in
the files list will be returned through the pointers.
See DWARF documentation for the meaning of these
fields.
\f(CWcount\fP.
Valid
\f(CWindex\fP.
values are 1 through
\f(CWcount\fP,
reflecting the way the table is defined by DWARF2,3,4.
For a dwarf5 line table index values 0...count-1 are legal.
This is certainly awkward.
.P
This returns the raw files data from the
line table header.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.


.H 3 "dwarf_srclines_include_dir_count()"
.DS
\f(CW
int dwarf_srclines_include_dir_count(Dwarf_Line_Context line_context,
    Dwarf_Signed * count,
    Dwarf_Error  * error);
\fP
.DE
On success, the number of files in the includes list of
a line table header will be returned through
\f(CWcount\fP.
.P
Valid
\f(CWindex\fP.
values are 1 through
\f(CWcount\fP,
reflecting the way the table is defined by DWARF 2,3 and 4.
For a dwarf5 line table index values 0...count-1 are legal.
This is certainly awkward.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.
.H 3 "dwarf_srclines_include_dir_data()"
.DS
\f(CW
int dwarf_srclines_include_dir_data(Dwarf_Line_Context line_context,
    Dwarf_Signed    index,
    const char **   name,
    Dwarf_Error  *  error);
\fP
.DE
On success, data about a single file in
the include files list will be returned through the pointers.
See DWARF documentation for the meaning of these
fields.
.P
Valid
\f(CWindex\fP.
values are 1 through
\f(CWcount\fP,
reflecting the way the table is defined by DWARF.
.P
In case of error,
\f(CWDW_DLV_ERROR\fP
is returned and the error is set through
the
\f(CWerror\fP
pointer.
\f(CWDW_DLV_NO_ENTRY\fP
will not be returned.
.H 3 "dwarf_srclines_subprog_count()"
\f(CW
int dwarf_srclines_subprog_count(Dwarf_Line_Context line_context,
    Dwarf_Signed * count,
    Dwarf_Error  * error);
\fP
This is only useful with experimental two-level line tables.
.H 3 "dwarf_srclines_subprog_data()"
\f(CW
int dwarf_srclines_subprog_data(Dwarf_Line_Context line_context,
    Dwarf_Signed     index,
    const char **    name,
    Dwarf_Unsigned * decl_file,
    Dwarf_Unsigned * decl_line,
    Dwarf_Error   *  error);
\fP
This is only useful with experimental two-level line tables.

.H 2 "Get A Set of Lines (DWARF2,3,4 style)"
The function returns information about every source line for a
particular compilation-unit.
The compilation-unit is specified
by the corresponding die.
It does not support line tables with no lines very well
nor does it support experimental two-level linetables.
.H 3 "dwarf_srclines()"
.DS
\f(CWint dwarf_srclines(
        Dwarf_Die die,
        Dwarf_Line **linebuf,
	Dwarf_Signed *linecount,
        Dwarf_Error *error)\fP
.DE
This function is not useful for DWARF5 skeleton line tables nor
for two-level line tables.
It works for DWARF2,3,4,5 ordinary single line tables.
The function \f(CWdwarf_srclines()\fP places all line number descriptors
for a single compilation unit into a single block, sets \f(CW*linebuf\fP
to point to that block,
sets \f(CW*linecount\fP to the number of descriptors in this block
and returns \f(CWDW_DLV_OK\fP.
.in +2
.P
To get a more detailed view of the contents of a dwarf line
table header see \f(CWdwarf_srclines_b()\fP and
the routines that use the Dwarf_Line_Context
information, such as \f(CWdwarf_srcfiles_comp_dir()\fP,
\f(CWdwarf_srclines_files_count()\fP,
\f(CWdwarf_srclines_include_dir_count()\fP
and similar functions.

.in -2

.P
The compilation-unit is indicated by the given \f(CWdie\fP which must be
a compilation-unit die.
It returns \f(CWDW_DLV_ERROR\fP on error.
On
successful return, line number information
should be freed using \f(CWdwarf_srclines_dealloc()\fP
when no longer of interest.
.P
.in +2
.FG "Exampled dwarf_srclines()"
.DS
\f(CW
/*  dwarf_srclines_b() should be used instead. */
void exampled(Dwarf_Debug dbg,Dwarf_Die somedie)
{
    Dwarf_Signed count = 0;
    Dwarf_Line *linebuf = 0;
    Dwarf_Signed i = 0;
    Dwarf_Error error = 0;
    int sres = 0;

    sres = dwarf_srclines(somedie, &linebuf,&count, &error);
    if (sres == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use linebuf[i] */
        }
        dwarf_srclines_dealloc(dbg, linebuf, count);
    }
}
\fP
.DE

.in -2
.P
An alternative using \f(CWdwarf_dealloc()\fP directly
is no longer (as of 2015) described here. It works as well
as ever, but it has been obsolete since 2005.
still works, but does not completely free all data allocated.
The \f(CWdwarf_srclines_dealloc()\fP routine was created
to fix the problem of incomplete deallocation.

.H 2 "Get the set of Source File Names"

The function returns the names of the source files that have contributed
to the compilation-unit represented by the given DIE.  Only the source
files named in the statement program prologue are returned.
.H 3 dwarf_srcfiles()
This works for for all line tables.
.DS
\f(CWint dwarf_srcfiles(
        Dwarf_Die die,
        char ***srcfiles,
        Dwarf_Signed *srccount,
        Dwarf_Error *error)\fP
.DE
When it succeeds
\f(CWdwarf_srcfiles()\fP returns
\f(CWDW_DLV_OK\fP
and puts
the number of source
files named in the statement program prologue indicated by the given
\f(CWdie\fP
into \f(CW*srccount\fP.
Source files defined in the statement program are ignored.
The given \f(CWdie\fP should have the tag
\f(CWDW_TAG_compile_unit\fP,
\f(CWDW_TAG_partial_unit\fP,
or \f(CWDW_TAG_type_unit\fP
.
The location pointed to by \f(CWsrcfiles\fP is set to point to a list
of pointers to null-terminated strings that name the source
files.
.P
On a successful return from  \f(CWdwarf_srcfiles()\fP each of the
strings returned should be individually freed using \f(CWdwarf_dealloc()\fP
with the allocation type \f(CWDW_DLA_STRING\fP when no longer of
interest.
This should be followed by free-ing the list using
\f(CWdwarf_dealloc()\fP with the allocation type \f(CWDW_DLA_LIST\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP
if there is no
corresponding statement program (i.e., if there is no line information).
.in +2
.FG "Exampled dwarf_srcfiles()"
.DS
\f(CW
void examplee(Dwarf_Debug dbg,Dwarf_Die somedie)
{
    Dwarf_Signed count = 0;
    char **srcfiles = 0;
    Dwarf_Signed i = 0;
    Dwarf_Error error = 0;
    int res = 0;

    res = dwarf_srcfiles(somedie, &srcfiles,&count,&error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use srcfiles[i] */
            dwarf_dealloc(dbg, srcfiles[i], DW_DLA_STRING);
        }
        dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2
.H 2 "Get Information About a Single Line Table Line"
The following functions can be used on the \f(CWDwarf_Line\fP descriptors
returned by
\f(CWdwarf_srclines()\fP
or
\f(CWdwarf_srclines_from_linecontext()\fP
to obtain information about the
source lines.

.H 3 "dwarf_linebeginstatement()"
.DS
\f(CWint dwarf_linebeginstatement(
        Dwarf_Line line,
	Dwarf_Bool *return_bool,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_linebeginstatement()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_bool\fP
to
\fInon-zero\fP
(if \f(CWline\fP represents a line number entry that is marked as
beginning a statement).
or
\fIzero\fP ((if \f(CWline\fP represents a line number entry
that is not marked as beginning a statement).
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.P
.H 3 "dwarf_lineendsequence()"
.DS
\f(CWint dwarf_lineendsequence(
	Dwarf_Line line,
	Dwarf_Bool *return_bool,
	Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_lineendsequence()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_bool\fP
\fInon-zero\fP
(in which case
\f(CWline\fP represents a line number entry that is marked as
ending a text sequence)
or
\fIzero\fP (in which case
\f(CWline\fP represents a line number entry
that is not marked as ending a text sequence).
A line number entry that is marked as
ending a text sequence is an entry with an address
one beyond the highest address used by the current
sequence of line table entries (that is, the table entry is
a DW_LNE_end_sequence entry (see the DWARF specification)).
.P
The function \f(CWdwarf_lineendsequence()\fP
returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.P
.H 3 "dwarf_lineno()"
.DS
\f(CWint dwarf_lineno(
        Dwarf_Line       line,
	Dwarf_Unsigned * returned_lineno,
        Dwarf_Error    * error)\fP
.DE
The function \f(CWdwarf_lineno()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_lineno\fP to
the source statement line
number corresponding to the descriptor \f(CWline\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.P
.H 3 "dwarf_line_srcfileno()"
.DS
\f(CWint dwarf_line_srcfileno(
        Dwarf_Line       line,
	Dwarf_Unsigned * returned_fileno,
        Dwarf_Error    * error)\fP
.DE
The function \f(CWdwarf_line_srcfileno()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_fileno\fP to
the source statement line
number corresponding to the descriptor \f(CWfile number\fP.
When the number returned through \f(CW*returned_fileno\fP is zero it means
the file name is unknown (see the DWARF2/3 line table specification).
When the number returned through \f(CW*returned_fileno\fP is non-zero
it is a file number:
subtract 1 from this file number
to get an
index into the array of strings returned by \f(CWdwarf_srcfiles()\fP
(verify the resulting index is in range for the array of strings
before indexing into the array of strings).
The file number may exceed the size of
the array of strings returned by \f(CWdwarf_srcfiles()\fP
because \f(CWdwarf_srcfiles()\fP does not return files names defined with
the  \f(CWDW_DLE_define_file\fP  operator.
The function \f(CWdwarf_line_srcfileno()\fP returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.P
.H 3 "dwarf_lineaddr()"
.DS
\f(CWint dwarf_lineaddr(
        Dwarf_Line   line,
	Dwarf_Addr  *return_lineaddr,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_lineaddr()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_lineaddr\fP to
the address associated
with the descriptor \f(CWline\fP.
It returns \f(CWDW_DLV_ERROR\fP  on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.P
.H 3 "dwarf_lineoff()"
.DS
\f(CWint dwarf_lineoff(
        Dwarf_Line line,
	Dwarf_Signed   * return_lineoff,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_lineoff()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_lineoff\fP to
the column number at which
the statement represented by \f(CWline\fP begins.
.P
It sets \f(CWreturn_lineoff\fP to zero
if the column number of the statement is not represented
(meaning the producer library call was given zero
as the column number).  Zero is the correct value meaning "left edge"
as defined in the DWARF2/3/4 specication (section 6.2.2).
.P
Before December 2011 zero was not returned through
the  \f(CWreturn_lineoff\fP pointer, -1 was returned through the pointer.
The reason for this oddity is unclear, lost in history.
But there is no good reason for -1.
.P
The type of  \f(CWreturn_lineoff\fP is a pointer-to-signed, but there
is no good reason for the value to be signed, the DWARF specification
does not deal with negative column numbers.  However, changing the
declaration would cause compilation errors for little benefit, so
the pointer-to-signed is left unchanged.
.P
On error it returns \f(CWDW_DLV_ERROR\fP.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
.H 3 "dwarf_lineoff_b()"
.DS
\f(CWint dwarf_lineoff_b(
        Dwarf_Line line,
	Dwarf_Unsigned   * return_lineoff,
        Dwarf_Error *error)\fP
.DE
The function
\f(CWdwarf_lineoff_b()\fP
returns
exactly the same as
\f(CWdwarf_lineoff()\fP
except the line offset returned
through
\f(CWreturn_lineoff()\fP
is an unsigned value.
The signed return offset never made much sense
but was harmless since line lengths are limited
by most language standards.

.H 3 "dwarf_linesrc()"
.DS
\f(CWint dwarf_linesrc(
        Dwarf_Line line,
	char  **   return_linesrc,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_linesrc()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_linesrc\fP to
a pointer to a
null-terminated string of characters that represents the name of the
source-file where \f(CWline\fP occurs.
It returns \f(CWDW_DLV_ERROR\fP on
error.
.P
If the applicable file name in the line table Statement Program Prolog
does not start with a '/' character
the string in \f(CWDW_AT_comp_dir\fP (if applicable and present)
or the applicable
directory name from the line Statement Program Prolog
is prepended to the
file name in the line table Statement Program Prolog
to make a full path.
.P
The storage pointed to by a successful return of
\f(CWdwarf_linesrc()\fP should be freed using \f(CWdwarf_dealloc()\fP with
the allocation type \f(CWDW_DLA_STRING\fP when no longer of interest.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_lineblock()"
.DS
\f(CWint dwarf_lineblock(
        Dwarf_Line line,
	Dwarf_Bool *return_bool,
        Dwarf_Error *error)\fP
.DE
The function
\f(CWdwarf_lineblock()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_linesrc\fP to
non-zero (i.e. true)(if the line is marked as
beginning a basic block)
or zero (i.e. false) (if the line is marked as not
beginning a basic block).
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_is_addr_set()"
.DS
\f(CWint dwarf_line_is_addr_set(
        Dwarf_Line line,
	Dwarf_Bool *return_bool,
        Dwarf_Error *error)\fP
.DE
The function
\f(CWdwarf_line_is_addr_set()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_bool\fP to
non-zero (i.e. true)(if the line is marked as
being a DW_LNE_set_address operation)
or zero (i.e. false) (if the line is marked as not
being a DW_LNE_set_address operation).
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

This is intended to allow consumers to do a more useful job
printing and analyzing DWARF data, it is not strictly
necessary.

.H 3 "dwarf_prologue_end_etc()"
.DS
\f(CWint dwarf_prologue_end_etc(Dwarf_Line  line,
        Dwarf_Bool  *    prologue_end,
        Dwarf_Bool  *    epilogue_begin,
        Dwarf_Unsigned * isa,
        Dwarf_Unsigned * discriminator,
        Dwarf_Error *    error)\fP
.DE
The function
\f(CWdwarf_prologue_end_etc()\fP returns
\f(CWDW_DLV_OK\fP and sets  the returned fields to
values currently set.
While it is pretty safe to assume that the
\f(CWisa\fP
and
\f(CWdiscriminator\fP
values returned are very small integers, there is
no restriction in the standard.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

This function is new in December 2011.


.H 2 "Global Name Space Operations"
These operations operate on the .debug_pubnames section of the debugging
information.

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_globals()"
.DS
\f(CWint dwarf_get_globals(
        Dwarf_Debug dbg,
        Dwarf_Global **globals,
        Dwarf_Signed * return_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_globals()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_count\fP to
the count of pubnames
represented in the section containing pubnames i.e. .debug_pubnames.
It also stores at \f(CW*globals\fP, a pointer
to a list of \f(CWDwarf_Global\fP descriptors, one for each of the
pubnames in the .debug_pubnames section.
The returned results are for the entire section.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if the .debug_pubnames
section does not exist.

.P
On a successful return from
\f(CWdwarf_get_globals()\fP, the \f(CWDwarf_Global\fP
descriptors should be
freed using \f(CWdwarf_globals_dealloc()\fP.
\f(CWdwarf_globals_dealloc()\fP is new as of July 15, 2005
and is the preferred approach to freeing this memory..
.P
Global names refer exclusively to names and offsets
in the .debug_info section.
See section 6.1.1 "Lookup by Name" in the dwarf standard.

.in +2
.FG "Exampled dwarf_get_globals()"
.DS
\f(CW
void examplef(Dwarf_Debug dbg)
{
    Dwarf_Signed count = 0;
    Dwarf_Global *globs = 0;
    Dwarf_Signed i = 0;
    Dwarf_Error  error = 0;
    int res = 0;

    res = dwarf_get_globals(dbg, &globs,&count, &error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use globs[i] */
        }
        dwarf_globals_dealloc(dbg, globs, count);
    }
}
\fP
.DE
.in -2
.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
On a successful return from
\f(CWdwarf_get_globals()\fP, the \f(CWDwarf_Global\fP
descriptors should be individually
freed using \f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_GLOBAL_CONTEXT\fP,
(or
\f(CWDW_DLA_GLOBAL\fP, an older name, supported for compatibility)
followed by the deallocation of the list itself
with the allocation type \f(CWDW_DLA_LIST\fP when the descriptors are
no longer of interest.

.in +2
.DS
\f(CWDwarf_Signed cnt;
Dwarf_Global *globs;
int res;

res = dwarf_get_globals(dbg, &globs,&cnt, &error);
if (res == DW_DLV_OK) {

        /* OBSOLETE: DO NOT USE to deallocate*/
        for (i = 0; i < cnt; ++i) {
                /* use globs[i] */
                dwarf_dealloc(dbg, globs[i], DW_DLA_GLOBAL_CONTEXT);
        }
        dwarf_dealloc(dbg, globs, DW_DLA_LIST);
}\fP
.DE
.in -2

.H 4 "dwarf_globname()"
.DS
\f(CWint dwarf_globname(
        Dwarf_Global global,
        char **      return_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_globname()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to a
null-terminated string that names the pubname represented by the
\f(CWDwarf_Global\fP descriptor, \f(CWglobal\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_global_die_offset()"
.DS
\f(CWint dwarf_global_die_offset(
        Dwarf_Global global,
	Dwarf_Off   *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_global_die_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the pubname that is described by the \f(CWDwarf_Global\fP descriptor,
\f(CWglob\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_global_cu_offset()"
.DS
\f(CWint dwarf_global_cu_offset(
        Dwarf_Global global,
	Dwarf_Off   *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_global_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the compilation-unit that contains the pubname described
by the \f(CWDwarf_Global\fP descriptor, \f(CWglobal\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_get_cu_die_offset_given_cu_header_offset()"
.DS
\f(CWint dwarf_get_cu_die_offset_given_cu_header_offset_b(
	Dwarf_Debug dbg,
	Dwarf_Off   in_cu_header_offset,
        Dwarf_Bool  is_info,
        Dwarf_Off * out_cu_die_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_cu_die_offset_given_cu_header_offset()\fP
returns
\f(CWDW_DLV_OK\fP and sets \f(CW*out_cu_die_offset\fP to
the offset of the compilation-unit DIE given the
offset \f(CWin_cu_header_offset\fP of a compilation-unit header.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
.P
If \f(CWis_info\fP is non-zero the \f(CWin_cu_header_offset\fP must refer
to a .debug_info section offset.
If \f(CWis_info\fP zero the \f(CWin_cu_header_offset\fP must refer
to a .debug_types section offset.
Chaos may result if the \f(CWis_info\fP flag is incorrect.

This effectively turns a compilation-unit-header offset
into a compilation-unit DIE offset (by adding the
size of the applicable CU header).
This function is also sometimes useful with the
\f(CWdwarf_weak_cu_offset()\fP,
\f(CWdwarf_func_cu_offset()\fP,
\f(CWdwarf_type_cu_offset()\fP,
and
\f(CWint dwarf_var_cu_offset()\fP
functions, though for those functions the data is
only in .debug_info by definition.

.H 4 "dwarf_get_cu_die_offset_given_cu_header_offset()"
.DS
\f(CWint dwarf_get_cu_die_offset_given_cu_header_offset(
	Dwarf_Debug dbg,
	Dwarf_Off   in_cu_header_offset,
        Dwarf_Off * out_cu_die_offset,
        Dwarf_Error *error)\fP
.DE
This function is superseded by
\f(CWdwarf_get_cu_die_offset_given_cu_header_offset_b()\fP,
a function which is still supported thought it refers only
to the .debug_info section.


\f(CWdwarf_get_cu_die_offset_given_cu_header_offset()\fP
added Rev 1.45, June, 2001.

This function is declared as 'optional' in libdwarf.h
on IRIX systems so the _MIPS_SYMBOL_PRESENT
predicate may be used at run time to determine if the version of
libdwarf linked into an application has this function.

.H 4 "dwarf_global_name_offsets()"
.DS
\f(CWint dwarf_global_name_offsets(
        Dwarf_Global global,
        char     **return_name,
        Dwarf_Off *die_offset,
        Dwarf_Off *cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_global_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to
a null-terminated string that gives the name of the pubname
described by the \f(CWDwarf_Global\fP descriptor \f(CWglobal\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP,
the offset of the DIE representing the pubname,
and
the offset of the DIE representing the compilation-unit
containing the
pubname, respectively.
On a
successful return from \f(CWdwarf_global_name_offsets()\fP the storage
pointed to by \f(CWreturn_name\fP
should be freed using \f(CWdwarf_dealloc()\fP,
with the allocation type \f(CWDW_DLA_STRING\fP when no longer of interest.


.H 2 "DWARF3 Type Names Operations"
Section ".debug_pubtypes" is new in DWARF3.
.P
These functions operate on the .debug_pubtypes section of the debugging
information.  The .debug_pubtypes section contains the names of file-scope
user-defined types, the offsets of the \f(CWDIE\fPs that represent the
definitions of those types, and the offsets of the compilation-units
that contain the definitions of those types.

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_pubtypes()"
.DS
\f(CWint dwarf_get_pubtypes(
        Dwarf_Debug dbg,
        Dwarf_Type **types,
        Dwarf_Signed *typecount,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_pubtypes()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*typecount\fP to
the count of user-defined
type names represented in the section containing user-defined type names,
i.e. .debug_pubtypes.
It also stores at \f(CW*types\fP,
a pointer to a list of \f(CWDwarf_Type\fP descriptors, one for each of the
user-defined type names in the .debug_pubtypes section.
The returned results are for the entire section.
It returns \f(CWDW_DLV_NOCOUNT\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if
the .debug_pubtypes section does not exist.

.P
On a successful
return from \f(CWdwarf_get_pubtypes()\fP,
the \f(CWDwarf_Type\fP descriptors should be
freed using \f(CWdwarf_types_dealloc()\fP.
\f(CWdwarf_types_dealloc()\fP is used for both
\f(CWdwarf_get_pubtypes()\fP and \f(CWdwarf_get_types()\fP
as the data types are the same.
.P
Global type names refer exclusively to names and offsets
in the .debug_info section.
See section 6.1.1 "Lookup by Name" in the dwarf standard.


.in +2
.FG "Exampled dwarf_get_pubtypes()"
.DS
\f(CW
Avoid exampleg(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Type *types = 0;
    Dwarf_Signed i = 0;
    int res = 0;

    res = dwarf_get_pubtypes(dbg, &types,&count, &error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use types[i] */
        }
        dwarf_types_dealloc(dbg, types, count);
    }
}
\fP
.DE
.in -2

.H 4 "dwarf_pubtypename()"
.DS
\f(CWint dwarf_pubtypename(
        Dwarf_Type   type,
        char       **return_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_pubtypename()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to a
null-terminated string that names the user-defined type represented by the
\f(CWDwarf_Type\fP descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.

.H 4 "dwarf_pubtype_type_die_offset()"
.DS
\f(CWint dwarf_pubtype_type_die_offset(
        Dwarf_Type type,
        Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_pubtype_type_die_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the user-defined type that is described by the \f(CWDwarf_Type\fP
descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_pubtype_cu_offset()"
.DS
\f(CWint dwarf_pubtype_cu_offset(
        Dwarf_Type type,
        Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_pubtype_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the compilation-unit that contains the user-defined type
described by the \f(CWDwarf_Type\fP descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_pubtype_name_offsets()"
.DS
\f(CWint dwarf_pubtype_name_offsets(
        Dwarf_Type   type,
        char      ** returned_name,
        Dwarf_Off *  die_offset,
        Dwarf_Off *  cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_pubtype_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_name\fP to
a pointer to
a null-terminated string that gives the name of the user-defined
type described by the \f(CWDwarf_Type\fP descriptor \f(CWtype\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP, the offsets
of the DIE representing the
user-defined type, and the DIE
representing the compilation-unit containing the
user-defined type, respectively.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from \f(CWdwarf_pubtype_name_offsets()\fP
the storage pointed to by \f(CWreturned_name\fP should
be freed using
\f(CWdwarf_dealloc()\fP, with the allocation type \f(CWDW_DLA_STRING\fP
when no longer of interest.


.H 2 "User Defined Static Variable Names Operations"
This section is SGI specific and is not part of standard DWARF version 2.
.P
These functions operate on the .debug_varnames section of the debugging
information.  The .debug_varnames section contains the names of file-scope
static variables, the offsets of the \f(CWDIE\fPs that represent the
definitions of those variables, and the offsets of the compilation-units
that contain the definitions of those variables.
.P


.H 2 "Weak Name Space Operations"
These operations operate on the .debug_weaknames section of the debugging
information.
.P
These operations are SGI specific, not part of standard DWARF.
.P

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_weaks()"
.DS
\f(CWint dwarf_get_weaks(
        Dwarf_Debug dbg,
        Dwarf_Weak **weaks,
	Dwarf_Signed *weak_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_weaks()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*weak_count\fP to
the count of weak names
represented in the section containing weak names i.e. .debug_weaknames.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if the section does not exist.
It also stores in \f(CW*weaks\fP, a pointer to
a list of \f(CWDwarf_Weak\fP descriptors, one for each of the weak names
in the .debug_weaknames section.
The returned results are for the entire section.

.P
On a successful return from this function,
the \f(CWDwarf_Weak\fP descriptors should be freed using
\f(CWdwarf_weaks_dealloc()\fP when the data is no longer of
interest.  \f(CWdwarf_weaks_dealloc()\fPis new as of July 15, 2005.

.in +2
.FG "Exampleh dwarf_get_weaks()"
.DS
\f(CW
void exampleh(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Weak *weaks = 0;
    Dwarf_Signed i = 0;
    int res = 0;

    res = dwarf_get_weaks(dbg, &weaks, &count, &error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use weaks[i] */
        }
        dwarf_weaks_dealloc(dbg, weaks, count);
    }
}
\fP
.DE
.in -2



.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
On a successful return from \f(CWdwarf_get_weaks()\fP
the \f(CWDwarf_Weak\fP descriptors should be individually freed using
\f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_WEAK_CONTEXT\fP,
(or
\f(CWDW_DLA_WEAK\fP, an older name, supported for compatibility)
followed by the deallocation of the list itself with the allocation type
\f(CWDW_DLA_LIST\fP when the descriptors are no longer of interest.

.in +2
.FG "Examplei dwarf_get_weaks() obsolete"
.DS
\f(CW
void examplei(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Weak *weaks = 0;
    Dwarf_Signed i = 0;
    int res = 0;

    /* Obsolete, see exampleh instead */
    res = dwarf_get_weaks(dbg, &weaks, &count, &error);
    if (res == DW_DLV_OK) {
        /*  OBSOLETE: do not use dealloc for this.
            See above */
        for (i = 0; i < count; ++i) {
            /* use weaks[i] */
            dwarf_dealloc(dbg, weaks[i], DW_DLA_WEAK);
        }
        dwarf_dealloc(dbg, weaks, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.H 4 "dwarf_weakname()"
.DS
\f(CWint dwarf_weakname(
        Dwarf_Weak weak,
	char    ** return_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_weakname()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to a null-terminated
string that names the weak name represented by the
\f(CWDwarf_Weak\fP descriptor, \f(CWweak\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.

.DS
\f(CWint dwarf_weak_die_offset(
        Dwarf_Weak weak,
	Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_weak_die_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the weak name that is described by the \f(CWDwarf_Weak\fP descriptor,
\f(CWweak\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_weak_cu_offset()"
.DS
\f(CWint dwarf_weak_cu_offset(
        Dwarf_Weak weak,
	Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_weak_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the compilation-unit that contains the weak name described
by the \f(CWDwarf_Weak\fP descriptor, \f(CWweak\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_weak_name_offsets()"
.DS
\f(CWint dwarf_weak_name_offsets(
        Dwarf_Weak weak,
	char **  weak_name,
        Dwarf_Off *die_offset,
        Dwarf_Off *cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_weak_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*weak_name\fP to
a pointer to
a null-terminated string that gives the name of the weak name
described by the \f(CWDwarf_Weak\fP descriptor \f(CWweak\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP, the offsets
of the DIE representing the
weakname, and the DIE
representing the compilation-unit containing the
weakname, respectively.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a
successful return from \f(CWdwarf_weak_name_offsets()\fP the storage
pointed to by \f(CWweak_name\fP
should be freed using \f(CWdwarf_dealloc()\fP,
with the allocation type \f(CWDW_DLA_STRING\fP when no longer of interest.

.H 2 "Static Function Names Operations"
This section is SGI specific and is not part of standard DWARF version 2.
.P
These function operate on the .debug_funcnames section of the debugging
information.  The .debug_funcnames section contains the names of static
functions defined in the object, the offsets of the \f(CWDIE\fPs that
represent the definitions of the corresponding functions, and the offsets
of the start of the compilation-units that contain the definitions of
those functions.

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_funcs()"
.DS
\f(CWint dwarf_get_funcs(
        Dwarf_Debug dbg,
        Dwarf_Func **funcs,
        Dwarf_Signed *func_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_funcs()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*func_count\fP to
the count of static
function names represented in the section containing static function
names, i.e. .debug_funcnames.
It also
stores, at \f(CW*funcs\fP, a pointer to a list of \f(CWDwarf_Func\fP
descriptors, one for each of the static functions in the .debug_funcnames
section.
The returned results are for the entire section.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP
if the .debug_funcnames section does not exist.
.P
On a successful return from \f(CWdwarf_get_funcs()\fP,
the \f(CWDwarf_Func\fP
descriptors should be freed using \f(CWdwarf_funcs_dealloc()\fP.
\f(CWdwarf_funcs_dealloc()\fP is new as of July 15, 2005.

.in +2
.FG "Examplej dwarf_get_funcs()"
.DS
\f(CW
void examplej(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Func *funcs = 0;
    Dwarf_Signed i = 0;
    int fres = 0;

    fres = dwarf_get_funcs(dbg, &funcs, &count, &error);
    if (fres == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use funcs[i] */
        }
        dwarf_funcs_dealloc(dbg, funcs, count);
    }
}
\fP
.DE
.in -2


.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
On a successful return from \f(CWdwarf_get_funcs()\fP,
the \f(CWDwarf_Func\fP
descriptors should be individually freed using \f(CWdwarf_dealloc()\fP
with the allocation type
\f(CWDW_DLA_FUNC_CONTEXT\fP,
(or
\f(CWDW_DLA_FUNC\fP, an older name, supported for compatibility)
followed by the deallocation
of the list itself with the allocation type \f(CWDW_DLA_LIST\fP when
the descriptors are no longer of interest.

.in +2
.FG "Examplek dwarf_get_funcs() obsolete"
.DS
\f(CW
void examplek(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Func *funcs = 0;
    Dwarf_Signed count = 0;
    Dwarf_Signed i = 0;
    int fres = 0;

    fres = dwarf_get_funcs(dbg, &funcs,&count, &error);
    if (fres == DW_DLV_OK) {
        /* OBSOLETE: see dwarf_funcs_dealloc() above */
        for (i = 0; i < count; ++i) {
            /* use funcs[i] */
            dwarf_dealloc(dbg, funcs[i], DW_DLA_FUNC);
        }
        dwarf_dealloc(dbg, funcs, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.H 4 "dwarf_funcname()"
.DS
\f(CWint dwarf_funcname(
        Dwarf_Func func,
        char **    return_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_funcname()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to a
null-terminated string that names the static function represented by the
\f(CWDwarf_Func\fP descriptor, \f(CWfunc\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.

.H 4 "dwarf_func_die_offset()"
.DS
\f(CWint dwarf_func_die_offset(
        Dwarf_Func func,
	Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_func_die_offset()\fP, returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the static function that is described by the \f(CWDwarf_Func\fP
descriptor, \f(CWfunc\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_func_cu_offset()"
.DS
\f(CWint dwarf_func_cu_offset(
        Dwarf_Func func,
	Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_func_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the
compilation-unit that contains the static function
described by the \f(CWDwarf_Func\fP descriptor, \f(CWfunc\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_func_name_offsets()"
.DS
\f(CWint dwarf_func_name_offsets(
        Dwarf_Func func,
	char     **func_name,
        Dwarf_Off *die_offset,
        Dwarf_Off *cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_func_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*func_name\fP to
a pointer to
a null-terminated string that gives the name of the static
function described by the \f(CWDwarf_Func\fP descriptor \f(CWfunc\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP, the offsets
of the DIE representing the
static function, and the DIE
representing the compilation-unit containing the
static function, respectively.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from \f(CWdwarf_func_name_offsets()\fP
the storage pointed to by  \f(CWfunc_name\fP should be freed using
\f(CWdwarf_dealloc()\fP, with the allocation type \f(CWDW_DLA_STRING\fP
when no longer of interest.

.H 2 "User Defined Type Names Operations"
Section "debug_typenames" is SGI specific
and is not part of standard DWARF version 2.
(However, an identical section is part of DWARF version 3
named ".debug_pubtypes", see  \f(CWdwarf_get_pubtypes()\fP above.)
.P
These functions operate on the .debug_typenames section of the debugging
information.  The .debug_typenames section contains the names of file-scope
user-defined types, the offsets of the \f(CWDIE\fPs that represent the
definitions of those types, and the offsets of the compilation-units
that contain the definitions of those types.

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_types()"
.DS
\f(CWint dwarf_get_types(
        Dwarf_Debug dbg,
        Dwarf_Type **types,
        Dwarf_Signed *typecount,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_types()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*typecount\fP to
the count of user-defined
type names represented in the section containing user-defined type names,
i.e. .debug_typenames.
It also stores at \f(CW*types\fP,
a pointer to a list of \f(CWDwarf_Type\fP descriptors, one for each of the
user-defined type names in the .debug_typenames section.
The returned results are for the entire section.
It returns \f(CWDW_DLV_NOCOUNT\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if
the .debug_typenames section does not exist.

.P

On a successful
return from \f(CWdwarf_get_types()\fP,
the \f(CWDwarf_Type\fP descriptors should be
freed using \f(CWdwarf_types_dealloc()\fP.
\f(CWdwarf_types_dealloc()\fP is new as of July 15, 2005
and frees all memory allocated by \f(CWdwarf_get_types()\fP.

.in +2
.FG "Examplel dwarf_get_types()"
.DS
\f(CW
void examplel(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Type *types = 0;
    Dwarf_Signed i = 0;
    int res = 0;

    res = dwarf_get_types(dbg, &types,&count, &error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use types[i] */
        }
        dwarf_types_dealloc(dbg, types, count);
    }
}
\fP
.DE
.in -2



.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
On a successful
return from \f(CWdwarf_get_types()\fP,
the \f(CWDwarf_Type\fP descriptors should be
individually freed using \f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_TYPENAME_CONTEXT\fP,
(or
\f(CWDW_DLA_TYPENAME\fP, an older name, supported for compatibility)
followed by the deallocation of the list itself
with the allocation type \f(CWDW_DLA_LIST\fP when the descriptors are no
longer of interest.

.in +2
.FG "Examplel dwarf_get_types() obsolete"
.DS
\f(CW
void examplem(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Type *types = 0;
    Dwarf_Signed i = 0;
    int res = 0;

    /*  OBSOLETE: see dwarf_types_dealloc() above */
    res = dwarf_get_types(dbg, &types,&count, &error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use types[i] */
            dwarf_dealloc(dbg, types[i], DW_DLA_TYPENAME);
        }
        dwarf_dealloc(dbg, types, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.H 4 "dwarf_typename()"
.DS
\f(CWint dwarf_typename(
        Dwarf_Type   type,
        char       **return_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_typename()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_name\fP to
a pointer to a
null-terminated string that names the user-defined type represented by the
\f(CWDwarf_Type\fP descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.

.H 4 "dwarf_type_die_offset()"
.DS
\f(CWint dwarf_type_die_offset(
        Dwarf_Type type,
        Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_type_die_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the user-defined type that is described by the \f(CWDwarf_Type\fP
descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_type_cu_offset()"
.DS
\f(CWint dwarf_type_cu_offset(
        Dwarf_Type type,
        Dwarf_Off  *return_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_type_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the compilation-unit that contains the user-defined type
described by the \f(CWDwarf_Type\fP descriptor, \f(CWtype\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_type_name_offsets()"
.DS
\f(CWint dwarf_type_name_offsets(
        Dwarf_Type   type,
        char      ** returned_name,
        Dwarf_Off *  die_offset,
        Dwarf_Off *  cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_type_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_name\fP to
a pointer to
a null-terminated string that gives the name of the user-defined
type described by the \f(CWDwarf_Type\fP descriptor \f(CWtype\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP, the offsets
of the DIE representing the
user-defined type, and the DIE
representing the compilation-unit containing the
user-defined type, respectively.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from \f(CWdwarf_type_name_offsets()\fP
the storage pointed to by \f(CWreturned_name\fP should
be freed using
\f(CWdwarf_dealloc()\fP, with the allocation type \f(CWDW_DLA_STRING\fP
when no longer of interest.


.H 2 "User Defined Static Variable Names Operations"
This section is SGI specific and is not part of standard DWARF version 2.
.P
These functions operate on the .debug_varnames section of the debugging
information.  The .debug_varnames section contains the names of file-scope
static variables, the offsets of the \f(CWDIE\fPs that represent the
definitions of those variables, and the offsets of the compilation-units
that contain the definitions of those variables.
.P

.H 3 "Debugger Interface Operations"

.H 4 "dwarf_get_vars()"

.DS
\f(CWint dwarf_get_vars(
        Dwarf_Debug dbg,
        Dwarf_Var **vars,
        Dwarf_Signed *var_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_vars()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*var_count\fP to
the count of file-scope
static variable names represented in the section containing file-scope
static variable names, i.e. .debug_varnames.
It also stores, at \f(CW*vars\fP, a pointer to a list of
\f(CWDwarf_Var\fP descriptors, one for each of the file-scope static
variable names in the .debug_varnames section.
The returned results are for the entire section.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if the .debug_varnames section does
not exist.

.P
The following is new as of July 15, 2005.
On a successful return
from \f(CWdwarf_get_vars()\fP, the \f(CWDwarf_Var\fP descriptors should be
freed using \f(CWdwarf_vars_dealloc()\fP.

.in +2
.FG "Examplen dwarf_get_vars()"
.DS
\f(CW
void examplen(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Var *vars = 0;
    Dwarf_Signed i = 0;
    int res = 0;
    res = dwarf_get_vars(dbg, &vars,&count,&error);
    if (res == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use vars[i] */
        }
        dwarf_vars_dealloc(dbg, vars, count);
    }
}
\fP
.DE
.in -2

.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
On a successful return
from \f(CWdwarf_get_vars()\fP, the \f(CWDwarf_Var\fP descriptors should be individually
freed using \f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_VAR_CONTEXT\fP,
(or
\f(CWDW_DLA_VAR\fP, an older name, supported for compatibility)
followed by the deallocation of the list itself with
the allocation type \f(CWDW_DLA_LIST\fP when the descriptors are no
longer of interest.

.in +2
.FG "Exampleo dwarf_get_vars() obsolete"
.DS
\f(CW
void exampleo(Dwarf_Debug dbg)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Var *vars = 0;
    Dwarf_Signed i = 0;
    int res = 0;
    res = dwarf_get_vars(dbg, &vars,&count,&error);
    if (res == DW_DLV_OK) {
        /* DO NOT USE: see dwarf_vars_dealloc() above */
        for (i = 0; i < count; ++i) {
            /* use vars[i] */
            dwarf_dealloc(dbg, vars[i], DW_DLA_VAR);
        }
        dwarf_dealloc(dbg, vars, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.H 4 "dwarf_varname()"
.DS
\f(CWint dwarf_varname(
        Dwarf_Var var,
        char **    returned_name,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_varname()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_name\fP to
a pointer to a
null-terminated string that names the file-scope static variable represented
by the \f(CWDwarf_Var\fP descriptor, \f(CWvar\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from this function, the string should
be freed using \f(CWdwarf_dealloc()\fP, with the allocation type
\f(CWDW_DLA_STRING\fP when no longer of interest.

.H 4 "dwarf_var_die_offset()"
.DS
\f(CWint dwarf_var_die_offset(
        Dwarf_Var    var,
        Dwarf_Off   *returned_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_var_die_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the DIE representing
the file-scope static variable that is described by the \f(CWDwarf_Var\fP
descriptor, \f(CWvar\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_var_cu_offset()"
.DS
\f(CWint dwarf_var_cu_offset(
        Dwarf_Var var,
        Dwarf_Off   *returned_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_var_cu_offset()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_offset\fP to
the offset in
the section containing DIEs, i.e. .debug_info, of the compilation-unit
header of the compilation-unit that contains the file-scope static
variable described by the \f(CWDwarf_Var\fP descriptor, \f(CWvar\fP.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 4 "dwarf_var_name_offsets()"
.DS
\f(CWint dwarf_var_name_offsets(
        Dwarf_Var var,
	char     **returned_name,
        Dwarf_Off *die_offset,
        Dwarf_Off *cu_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_var_name_offsets()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_name\fP to
a pointer to
a null-terminated string that gives the name of the file-scope
static variable described by the \f(CWDwarf_Var\fP descriptor \f(CWvar\fP.
It also returns in the locations
pointed to by \f(CWdie_offset\fP, and \f(CWcu_offset\fP, the offsets
of the DIE representing the

representing the compilation-unit containing the
file-scope static variable, respectively.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
On a successful return from
\f(CWdwarf_var_name_offsets()\fP the storage pointed to by
\f(CWreturned_name\fP
should be freed using \f(CWdwarf_dealloc()\fP, with the allocation
type
\f(CWDW_DLA_STRING\fP
when no longer of interest.

.H 2 "Names Fast Access (DWARF5) .debug_names"
The section
\f(CW.debug_names\fP
section is new in DWARF5 so a new set
of functions is defined to access this section.
This section replaces
\f(CW.debug_pubnames\fP
and
\f(CW.debug_pubtypes\fP
as those older sections were not found to be
useful in practice.

.H 3 "dwarf_debugnames_header()"
.DS
\f(CWint dwarf_debugnames_header(
    Dwarf_Debug         dbg,
    Dwarf_Dnames_Head * dn_out,
    Dwarf_Unsigned    * dn_index_count_out,
    Dwarf_Error *error)\fP
.DE
.P
The function
\f(CWdwarf_debugnames_header()\fP
allocates an opaque data structure used
in all the other debugnames calls.
.P
Many of the function calls here let one
extract the entire content of the section,
which is useful if one wishes to dump the
section or to use its data to create one's
own internal data structures.
.P
To free space allocated when one has finished
with these data structures, call
.DS
    Debug_Dnames_Head dn /* Assume set somehow */;
    ...
    dwarf_dealloc(dbg,dn,DW_DLA_DNAMES_HEAD);
.DE
which will free up all data
allocated for
\f(CWdwarf_debugnames_header()\fP.
.P
FIXME describe arguments.

.in +2
.DS
\f(CW
void exampledebugnames(Dwarf_Debug dbg)
{
FIXME need extended example of debugnames use.
}
\fP
.DE
.in -2
FIXME


.H 3 " dwarf_debugnames_sizes()"
.DS
\f(CW int dwarf_debugnames_sizes(Dwarf_Dnames_Head dn,
    Dwarf_Unsigned   index_number,

    Dwarf_Unsigned * section_offsets,
    Dwarf_Unsigned * version,
    Dwarf_Unsigned * offset_size,

    /* The counts are entry counts, not byte sizes. */
    Dwarf_Unsigned * comp_unit_count,
    Dwarf_Unsigned * local_type_unit_count,
    Dwarf_Unsigned * foreign_type_unit_count,
    Dwarf_Unsigned * bucket_count,
    Dwarf_Unsigned * name_count,

    /* The following are counted in bytes */
    Dwarf_Unsigned * indextable_overall_length,
    Dwarf_Unsigned * abbrev_table_size,
    Dwarf_Unsigned * entry_pool_size,
    Dwarf_Unsigned * augmentation_string_size,

    Dwarf_Error *    error*/)\fP
.DE
.P
Allows access to fields in a .debug_names DWARF5
header record.
FIXME

.H 3 " dwarf_debugnames_cu_entry()"
.DS
\f(CW int dwarf_debugnames_cu_entry(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      offset_number,
    Dwarf_Unsigned    * offset_count,
    Dwarf_Unsigned    * offset,
    Dwarf_Error *       error)\fP
.DE
Allows access to fields in cu entry from
a .debug_names DWARF5 compilation unit entry.
FIXME

.H 3 " dwarf_debugnames_local_tu_entry()"
.DS
\f(CW int dwarf_debugnames_local_tu_entry(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      offset_number,
    Dwarf_Unsigned    * offset_count,
    Dwarf_Unsigned    * offset,
    Dwarf_Error *       error)
\fP
.DE
FIXME
.H 3 " dwarf_debugnames_foreign_tu_entry()"
.DS
\f(CW int dwarf_debugnames_foreign_tu_entry(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      sig_number,
    Dwarf_Unsigned    * sig_minimum,
    Dwarf_Unsigned    * sig_count,
    Dwarf_Sig8        * signature,
    Dwarf_Error *       error)
\fP
.DE
FIXME
.H 3 " dwarf_debugnames_bucket()"
.DS
\f(CW int dwarf_debugnames_bucket(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      bucket_number,
    Dwarf_Unsigned    * bucket_count,
    Dwarf_Unsigned    * index_of_name_entry,
    Dwarf_Error *       error)
\fP
.DE
FIXME
.H 3 " dwarf_debugnames_name()"
.DS
\f(CW int dwarf_debugnames_bucket(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      name_entry,
    Dwarf_Unsigned    * names_count,
    Dwarf_Sig8        * signature,
    Dwarf_Unsigned    * offset_to_debug_str,
    Dwarf_Unsigned    * offset_in_entrypool,
    Dwarf_Error *       error)
\fP
.DE
FIXME
.H 3" dwarf_debugnames_abbrev_by_index()"
.DS
\f(CW int dwarf_debugnames_abbrev_by_index(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      abbrev_entry,
    Dwarf_Unsigned    * abbrev_code,
    Dwarf_Unsigned    * tag,
    Dwarf_Unsigned    * number_of_abbrev,
    Dwarf_Unsigned    * number_of_attr_form_entries,
    Dwarf_Error *       error)
\fP
.DE
FIXME
.H 3 " dwarf_debugnames_abbrev_by_code()"
.DS
\f(CW int dwarf_debugnames_abbrev_by_code(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      abbrev_code,
    Dwarf_Unsigned    * tag,
    Dwarf_Unsigned    * index_of_abbrev,
    Dwarf_Unsigned    * index_of_attr_form_entries,
    Dwarf_Error *       error)
\fP
.DE
FIXME

.H 3 " dwarf_debugnames_form_by_index()"
.DS
\f(CW int dwarf_debugnames_form_by_index(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      abbrev_entry_index,
    Dwarf_Unsigned      abbrev_form_index,
    Dwarf_Unsigned    * name_attr_index,
    Dwarf_Unsigned    * form,
    Dwarf_Unsigned    * number_of_attr_form_entries,
    Dwarf_Error *       error)
\fP
.DE
FIXME

.H 3 " dwarf_debugnames_entrypool()"
.DS
\f(CW int dwarf_debugnames_entrypool(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      offset_in_entrypool,
    Dwarf_Unsigned    * abbrev_code,
    Dwarf_Unsigned    * tag,
    Dwarf_Unsigned    * value_count,
    Dwarf_Unsigned    * index_of_abbrev,
    Dwarf_Unsigned    * offset_of_initial_value,
    Dwarf_Error *       error)
\fP
.DE
FIXME

.H 3 " dwarf_debugnames_entrypool_values()"
.DS
\f(CW int dwarf_debugnames_entrypool_values(
    Dwarf_Dnames_Head dn,
    Dwarf_Unsigned      index_number,
    Dwarf_Unsigned      index_of_abbrev,
    Dwarf_Unsigned      offset_in_entrypool_of_values,
    Dwarf_Unsigned    * array_dw_idx_number,
    Dwarf_Unsigned    * array_form,
    Dwarf_Unsigned    * array_of_offsets,
    Dwarf_Sig8        * array_of_signatures,
    Dwarf_Error *       error)
\fP
.DE
FIXME




.H 2 "Macro Information Operations (DWARF4, DWARF5)"
This section refers to DWARF4 and later
macro information from the .debug_macro
section (for DWARF 4 some producers
generated .debug_macro before its formal
standardization in DWARF 5).
While standard operations are supported there
is as yet no support for implementation-defined
extensions.
Once someone has defined such things it will
make sense to design an interface for extensions.

.H 3 "Getting access"
The opaque struct pointer Dwarf_Macro_Context
is allocated by either
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP
and once the context is no longer needed
one frees up all its storage by
\f(CWdwarf_dealloc_macro_context()\fP.

.H 4 "dwarf_get_macro_context()"
.DS
\f(CWint dwarf_get_macro_context(Dwarf_Die die,
    Dwarf_Unsigned      * version_out,
    Dwarf_Macro_Context * macro_context,
    Dwarf_Unsigned      * macro_unit_offset_out,
    Dwarf_Unsigned      * macro_ops_count_out,
    Dwarf_Unsigned      * macro_ops_data_length_out,
    Dwarf_Error         * error);\fP
.DE
Given a Compilation Unit (CU) die,
on success
\f(CWdwarf_get_macro_context()\fP
opens a
\f(CWDwarf_Macro_Context\fP
and returns a pointer to it and some data from the
macro unit for that CU.
The \f(CWDwarf_Macro_Context\fP
is used to get at the details of the macros.
.P
The value
\f(CWversion_out\fP
is set to the DWARF version number of the macro data.
Version 5 means DWARF5 version information. Version 4
means the DWARF5 format macro data is present as an
extension of DWARF4.
.P
The value
\f(CWmacro_unit_offset_out\fP
is set to the offset in the .debug_macro section
of the first byte of macro data for this CU.
.P
The value
\f(CWmacro_ops_count_out\fP
is set to the number of macro entries in the macro data
data for this CU.
The count includes the final zero entry (which is not
really a macro, it is a terminator, a zero byte ending
the macro unit).
.P
The value
\f(CWmacro_ops_data_length_out\fP
is set to the number of bytes of data in the macro unit,
including the macro unit header.

.P
If
\f(CWDW_DLV_NO_ENTRY\fP
is returned the CU has no macro data attribute or
there is no .debug_macro section present.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.

.H 4 "dwarf_get_macro_context_by_offset()"
.DS
\f(CWint dwarf_get_macro_context_by_offset(Dwarf_Die die,
    Dwarf_Unsigned        offset,
    Dwarf_Unsigned      * version_out,
    Dwarf_Macro_Context * macro_context,
    Dwarf_Unsigned      * macro_ops_count_out,
    Dwarf_Unsigned      * macro_ops_total_byte_len,
    Dwarf_Error         * error);\fP
.DE
Given a Compilation Unit (CU) die and
the offset of an imported macro unit
\f(CWdwarf_get_macro_context_by_offset()\fP
opens a
\f(CWDwarf_Macro_Context\fP
and returns a pointer to it and some data from the
macro unit for that CU on success.
.P
On success
the function produces the same output values as
\f(CWdwarf_get_macro_context()\fP
except  there is no offset returned (
the caller provides it).
.P
If
\f(CWDW_DLV_NO_ENTRY\fP
is returned there is no .debug_macro section present.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.



.H 4 "dwarf_dealloc_macro_context()"
.DS
\f(CWvoid dwarf_dealloc_macro_context(Dwarf_Macro_Context macro_context);\fP
.DE
The function
\f(CWdwarf_dealloc_macro_context()\fP
cleans up memory allocated by a successful
call to
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP.

.in +2
.FG "Examplep5 dwarf_dealloc_macro_context()"
.DS
\f(CW
/*  This builds an list or some other data structure
    (not defined) to give an import somewhere to list
    the import offset and then later to enquire
    if the list has unexamined offsets.
    A candidate set of hypothetical functions that
    callers would write:
    has_unchecked_import_in_list()
    get_next_import_from_list()
    mark_this_offset_as_examined(macro_unit_offset);
    add_offset_to_list(offset);
*/
void examplep5(Dwarf_Debug dbg, Dwarf_Die cu_die)
{
    int lres = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Macro_Context macro_context = 0;
    Dwarf_Unsigned macro_unit_offset = 0;
    Dwarf_Unsigned number_of_ops = 0;
    Dwarf_Unsigned ops_total_byte_len = 0;
    Dwarf_Bool is_primary = TRUE;
    unsigned k = 0;
    Dwarf_Error err = 0;

    for(;;) {
        if (is_primary) {
            lres = dwarf_get_macro_context(cu_die,
                &version,&macro_context,
                &macro_unit_offset,
                &number_of_ops,
                &ops_total_byte_len,
                &err);
            is_primary = FALSE;
        } else {
            if (has_unchecked_import_in_list()) {
                macro_unit_offset = get_next_import_from_list();
            } else {
                /* We are done */
                break;
            }
            lres = dwarf_get_macro_context_by_offset(cu_die,
                macro_unit_offset,
                &version,
                &macro_context,
                &number_of_ops,
                &ops_total_byte_len,
                &err);
            mark_this_offset_as_examined(macro_unit_offset);
        }

        if (lres == DW_DLV_ERROR) {
            /* Something is wrong. */
            return;
        }
        if (lres == DW_DLV_NO_ENTRY) {
            /* We are done. */
            break;
        }
        /* lres ==  DW_DLV_OK) */
        for (k = 0; k < number_of_ops; ++k) {
            Dwarf_Unsigned  section_offset = 0;
            Dwarf_Half      macro_operator = 0;
            Dwarf_Half      forms_count = 0;
            const Dwarf_Small *formcode_array = 0;
            Dwarf_Unsigned  line_number = 0;
            Dwarf_Unsigned  index = 0;
            Dwarf_Unsigned  offset =0;
            const char    * macro_string =0;
            int lres = 0;

            lres = dwarf_get_macro_op(macro_context,
                k, &section_offset,&macro_operator,
                &forms_count, &formcode_array,&err);
            if (lres != DW_DLV_OK) {
                print_error(dbg,
                    "ERROR from  dwarf_get_macro_op()",
                    lres,err);
                dwarf_dealloc_macro_context(macro_context);
                return;
            }
            switch(macro_operator) {
            case 0:
                /* Nothing to do. */
                break;
            case DW_MACRO_end_file:
                /* Do something */
                break;
            case DW_MACRO_define:
            case DW_MACRO_undef:
            case DW_MACRO_define_strp:
            case DW_MACRO_undef_strp:
            case DW_MACRO_define_strx:
            case DW_MACRO_undef_strx:
            case DW_MACRO_define_sup:
            case DW_MACRO_undef_sup: {
                lres = dwarf_get_macro_defundef(macro_context,
                    k,
                    &line_number,
                    &index,
                    &offset,
                    &forms_count,
                    &macro_string,
                    &err);
                if (lres != DW_DLV_OK) {
                    print_error(dbg,
                        "ERROR from sup dwarf_get_macro_defundef()",
                        lres,err);
                    dwarf_dealloc_macro_context(macro_context);
                    return;
                }
                /* do something */
                }
                break;
            case DW_MACRO_start_file: {
                lres = dwarf_get_macro_startend_file(macro_context,
                    k,&line_number,
                    &index,
                    &macro_string,&err);
                if (lres != DW_DLV_OK) {
                    print_error(dbg,
                        "ERROR from  dwarf_get_macro_startend_file()(sup)",
                        lres,err);
                    dwarf_dealloc_macro_context(macro_context);
                    return;
                }
                /* do something */
                }
                break;
            case DW_MACRO_import: {
                lres = dwarf_get_macro_import(macro_context,
                    k,&offset,&err);
                if (lres != DW_DLV_OK) {
                    print_error(dbg,
                        "ERROR from  dwarf_get_macro_import()(sup)",
                        lres,err);
                    dwarf_dealloc_macro_context(macro_context);
                    return;
                }
                add_offset_to_list(offset);
                }
                break;
            case DW_MACRO_import_sup: {
                lres = dwarf_get_macro_import(macro_context,
                    k,&offset,&err);
                if (lres != DW_DLV_OK) {
                    print_error(dbg,
                        "ERROR from  dwarf_get_macro_import()(sup)",
                        lres,err);
                    dwarf_dealloc_macro_context(macro_context);
                    return;
                }
                /* do something */
                }
                break;
            }
        }
        dwarf_dealloc_macro_context(macro_context);
        macro_context = 0;
    }
}
\fP
.DE
.in -2


.H 3 "Getting Macro Unit Header Data"
.H 4 "dwarf_macro_context_head()"
.DS
\f(CWint dwarf_macro_context_head(Dwarf_Macro_Context macro_context,
    Dwarf_Half     * version,
    Dwarf_Unsigned * mac_offset,
    Dwarf_Unsigned * mac_len,
    Dwarf_Unsigned * mac_header_len,
    unsigned       * flags,
    Dwarf_Bool     * has_line_offset,
    Dwarf_Unsigned * line_offset,
    Dwarf_Bool     * has_offset_size_64,
    Dwarf_Bool     * has_operands_table,
    Dwarf_Half     * opcode_count,
    Dwarf_Error    * error); \fP
.DE
Given a
\f(CWDwarf_Macro_Context\fP
pointer this function returns the basic fields
of a macro unit header (Macro Information Header)
on success.
.P
The value
\f(CWversion\fP
is set to the DWARF version number of the macro unit header.
Version 5 means DWARF5 version information. Version 4
means the DWARF5 format macro data is present as an
extension of DWARF4.
.P
The value
\f(CWmac_offset\fP
is set to the offset in the .debug_macro section
of the first byte of macro data for this CU.
.P
The value
\f(CWmac_len\fP
is set to the number of bytes of data in the macro unit,
including the macro unit header.
.P
The value
\f(CWmac_header_len\fP
is set to the number of bytes in the macro unit header
(not a field that is generally useful).
.P
The value
\f(CWflags\fP
is set to the value of the
\f(CWflags\fP
field of the macro unit header.
.P
The value
\f(CWhas_line_offset\fP
is set to non-zero if the
\f(CWdebug_line_offset_flag\fP
bit is set in the
\f(CWflags\fP
field of the macro unit header.
If
\f(CWhas_line_offset\fP
is set then
\f(CWline_offset\fP
is set to the  value of the
\f(CWdebug_line_offset\fP
field in the macro unit header.
If
\f(CWhas_line_offset\fP
is not set there is no
\f(CWdebug_line_offset\fP
field present in the
macro unit header.
.P
The value
\f(CWhas_offset_size_64\fP
is set non-zero if the
\f(CWoffset_size_flag\fP
bit is set in the
\f(CWflags\fP
field of the macro unit header and in this
case offset fields in this macro unit are
64 bits.
If
\f(CWhas_offset_size_64\fP
is not set then
offset fields in this macro unit are
32 bits.
.P
The value
\f(CWhas_operands_table\fP
is set to non-zero if the
\f(CWopcod_operands_table_flag\fP
bit is set in the
\f(CWflags\fP
field of the macro unit header.
.P
If
\f(CWhas_operands_table\fP
is set non-zero then
The value
\f(CWopcode_count\fP
is set to the number of opcodes
in the macro unit header
\f(CWopcode_operands_table\fP.
See
\f(CWdwarf_get_macro_op()\fP.
.P
\f(CWDW_DLV_NO_ENTRY\fP
is not returned.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.

.H 4 "dwarf_macro_operands_table()"
.DS
\f(CWint dwarf_macro_operands_table(Dwarf_Macro_Context macro_context,
    Dwarf_Half    index, /* 0 to opcode_count -1 */
    Dwarf_Half  * opcode_number,
    Dwarf_Half  * operand_count,
    const Dwarf_Small ** operand_array,
    Dwarf_Error * error); \fP
.DE
\f(CWdwarf_macro_operands_table()\fP
is used to index through the operands table
in a macro unit header if the operands
table exists in the macro unit header.
The operands table provides the mechanism
for implementations to add extensions
to the macro operations while allowing
clients to skip macro operations the
client code does not recognize.
.P
The
\f(CWmacro_context\fP
field passed in identifies the macro unit involved.
The
\f(CWindex\fP
field passed in identifies which macro operand to
look at. Valid index values are
zero through
the
\f(CWopcode_count\fP-1
(returned by
\f(CWdwarf_macro_context_head()\fP).
.P
The
\f(CWopcode_number\fP
value returned through the pointer is
the the macro operation code.
The operation code could be one of the
standard codes or if there are user extensions
there would be an extension code in the
\f(CWDW_MACRO_lo_user\fP
to
\f(CWDW_MACRO_hi_user\fP
range.
.P
The
\f(CWoperand_count\fP
returned
is the number of form codes in the form codes
array of unsigned bytes
\f(CWoperand_array\fP.

.P
\f(CWDW_DLV_NO_ENTRY\fP
is not returned.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.

.H 3 "Getting Individual Macro Operations Data"

.H 4 "dwarf_get_macro_op()"
.DS
\f(CWint dwarf_get_macro_op(Dwarf_Macro_Context macro_context,
    Dwarf_Unsigned op_number,
    Dwarf_Unsigned * op_start_section_offset,
    Dwarf_Half    * macro_operator,
    Dwarf_Half    * forms_count,
    const Dwarf_Small **   formcode_array,
    Dwarf_Error   * error);\fP
.DE
Use
\f(CWdwarf_get_macro_op()\fP
to access the macro operations of this macro unit.
.P
The
\f(CWmacro_context\fP
field passed in identifies the macro unit involved.
The
\f(CWop_number\fP
field passed in identifies which macro operand to
look at. Valid index values are
zero through
\f(CWmacro_ops_count_out\fP-1
(field returned by
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP)
.P
On success the function returns values
through the pointers.
.P
The
\f(CWop_start_section_offset\fP
returned is useful for debugging
but otherwise is not normally useful.
It is the byte offset of the beginning
of this macro operator's data.
.P
The
\f(CWmacro_operator\fP
returned is one of the defined
macro operations such as
\f(CWDW_MACRO_define\fP.
This is the field you will use to
choose what call to use to get the
data for a macro operator.
For example, for
\f(CWDW_MACRO_undef\fP
one would call
\f(CWdwarf_get_macro_defundef()\fP
(see below)
to get the details about the undefine.
.P
The
\f(CWforms_count\fP
returned is useful for debugging
but otherwise is not normally useful.
It is the number of bytes of form numbers
in the
\f(CWformcode_array\fP
of this macro operator's applicable forms.

.P
\f(CWDW_DLV_NO_ENTRY\fP
is not returned.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.

.H 4 "dwarf_get_macro_defundef()"
.DS
\f(CWint dwarf_get_macro_defundef(Dwarf_Macro_Context macro_context,
    Dwarf_Unsigned   op_number,
    Dwarf_Unsigned * line_number,
    Dwarf_Unsigned * index,
    Dwarf_Unsigned * offset,
    Dwarf_Half     * forms_count,
    const char    ** macro_string,
    Dwarf_Error    * error);\fP
.DE
Call
\f(CWdwarf_get_macro_defundef\fP for
any of the macro define/undefine operators.
Which fields are set through the pointers
depends on the particular operator.
.P
The
\f(CWmacro_context\fP
field passed in identifies the macro unit involved.
The
\f(CWop_number\fP
field passed in identifies which macro operand to
look at. Valid index values are
zero through
\f(CWmacro_ops_count_out\fP-1
(field returned by
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP).
.P
The
\f(CWline_number\fP
field is set with the source line number of the macro.
.P
The
\f(CWindex\fP
field only set meaningfully if the macro operator is
\f(CWDW_MACRO_define_strx\fP
or
\f(CWDW_MACRO_undef_strx\fP.
If set it is an index into an array of offsets in the .debug_str_offsets
section.
.P
The
\f(CWoffset\fP
field only set meaningfully if the macro operator is
\f(CWDW_MACRO_define_strx\fP,
\f(CWDW_MACRO_undef_strx\fP
\f(CWDW_MACRO_define_strp\fP,
or
\f(CWDW_MACRO_undef_strp\fP
If set it is an offset of a string in the .debug_str section.
.P
The
\f(CWforms_count\fP
is set to the number of forms
that apply to the macro operator.
.P
The
\f(CWmacro_string\fP
pointer is used to return a pointer to
the macro string.
If the actual string cannot be found
(as when section with the string is in
a different object, see
\f(CWset_tied_dbg()\fP)
the string returned may be
"<:No string available>"
or
"<.debug_str_offsets not available>"
(without the quotes).
.P
The function returns
\f(CWDW_DLV_NO_ENTRY\fP
if the macro operation is not
one of the define/undef
operations.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.
.H 4 "dwarf_get_macro_startend_file()"
.DS
\f(CWint dwarf_get_macro_startend_file(Dwarf_Macro_Context macro_context,
    Dwarf_Unsigned   op_number,
    Dwarf_Unsigned * line_number,
    Dwarf_Unsigned * name_index_to_line_tab,
    const char    ** src_file_name,
    Dwarf_Error    * error);\fP
.DE
Call
\f(CWdwarf_get_macro_startend_file\fP for
operators
\f(CWDW_MACRO_start_file\fP
or
\f(CWDW_MACRO_end_file\fP.
.P
The
\f(CWmacro_context\fP
field passed in identifies the macro unit involved.
.P
The
\f(CWop_number\fP
field passed in identifies which macro operand to
look at. Valid index values are
zero through
\f(CWmacro_ops_count_out\fP-1
(field returned by
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP)
.P
For
\f(CWDW_MACRO_end_file\fP
none of the following fields are set
on successful return, they are only
set for.
\f(CWDW_MACRO_start_file\fP
.P
The
\f(CWline_number\fP
field is set with the source line number of the macro.
.P
.P
The
\f(CWname_index_to_line_tab\fP
field is set with the index into the
file name table of the line table section.
For DWARF2, DWARF3, DWARF4 line tables
the index value assumes DWARF2 line table header rules
(identical to DWARF3, DWARF4 line table header rules).
For DWARF5
the index value assumes DWARF5 line table header rules.
The
\f(CWsrc_file_name\fP
is set with the source file name.
If the index seems wrong or
the line table is unavailable
the name returned is "<no-source-file-name-available>");

.P
The function returns
\f(CWDW_DLV_NO_ENTRY\fP
if the macro operation is not
one of the start/end
operations.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.

.H 4 "dwarf_get_macro_import()"
.DS
\f(CWint dwarf_get_macro_import(Dwarf_Macro_Context macro_context,
    Dwarf_Unsigned   op_number,
    Dwarf_Unsigned * target_offset,
    Dwarf_Error    * error);\fP
.DE
Call
\f(CWdwarf_get_macro_import\fP for
operators
\f(CWDW_MACRO_import\fP
or
\f(CWDW_MACRO_import_sup\fP.
.P
The
\f(CWmacro_context\fP
field passed in identifies the macro unit involved.
The
\f(CWop_number\fP
field passed in identifies which macro operand to
look at. Valid index values are
zero through
\f(CWmacro_ops_count_out\fP-1
(field returned by
\f(CWdwarf_get_macro_context()\fP
or
\f(CWdwarf_get_macro_context_by_offset()\fP)
.P
On success the
\f(CWtarget_offset\fP
field is set to the
offset in the referenced section.
For DW_MACRO_import the referenced section is
the same section as the macro operation referenced here.
For DW_MACRO_import_sup the referenced section is
in a supplementary object.
.P
The function returns
\f(CWDW_DLV_NO_ENTRY\fP
if the macro operation is not
one of the import
operations.
.P
On error
\f(CWDW_DLV_ERROR\fP
is returned and
the error details are returned through the pointer
\f(CWerror\fP.


.H 2 "Macro Information Operations (DWARF2, DWARF3, DWARF4)"
This section refers to DWARF2,DWARF3,and DWARF4
macro information from the .debug_macinfo
section. These do not apply to DWARF5
macro data.
.H 3 "General Macro Operations"
.H 4 "dwarf_find_macro_value_start()"
.DS
\f(CWchar *dwarf_find_macro_value_start(char * macro_string);\fP
.DE
Given a macro string in the standard form defined in the DWARF
document ("name <space> value" or "name(args)<space>value")
this returns a pointer to the first byte of the macro value.
It does not alter the string pointed to by macro_string or copy
the string: it returns a pointer into the string whose
address was passed in.
.H 3 "Debugger Interface Macro Operations"
Macro information is accessed from the .debug_info section via the
DW_AT_macro_info attribute (whose value is an offset into .debug_macinfo).
.P
No Functions yet defined.
.H 3 "Low Level Macro Information Operations"
.H 4 "dwarf_get_macro_details()"
.DS
\f(CWint dwarf_get_macro_details(Dwarf_Debug /*dbg*/,
  Dwarf_Off              macro_offset,
  Dwarf_Unsigned         maximum_count,
  Dwarf_Signed         * entry_count,
  Dwarf_Macro_Details ** details,
  Dwarf_Error *          err);\fP
.DE
\f(CWdwarf_get_macro_details()\fP
returns
\f(CWDW_DLV_OK\fP and sets
\f(CWentry_count\fP to the number of \f(CWdetails\fP records
returned through the \f(CWdetails\fP pointer.
The data returned through  \f(CWdetails\fP should be freed
by a call to \f(CWdwarf_dealloc()\fP with the allocation type
\f(CWDW_DLA_STRING\fP.
If \f(CWDW_DLV_OK\fP is returned, the \f(CWentry_count\fP will
be at least 1, since
a compilation unit with macro information but no macros will
have at least one macro data byte of 0.
.P
\f(CWdwarf_get_macro_details()\fP
begins at the \f(CWmacro_offset\fP offset you supply
and ends at the end of a compilation unit or at \f(CWmaximum_count\fP
detail records (whichever comes first).
If \f(CWmaximum_count\fP is 0, it is treated as if it were the maximum
possible unsigned integer.
.P
\f(CWdwarf_get_macro_details()\fP
attempts to set \f(CWdmd_fileindex\fP to the correct file in every
\f(CWdetails\fP record. If it is unable to do so (or whenever
the current file index is unknown, it sets \f(CWdmd_fileindex\fP
to -1.
.P
\f(CWdwarf_get_macro_details()\fP returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if there is no more
macro information at that \f(CWmacro_offset\fP. If \f(CWmacro_offset\fP
is passed in as 0, a \f(CWDW_DLV_NO_ENTRY\fP return means there is
no macro information.
.P
.in +2
.FG "Examplep2 dwarf_get_macro_details()"
.DS
\f(CW
void examplep2(Dwarf_Debug dbg, Dwarf_Off cur_off)
{
    Dwarf_Error error = 0;
    Dwarf_Signed count = 0;
    Dwarf_Macro_Details *maclist = 0;
    Dwarf_Signed i = 0;
    Dwarf_Unsigned max = 500000; /* sanity limit */
    int errv = 0;

    /*  Given an offset from a compilation unit,
        start at that offset (from DW_AT_macroinfo)
        and get its macro details. */
    errv = dwarf_get_macro_details(dbg, cur_off,max,
        &count,&maclist,&error);
    if (errv == DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use maclist[i] */
        }
        dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    }
    /*  Loop through all the compilation units macro info from zero.
        This is not guaranteed to work because DWARF does not
        guarantee every byte in the section is meaningful:
        there can be garbage between the macro info
        for CUs.  But this loop will sometimes work.
    */
    cur_off = 0;
    while((errv = dwarf_get_macro_details(dbg, cur_off,max,
        &count,&maclist,&error))== DW_DLV_OK) {
        for (i = 0; i < count; ++i) {
            /* use maclist[i] */
        }
        cur_off = maclist[count-1].dmd_offset + 1;
        dwarf_dealloc(dbg, maclist, DW_DLA_STRING);
    }
}
\fP
.DE
.in -2


.H 2 "Low Level Frame Operations"
These functions provide information about stack frames to be
used to perform stack traces.  The information is an abstraction
of a table with a row per instruction and a column per register
and a column for the canonical frame address (CFA, which corresponds
to the notion of a frame pointer),
as well as a column for the return address.
.P
From 1993-2006 the interface we'll here refer to as DWARF2
made the CFA be a column in the matrix, but left
DW_FRAME_UNDEFINED_VAL, and DW_FRAME_SAME_VAL out of the matrix
(giving them high numbers). As of the DWARF3 interfaces
introduced in this document in April 2006, there are *two*
interfaces (the original set and a new set).
Several frame functions work transparently for either set, we
will focus on the ones that are not equally suitable
now.
.P
The original DWARF2 interface set still exists
(dwarf_get_fde_info_for_reg(),
dwarf_get_fde_info_for_cfa_reg(), and dwarf_get_fde_info_for_all_regs())
and works adequately for MIPS/IRIX DWARF2 and ABI/ISA sets
that are sufficiently similar to MIPS.
These functions not a good choice for non-MIPS architectures nor
were they a good design for MIPS either.
It's better to switch entirely to the new functions mentioned
in the next paragraph.
This DWARF2 interface set assumes and uses DW_FRAME_CFA_COL
and that is assumed when libdwarf is configured with --enable-oldframecol .
.P
A new DWARF3 interface set of dwarf_get_fde_info_for_reg3(),
dwarf_get_fde_info_for_cfa_reg3(), dwarf_get_fde_info_for_all_regs3(),
dwarf_set_frame_rule_table_size()
dwarf_set_frame_cfa_value(),
dwarf_set_frame_same_value(),
dwarf_set_frame_undefined_value(), and
dwarf_set_frame_rule_initial_value()
is more flexible
and will work for many more architectures.
It is also entirely suitable for use with DWARF2 and DWARF4.
The setting of  the 'frame cfa column number'
defaults to DW_FRAME_CFA_COL3
and it can be set at runtime with dwarf_set_frame_cfa_value().
.P
Mixing use of the DWARF2 interface set with use
of the new DWARF3 interface set
on a single open Dwarf_Debug instance is a mistake.
Do not do it.
.P
We will pretend, from here on unless otherwise
specified, that
DW_FRAME_CFA_COL3, DW_FRAME_UNDEFINED_VAL,
and DW_FRAME_SAME_VAL are the synthetic column numbers.
These columns may be user-chosen by calls of
dwarf_set_frame_cfa_value()
dwarf_set_frame_undefined_value(), and
dwarf_set_frame_same_value() respectively.

.P
Each cell in the table contains one of the following:

.AL
.LI
A register + offset(a)(b)

.LI
A register(c)(d)


.LI
A marker (DW_FRAME_UNDEFINED_VAL) meaning \fIregister value undefined\fP

.LI
A marker (DW_FRAME_SAME_VAL) meaning
\fIregister value same as in caller\fP
.LE
.P
(a old DWARF2 interface) When  the column is DW_FRAME_CFA_COL:
the register
number is a real hardware register, not a reference
to DW_FRAME_CFA_COL, not  DW_FRAME_UNDEFINED_VAL,
and not DW_FRAME_SAME_VAL.
The CFA rule value should be the stack pointer
plus offset 0 when no other value makes sense.
A value of DW_FRAME_SAME_VAL would
be semi-logical, but since the CFA is not a real register,
not really correct.
A value of DW_FRAME_UNDEFINED_VAL would imply
the CFA is undefined  --
this seems to be a useless notion, as
the CFA is a means to finding real registers,
so those real registers should be marked DW_FRAME_UNDEFINED_VAL,
and the CFA column content (whatever register it
specifies) becomes unreferenced by anything.
.P
(a new April 2006 DWARF2/3 interface): The CFA is
separately accessible and not part of the table.
The 'rule number' for the CFA is a number outside the table.
So the CFA is a marker, not a register number.
See  DW_FRAME_CFA_COL3 in libdwarf.h and
dwarf_get_fde_info_for_cfa_reg3() and
dwarf_set_frame_rule_cfa_value().
.P
(b) When the column is not DW_FRAME_CFA_COL3, the 'register'
will and must be DW_FRAME_CFA_COL3(COL), implying that
to get the final location for the column one must add
the offset here plus the DW_FRAME_CFA_COL3 rule value.
.P
(c) When the column is DW_FRAME_CFA_COL3, then the 'register'
number is (must be) a real hardware register .
(This paragraph does not apply to the April 2006 new interface).
If it were DW_FRAME_UNDEFINED_VAL or DW_FRAME_SAME_VAL
it would be a marker, not a register number.
.P
(d) When the column is not DW_FRAME_CFA_COL3, the register
may be a hardware register.
It will not be DW_FRAME_CFA_COL3.
.P
There is no 'column' for DW_FRAME_UNDEFINED_VAL or DW_FRAME_SAME_VAL.
Nor for DW_FRAME_CFA_COL3.

Figure \n(aX
is machine dependent and represents MIPS CPU register
assignments.  The DW_FRAME_CFA_COL define in dwarf.h
is historical and really belongs
in libdwarf.h, not dwarf.h.
.DS
.TS
center box, tab(:);
lfB lfB lfB
l c l.
NAME:value:PURPOSE
_
DW_FRAME_CFA_COL:0:column used for CFA
DW_FRAME_REG1:1:integer register 1
DW_FRAME_REG2:2:integer register 2
---::obvious names and values here
DW_FRAME_REG30:30:integer register 30
DW_FRAME_REG31:31:integer register 31
DW_FRAME_FREG0:32:floating point register 0
DW_FRAME_FREG1:33:floating point register 1
---::obvious names and values here
DW_FRAME_FREG30:62:floating point register 30
DW_FRAME_FREG31:63:floating point register 31
DW_FRAME_RA_COL:64:column recording ra
DW_FRAME_UNDEFINED_VAL:1034:register val undefined
DW_FRAME_SAME_VAL:1035:register same as in caller
.TE

.FG "Frame Information Rule Assignments MIPS"
.DE
.P
The following table shows SGI/MIPS specific
special cell values: these values mean
that the cell has the value \fIundefined\fP or \fIsame value\fP
respectively, rather than containing a \fIregister\fP or
\fIregister+offset\fP.
It assumes DW_FRAME_CFA_COL is a table rule, which
is not readily accomplished or even sensible for some architectures.
.P
.DS
.TS
center box, tab(:);
lfB lfB lfB
l c l.
NAME:value:PURPOSE
_
DW_FRAME_UNDEFINED_VAL:1034:means undefined value.
::Not a column or register value
DW_FRAME_SAME_VAL:1035:means 'same value' as
::caller had. Not a column or
::register value
DW_FRAME_CFA_COL:0:means register zero is
::usurped by the CFA column.
::
.TE
.FG "Frame Information Special Values any architecture"
.DE

.P
The following table shows more general special cell values.
These values mean
that the cell register-number refers to the \fIcfa-register\fP or
\fIundefined-value\fP or \fIsame-value\fP
respectively, rather than referring to a \fIregister in the table\fP.
The generality arises from making DW_FRAME_CFA_COL3 be
outside the set of registers and making the cfa rule accessible
from outside the rule-table.
.P
.DS
.TS
center box, tab(:);
lfB lfB lfB
l c l.
NAME:value:PURPOSE
_
DW_FRAME_UNDEFINED_VAL:1034:means undefined
::value. Not a column or register value
DW_FRAME_SAME_VAL:1035:means 'same value' as
::caller had. Not a column or
::register value
DW_FRAME_CFA_COL3:1436:means 'cfa register'
::is referred to, not a real register, not
::a column, but the cfa (the cfa does have
::a value, but in the DWARF3 libdwarf interface
::it does not have a 'real register number').
.TE
.DE

.P
.H 3 "dwarf_get_frame_section_name()"
.DS
\f(CWint dwarf_get_frame_section_name(Dwarf_Debug dbg,
    const char ** sec_name,
    Dwarf_Error  *error)\fP
.DE
\f(CWdwarf_get_string_section_name()\fP lets consumers
access the object string section name.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
See  also \f(CWdwarf_get_frame_section_name_eh_gnu()\fP.
.P
The function
\f(CWdwarf_get_frame_section_name()\fP operates on
the the .debug_frame section.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.

.H 3 "dwarf_get_frame_section_name_eh_gnu()"
.DS
\f(CWint dwarf_get_frame_section_name_eh_gnu(Dwarf_Debug dbg
    const char ** sec_name,
    Dwarf_Error  *error)\fP
.DE
\f(CWdwarf_get_frame_section_name_eh_gnu()\fP lets consumers
access the object string section name.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
See  also \f(CWdwarf_get_frame_section_name()\fP.
.P
The function
\f(CWdwarf_get_frame_section_name_eh_ghu()\fP operates on
the the .eh_frame section.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.

.H 3 "dwarf_get_fde_list()"
.DS
\f(CWint dwarf_get_fde_list(
        Dwarf_Debug dbg,
        Dwarf_Cie **cie_data,
        Dwarf_Signed *cie_element_count,
        Dwarf_Fde **fde_data,
        Dwarf_Signed *fde_element_count,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_fde_list()\fP stores a pointer to a list of
\f(CWDwarf_Cie\fP descriptors in \f(CW*cie_data\fP, and the
count of the number of descriptors in \f(CW*cie_element_count\fP.
There is a descriptor for each CIE in the .debug_frame section.
Similarly, it stores a pointer to a list of \f(CWDwarf_Fde\fP
descriptors in \f(CW*fde_data\fP, and the count of the number
of descriptors in \f(CW*fde_element_count\fP.  There is one
descriptor per FDE in the .debug_frame section.
\f(CWdwarf_get_fde_list()\fP  returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if it cannot find frame entries.
It returns \f(CWDW_DLV_OK\fP on a successful return.
.P
On successful return, structures pointed to by a
descriptor should be freed using \f(CWdwarf_fde_cie_list_dealloc()\fP.
This dealloc approach is new as of July 15, 2005.

.in +2
.FG "Exampleq dwarf_get_fde_list()"
.DS
\f(CW
void exampleq(Dwarf_Debug dbg)
{
    Dwarf_Cie *cie_data = 0;
    Dwarf_Signed cie_count = 0;
    Dwarf_Fde *fde_data = 0;
    Dwarf_Signed fde_count = 0;
    Dwarf_Error error = 0;
    int fres = 0;

    fres = dwarf_get_fde_list(dbg,&cie_data,&cie_count,
        &fde_data,&fde_count,&error);
    if (fres == DW_DLV_OK) {
        dwarf_fde_cie_list_dealloc(dbg, cie_data, cie_count,
            fde_data,fde_count);
    }
}
\fP
.DE
.in -2


.P
The following code is deprecated as of July 15, 2005 as it does not
free all relevant memory.
This approach  still works as well as it ever did.
.in +2
.FG "Exampleqb dwarf_get_fde_list() obsolete"
.DS
\f(CW
/* OBSOLETE EXAMPLE */
void exampleqb(Dwarf_Debug dbg)
{
    Dwarf_Cie *cie_data = 0;
    Dwarf_Signed cie_count = 0;
    Dwarf_Fde *fde_data = 0;
    Dwarf_Signed fde_count = 0;
    Dwarf_Error error = 0;
    Dwarf_Signed i = 0;
    int fres = 0;

    fres = dwarf_get_fde_list(dbg,&cie_data,&cie_count,
        &fde_data,&fde_count,&error);
    if (fres == DW_DLV_OK) {
        for (i = 0; i < cie_count; ++i) {
            /* use cie[i] */
            dwarf_dealloc(dbg, cie_data[i], DW_DLA_CIE);
        }
        for (i = 0; i < fde_count; ++i) {
            /* use fde[i] */
            dwarf_dealloc(dbg, fde_data[i], DW_DLA_FDE);
        }
        dwarf_dealloc(dbg, cie_data, DW_DLA_LIST);
        dwarf_dealloc(dbg, fde_data, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.P
.H 3 "dwarf_get_fde_list_eh()"
.DS
\f(CWint dwarf_get_fde_list_eh(
        Dwarf_Debug dbg,
        Dwarf_Cie **cie_data,
        Dwarf_Signed *cie_element_count,
        Dwarf_Fde **fde_data,
        Dwarf_Signed *fde_element_count,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_fde_list_eh()\fP is identical to
\f(CWdwarf_get_fde_list()\fP except that
\f(CWdwarf_get_fde_list_eh()\fP reads the GNU gcc
section named .eh_frame (C++ exception handling information).

\f(CWdwarf_get_fde_list_eh()\fP stores a pointer to a list of
\f(CWDwarf_Cie\fP descriptors in \f(CW*cie_data\fP, and the
count of the number of descriptors in \f(CW*cie_element_count\fP.
There is a descriptor for each CIE in the .debug_frame section.
Similarly, it stores a pointer to a list of \f(CWDwarf_Fde\fP
descriptors in \f(CW*fde_data\fP, and the count of the number
of descriptors in \f(CW*fde_element_count\fP.  There is one
descriptor per FDE in the .debug_frame section.
\f(CWdwarf_get_fde_list()\fP  returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if it cannot find
exception handling entries.
It returns \f(CWDW_DLV_OK\fP on a successful return.

.P
On successful return, structures pointed to by a
descriptor should be freed using \f(CWdwarf_fde_cie_list_dealloc()\fP.
This dealloc approach is new as of July 15, 2005.

.in +2
.FG "Exampler dwarf_get_fde_list_eh()"
.DS
\f(CW
void exampler(Dwarf_Debug dbg,Dwarf_Addr mypcval)
{
    /*  Given a pc value
        for a function find the FDE and CIE data for
        the function.
        Example shows basic access to FDE/CIE plus
        one way to access details given a PC value.
        dwarf_get_fde_n() allows accessing all FDE/CIE
        data so one could build up an application-specific
        table of information if that is more useful.  */
    Dwarf_Signed count = 0;
    Dwarf_Cie *cie_data = 0;
    Dwarf_Signed cie_count = 0;
    Dwarf_Fde *fde_data = 0;
    Dwarf_Signed fde_count = 0;
    Dwarf_Error error = 0;
    int fres = 0;

    fres = dwarf_get_fde_list_eh(dbg,&cie_data,&cie_count,
        &fde_data,&fde_count,&error);
    if (fres == DW_DLV_OK) {
        Dwarf_Fde myfde = 0;
        Dwarf_Addr low_pc = 0;
        Dwarf_Addr high_pc = 0;
        fres = dwarf_get_fde_at_pc(fde_data,mypcval,
            &myfde,&low_pc,&high_pc,
            &error);
        if (fres == DW_DLV_OK) {
            Dwarf_Cie mycie = 0;
            fres = dwarf_get_cie_of_fde(myfde,&mycie,&error);
            if (fres == DW_DLV_OK) {
                /*  Now we can access a range of information
                    about the fde and cie applicable. */
            }
        }
        dwarf_fde_cie_list_dealloc(dbg, cie_data, cie_count,
            fde_data,fde_count);
    }
    /* ERROR or NO ENTRY. Do something */
}
\fP
.DE
.in -2


.P
.H 3 "dwarf_get_cie_of_fde()"
.DS
\f(CWint dwarf_get_cie_of_fde(Dwarf_Fde fde,
        Dwarf_Cie *cie_returned,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_cie_of_fde()\fP stores a \f(CWDwarf_Cie\fP
into the  \f(CWDwarf_Cie\fP that \f(CWcie_returned\fP points at.

If one has called
\f(CWdwarf_get_fde_list()\fP
must avoid dwarf_dealloc-ing the FDEs and  the CIEs for those FDEs
individually (see its documentation here).
Failing to observe this restriction will cause the  FDE(s) not
dealloc'd to become invalid: an FDE contains (hidden in it)
a CIE pointer which will be be invalid (stale, pointing to freed memory)
if the CIE is dealloc'd.
The invalid CIE pointer internal to the FDE cannot be detected
as invalid by libdwarf.
If one later passes an FDE with a stale internal CIE pointer
to one of the routines taking an FDE as input the result will
be failure of the call (returning DW_DLV_ERROR) at best and
it is possible a coredump or worse will happen (eventually).

\f(CWdwarf_get_cie_of_fde()\fP returns
\f(CWDW_DLV_OK\fP if it is successful (it will be
unless fde is the NULL pointer).
It returns \f(CWDW_DLV_ERROR\fP if the fde is invalid (NULL).

.P
Each \f(CWDwarf_Fde\fP descriptor describes information about the
frame for a particular subroutine or function.

\f(CWint dwarf_get_fde_for_die\fP is SGI/MIPS specific.

.H 3 "dwarf_get_fde_for_die()"
.DS
\f(CWint dwarf_get_fde_for_die(
        Dwarf_Debug dbg,
        Dwarf_Die die,
        Dwarf_Fde *  return_fde,
        Dwarf_Error *error)\fP
.DE
When it succeeds,
\f(CWdwarf_get_fde_for_die()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_fde\fP
to
a \f(CWDwarf_Fde\fP
descriptor representing frame information for the given \f(CWdie\fP.  It
looks for the \f(CWDW_AT_MIPS_fde\fP attribute in the given \f(CWdie\fP.
If it finds it, is uses the value of the attribute as the offset in
the .debug_frame section where the FDE begins.
If there is no \f(CWDW_AT_MIPS_fde\fP it returns \f(CWDW_DLV_NO_ENTRY\fP.
If there is an error it returns \f(CWDW_DLV_ERROR\fP.

.H 3 "dwarf_get_fde_range()"
.DS
\f(CWint dwarf_get_fde_range(
        Dwarf_Fde fde,
        Dwarf_Addr *low_pc,
        Dwarf_Unsigned *func_length,
        Dwarf_Ptr *fde_bytes,
        Dwarf_Unsigned *fde_byte_length,
        Dwarf_Off *cie_offset,
        Dwarf_Signed *cie_index,
        Dwarf_Off *fde_offset,
        Dwarf_Error *error);\fP
.DE
On success,
\f(CWdwarf_get_fde_range()\fP returns
\f(CWDW_DLV_OK\fP.

The location pointed to by \f(CWlow_pc\fP is set to the low pc value for
this function.

The location pointed to by \f(CWfunc_length\fP is
set to the length of the function in bytes.
This is essentially the
length of the text section for the function.

The location pointed
to by \f(CWfde_bytes\fP is set to the address where the FDE begins
in the .debug_frame section.

The location pointed to by
\f(CWfde_byte_length\fP is set to the length in bytes of the portion
of .debug_frame for this FDE.
This is the same as the value returned
by \f(CWdwarf_get_fde_range\fP.

The location pointed to by
\f(CWcie_offset\fP is set to the offset in the .debug_frame section
of the CIE used by this FDE.

The location pointed to by \f(CWcie_index\fP
is set to the index of the CIE used by this FDE.
The index is the
index of the CIE in the list pointed to by \f(CWcie_data\fP as set
by the function \f(CWdwarf_get_fde_list()\fP.
However, if the function
\f(CWdwarf_get_fde_for_die()\fP was used to obtain the given \f(CWfde\fP,
this index may not be correct.

The location pointed to by
\f(CWfde_offset\fP is set to the offset of the start of this FDE in
the .debug_frame section.

\f(CWdwarf_get_fde_range()\fP returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_get_cie_info()"
.DS
\f(CWint dwarf_get_cie_info(
        Dwarf_Cie       cie,
	Dwarf_Unsigned *bytes_in_cie,
        Dwarf_Small    *version,
        char          **augmenter,
        Dwarf_Unsigned *code_alignment_factor,
        Dwarf_Signed *data_alignment_factor,
        Dwarf_Half     *return_address_register_rule,
        Dwarf_Ptr      *initial_instructions,
        Dwarf_Unsigned *initial_instructions_length,
        Dwarf_Error    *error);\fP
.DE
\f(CWdwarf_get_cie_info()\fP is primarily for Internal-level Interface
consumers.
If successful,
it returns
\f(CWDW_DLV_OK\fP and sets \f(CW*bytes_in_cie\fP to
the number of bytes in the portion of the
frames section for the CIE represented by the given \f(CWDwarf_Cie\fP
descriptor, \f(CWcie\fP.
The other fields are directly taken from
the cie and returned, via the pointers to the caller.
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_get_cie_index()"
.DS
\f(CWint dwarf_get_cie_index(
        Dwarf_Cie cie,
        Dwarf_Signed *cie_index,
        Dwarf_Error *error);\fP
.DE
On success,
\f(CWdwarf_get_cie_index()\fP returns
\f(CWDW_DLV_OK\fP.
On error this function returns \f(CWDW_DLV_ERROR\fP.

The location pointed to by \f(CWcie_index\fP
is set to the index of the CIE of this FDE.
The index is the
index of the CIE in the list pointed to by \f(CWcie_data\fP as set
by the function \f(CWdwarf_get_fde_list()\fP.

So one must have used \f(CWdwarf_get_fde_list()\fP or
\f(CWdwarf_get_fde_list_eh()\fP to get
a cie list before this is meaningful.

This function is occasionally useful, but is
little used.

.H 3 "dwarf_get_fde_instr_bytes()"
.DS
\f(CWint dwarf_get_fde_instr_bytes(
        Dwarf_Fde fde,
	Dwarf_Ptr *outinstrs,
	Dwarf_Unsigned *outlen,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_fde_instr_bytes()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*outinstrs\fP to
a pointer to a set of bytes which are the
actual frame instructions for this fde.
It also sets \f(CW*outlen\fP to the length, in
bytes, of the frame instructions.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
The intent is to allow low-level consumers like a dwarf-dumper
to print the bytes in some fashion.
The memory pointed to by \f(CWoutinstrs\fP
must not be changed and there
is nothing to free.

.H 3 "dwarf_get_fde_info_for_reg()"
This interface is suitable for DWARF2 but is not
sufficient for DWARF3.  See \f(CWint dwarf_get_fde_info_for_reg3\fP.
.DS
\f(CWint dwarf_get_fde_info_for_reg(
        Dwarf_Fde fde,
        Dwarf_Half table_column,
        Dwarf_Addr pc_requested,
	Dwarf_Signed *offset_relevant,
        Dwarf_Signed *register_num,
        Dwarf_Signed *offset,
        Dwarf_Addr *row_pc,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_fde_info_for_reg()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*offset_relevant\fP to
non-zero if the offset is relevant for the
row specified by \f(CWpc_requested\fP and column specified by
\f(CWtable_column\fP, for the FDE specified by \f(CWfde\fP.
The
intent is to return the rule for the given pc value and register.
The location pointed to by \f(CWregister_num\fP is set to the register
value for the rule.
The location pointed to by \f(CWoffset\fP
is set to the offset value for the rule.
If offset is not relevant for this rule, \f(CW*offset_relevant\fP is
set to zero.
Since more than one pc
value will have rows with identical entries, the user may want to
know the earliest pc value after which the rules for all the columns
remained unchanged.
Recall that in the virtual table that the frame information
represents there may be one or more table rows with identical data
(each such table row at a different pc value).
Given a \f(CWpc_requested\fP which refers to a pc in such a group
of identical rows,
the location pointed to by \f(CWrow_pc\fP is set
to the lowest pc value
within the group of  identical rows.
The  value put in \f(CW*register_num\fP any of the
\f(CWDW_FRAME_*\fP table columns values specified in \f(CWlibdwarf.h\fP
or \f(CWdwarf.h\fP.

\f(CWdwarf_get_fde_info_for_reg\fP returns \f(CWDW_DLV_ERROR\fP if there is an error.

It is usable with either
\f(CWdwarf_get_fde_n()\fP or \f(CWdwarf_get_fde_at_pc()\fP.

\f(CWdwarf_get_fde_info_for_reg()\fP is tailored to MIPS, please use
\f(CWdwarf_get_fde_info_for_reg3()\fP instead for all architectures.


.H 3 "dwarf_get_fde_info_for_all_regs()"
.DS
\f(CWint dwarf_get_fde_info_for_all_regs(
        Dwarf_Fde fde,
        Dwarf_Addr pc_requested,
	Dwarf_Regtable *reg_table,
        Dwarf_Addr *row_pc,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_get_fde_info_for_all_regs()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*reg_table\fP for the row specified by
\f(CWpc_requested\fP for the FDE specified by \f(CWfde\fP.
.P
The intent is
to return the rules for decoding all the registers, given a pc value.
\f(CWreg_table\fP is an array of rules, one for each register specified in
\f(CWdwarf.h\fP. The rule for each register contains three items -
\f(CWdw_regnum\fP which denotes the register value for that rule,
\f(CWdw_offset\fP which denotes the offset value for that rule and
\f(CWdw_offset_relevant\fP which is set to zero if offset is not relevant
for that rule. See \f(CWdwarf_get_fde_info_for_reg()\fP for a description
of \f(CWrow_pc\fP.
.P
\f(CWdwarf_get_fde_info_for_all_regs\fP returns \f(CWDW_DLV_ERROR\fP if there is an error.
.P
\f(CWint dwarf_get_fde_info_for_all_regs\fP is tailored to SGI/MIPS,
please use dwarf_get_fde_info_for_all_regs3() instead for all architectures.

.H 3 "dwarf_fde_section_offset()"
.DS
\f(CWint dwarf_fde_section_offset(
    Dwarf_Debug       /*dbg*/,
    Dwarf_Fde         /*in_fde*/,
    Dwarf_Off *       /*fde_off*/,
    Dwarf_Off *       /*cie_off*/,
    Dwarf_Error *error);\fP
.DE
On success
\f(CWdwarf_fde_section_offset()\fP returns
the .dwarf_line section offset of the fde passed in
and also the offset of its CIE.
.P
It returns \f(CWDW_DLV_ERROR\fP if there is an error.
.P
It returns \f(CWDW_DLV_ERROR\fP if there is an error.
.P It is intended to be used by applications like dwarfdump
when such want to print the offsets of CIEs and FDEs.

.H 3 "dwarf_cie_section_offset()"
.DS
\f(CWint  dwarf_cie_section_offset(
    Dwarf_Debug   /*dbg*/,
    Dwarf_Cie     /*in_cie*/,
    Dwarf_Off *   /*cie_off*/,
    Dwarf_Error * /*err*/);
    Dwarf_Error *error);\fP
.DE
On success
\f(CWdwarf_cie_section_offset()\fP returns
the .dwarf_line section offset of the cie passed in.
.P
It returns \f(CWDW_DLV_ERROR\fP if there is an error.
.P It is intended to be used by applications like dwarfdump
when such want to print the offsets of CIEs.



.H 3 "dwarf_set_frame_rule_table_size()"
.P
This allows consumers to set the size of the (internal to libdwarf)
rule table when using the 'reg3' interfaces (these interfaces
are strongly preferred over the older 'reg' interfaces).
It should be at least as large as the
number of real registers in the ABI which is to be read in
for the dwarf_get_fde_info_for_reg3() or dwarf_get_fde_info_for_all_regs3()
functions to work properly.

The frame rule table size must be less than the marker values
DW_FRAME_UNDEFINED_VAL, DW_FRAME_SAME_VAL, DW_FRAME_CFA_COL3
(dwarf_set_frame_rule_undefined_value()
dwarf_set_frame_same_value()
dwarf_set_frame_cfa_value()
effectively set these markers so
the frame rule table size can actually be any value regardless
of the macro values in libdwarf.h as long as the table size
does not overlap these markers).
.P
.DS
\f(CWDwarf_Half
dwarf_set_frame_rule_table_size(Dwarf_Debug dbg,
         Dwarf_Half value);\fP

.DE
\f(CWdwarf_set_frame_rule_table_size()\fP sets the
value \f(CWvalue\fP as the size of libdwarf-internal
rules tables  of \f(CWdbg\fP.
.P
The function returns
the previous value of the rules table size setting (taken from the
\f(CWdbg\fP structure).

.H 3 "dwarf_set_frame_rule_initial_value()"
This allows consumers to set the initial value
for rows in the frame tables.  By default it
is taken from libdwarf.h and is DW_FRAME_REG_INITIAL_VALUE
(which itself is either DW_FRAME_SAME_VAL or DW_FRAME_UNDEFINED_VAL).
The MIPS/IRIX default is DW_FRAME_SAME_VAL.
Consumer code should set this appropriately and for
many architectures (but probably not MIPS) DW_FRAME_UNDEFINED_VAL is an
appropriate setting.
Note: an earlier spelling of dwarf_set_frame_rule_inital_value()
is still supported as an interface, but please change to use
the new correctly spelled name.
.DS
\f(CWDwarf_Half
dwarf_set_frame_rule_initial_value(Dwarf_Debug dbg,
         Dwarf_Half value);\fP

.DE
\f(CWdwarf_set_frame_rule_initial_value()\fP sets the
value \f(CWvalue\fP as the initial value for this \f(CWdbg\fP
when initializing rules tables.
.P
The function returns
the previous value of initial value (taken from the
\f(CWdbg\fP structure).

.H 3 "dwarf_set_frame_cfa_value()"
This allows consumers to set the number of the CFA register
for rows in the frame tables.  By default it
is taken from libdwarf.h and is \f(CWDW_FRAME_CFA_COL\fP.
Consumer code should set this appropriately and for
nearly all architectures \f(CWDW_FRAME_CFA_COL3\fP is an
appropriate setting.
.DS
\f(CWDwarf_Half
dwarf_set_frame_rule_cfa_value(Dwarf_Debug dbg,
         Dwarf_Half value);\fP

.DE
\f(CWdwarf_set_frame_rule_cfa_value()\fP sets the
value \f(CWvalue\fP as the  number of the cfa 'register rule'
for this \f(CWdbg\fP
when initializing rules tables.
.P
The function returns
the previous value of the pseudo-register (taken from the
\f(CWdbg\fP structure).

.H 3 "dwarf_set_frame_same_value()"
This allows consumers to set the number of the pseudo-register
when DW_CFA_same_value is the operation.  By default it
is taken from libdwarf.h and is \f(CWDW_FRAME_SAME_VAL\fP.
Consumer code should set this appropriately, though for
many architectures \f(CWDW_FRAME_SAME_VAL\fP is an
appropriate setting.
.DS
\f(CWDwarf_Half
dwarf_set_frame_rule_same_value(Dwarf_Debug dbg,
         Dwarf_Half value);\fP

.DE
\f(CWdwarf_set_frame_rule_same_value()\fP sets the
value \f(CWvalue\fP as the  number of the register
that is the pseudo-register set by the DW_CFA_same_value
frame operation.
.P
The function returns
the previous value of the pseudo-register  (taken from the
\f(CWdbg\fP structure).


.H 3 "dwarf_set_frame_undefined_value()"
This allows consumers to set the number of the pseudo-register
 when DW_CFA_undefined_value is the operation.  By default it
is taken from libdwarf.h and is \f(CWDW_FRAME_UNDEFINED_VAL\fP.
Consumer code should set this appropriately, though for
many architectures \f(CWDW_FRAME_UNDEFINED_VAL\fP is an
appropriate setting.
.DS
\f(CWDwarf_Half
dwarf_set_frame_rule_undefined_value(Dwarf_Debug dbg,
         Dwarf_Half value);\fP

.DE
\f(CWdwarf_set_frame_rule_undefined_value()\fP sets the
value \f(CWvalue\fP as the  number of the register
that is the pseudo-register set by the DW_CFA_undefined_value
frame operation.
.P
The function returns
the previous value of the pseudo-register  (taken from the
\f(CWdbg\fP structure).

.H 3 "dwarf_set_default_address_size()"
This allows consumers to set a default address size.
When one has an object where the
default address_size does not match the frame address
size where there is no debug_info available to get a
frame-specific address-size, this function is useful.
For example, if an Elf64 object has a .debug_frame whose
real address_size is 4 (32 bits).  This a very rare
situation.
.DS
\f(CWDwarf_Small
dwarf_set_default_address_size(Dwarf_Debug dbg,
         Dwarf_Small value);\fP

.DE
\f(CWdwarf_set_default_address_size()\fP sets the
value \f(CWvalue\fP as the  default address size for
this activation of the reader, but only if \f(CWvalue\fP
is greater than zero (otherwise the default address size
is not changed).
.P
The function returns
the previous value of the default address size  (taken from the
\f(CWdbg\fP structure).



.H 3 "dwarf_get_fde_info_for_reg3()"
This interface is suitable for DWARF2 and later.
It returns the values for a particular real register
(Not for the CFA virtual register,
see dwarf_get_fde_info_for_cfa_reg3()
below).
If the application is going to retrieve the value for more
than a few \f(CWtable_column\fP values at this \f(CWpc_requested\fP
(by calling this function multiple times)
it is much more efficient to
call dwarf_get_fde_info_for_all_regs3() (in spite
of the additional setup that requires of the caller).

.DS
\f(CWint dwarf_get_fde_info_for_reg3(
        Dwarf_Fde fde,
        Dwarf_Half table_column,
        Dwarf_Addr pc_requested,
	Dwarf_Small  *value_type,
	Dwarf_Signed *offset_relevant,
        Dwarf_Signed *register_num,
        Dwarf_Signed *offset_or_block_len,
	Dwarf_Ptr    *block_ptr,
        Dwarf_Addr   *row_pc,
        Dwarf_Error  *error);\fP
.DE
See also the nearly identical function
\f(CWdwarf_get_fde_info_for_reg3_b().
.P
\f(CWdwarf_get_fde_info_for_reg3()\fP returns
\f(CWDW_DLV_OK\fP on success.
It sets \f(CW*value_type\fP
to one of  DW_EXPR_OFFSET (0),
DW_EXPR_VAL_OFFSET(1), DW_EXPR_EXPRESSION(2) or
DW_EXPR_VAL_EXPRESSION(3).
On call, \f(CWtable_column\fP must be set to the
register number of a real register. Not
the cfa 'register' or DW_FRAME_SAME_VALUE or
DW_FRAME_UNDEFINED_VALUE.


if \f(CW*value_type\fP has the value DW_EXPR_OFFSET (0) then:
.in +4
.P
It sets \f(CW*offset_relevant\fP to
non-zero if the offset is relevant for the
row specified by \f(CWpc_requested\fP and column specified by
\f(CWtable_column\fP or, for the FDE specified by \f(CWfde\fP.
In this case  the  \f(CW*register_num\fP will be set
to DW_FRAME_CFA_COL3 (.  This is an offset(N) rule
as specified in the DWARF3/2 documents.
.P
Adding the value of \f(CW*offset_or_block_len\fP
to the value of the CFA register gives the address
of a location holding the previous value of
register \f(CWtable_column\fP.

.P
If offset is not relevant for this rule, \f(CW*offset_relevant\fP is
set to zero.  \f(CW*register_num\fP will be set
to the number of the real register holding the value of
the \f(CWtable_column\fP register.
This is the register(R) rule as specified in DWARF3/2 documents.
.P
The
intent is to return the rule for the given pc value and register.
The location pointed to by \f(CWregister_num\fP is set to the register
value for the rule.
The location pointed to by \f(CWoffset\fP
is set to the offset value for the rule.
Since more than one pc
value will have rows with identical entries, the user may want to
know the earliest pc value after which the rules for all the columns
remained unchanged.
Recall that in the virtual table that the frame information
represents there may be one or more table rows with identical data
(each such table row at a different pc value).
Given a \f(CWpc_requested\fP which refers to a pc in such a group
of identical rows,
the location pointed to by \f(CWrow_pc\fP is set
to the lowest pc value
within the group of  identical rows.

.in -4

.P
If \f(CW*value_type\fP has the value DW_EXPR_VAL_OFFSET (1) then:
.in +4
This will be a val_offset(N) rule as specified in the
DWARF3/2 documents so  \f(CW*offset_relevant\fP will
be non zero.
The calculation is identical to the  DW_EXPR_OFFSET (0)
calculation with  \f(CW*offset_relevant\fP non-zero,
but the value  resulting is the actual \f(CWtable_column\fP
value (rather than the address where the value may be found).
.in -4
.P
If \f(CW*value_type\fP has the value DW_EXPR_EXPRESSION (1) then:
.in +4
 \f(CW*offset_or_block_len\fP
is set to the length in bytes of a block of memory
with a DWARF expression in the block.
\f(CW*block_ptr\fP is set to point at the block of memory.
The consumer code should  evaluate the block as
a DWARF-expression. The result is the address where
the previous value of the register may be found.
This is a DWARF3/2 expression(E) rule.
.in -4
.P
If \f(CW*value_type\fP has the value DW_EXPR_VAL_EXPRESSION (1) then:
.in +4
The calculation is exactly as for DW_EXPR_EXPRESSION (1)
but the result of the DWARF-expression evaluation is
the value of the   \f(CWtable_column\fP (not
the address of the value).
This is a DWARF3/2 val_expression(E) rule.
.in -4

\f(CWdwarf_get_fde_info_for_reg\fP
returns \f(CWDW_DLV_ERROR\fP if there is an error and
if there is an error only the \f(CWerror\fP pointer is set, none
of the other output arguments are touched.

It is usable with either
\f(CWdwarf_get_fde_n()\fP or \f(CWdwarf_get_fde_at_pc()\fP.

.H 3 "dwarf_get_fde_info_for_reg3_b()"
This interface is suitable for DWARF2 and later.
It returns the values for a particular real register
(Not for the CFA virtual register,
see dwarf_get_fde_info_for_cfa_reg3_b()
below).
If the application is going to retrieve the value for more
than a few \f(CWtable_column\fP values at this \f(CWpc_requested\fP
(by calling this function multiple times)
it is much more efficient to
call dwarf_get_fde_info_for_all_regs3() (in spite
of the additional setup that requires of the caller).

.DS
\f(CWint dwarf_get_fde_info_for_reg3_b(
        Dwarf_Fde fde,
        Dwarf_Half table_column,
        Dwarf_Addr pc_requested,
        Dwarf_Small  *value_type,
        Dwarf_Signed *offset_relevant,
        Dwarf_Signed *register_num,
        Dwarf_Signed *offset_or_block_len,
        Dwarf_Ptr    *block_ptr,
        Dwarf_Addr   *row_pc,
        Dwarf_Bool   *has_more_rows,
        Dwarf_Addr   *subsequent_pc,
        Dwarf_Error  *error);\fP
.DE
.P
This is identical to
\f(CWdwarf_get_fde_info_for_reg3()\fP
except for the new arguments
\f(CWhas_more_rows\fP
and
\f(CWsubsequent_pc\fP
which allow the caller to
know if there are more rows in the frame table
and what the next pc value in the frame table
for this fde is.
The two new arguments may be passed in as NULL
if their values are not needed by the caller.


.H 3 "dwarf_get_fde_info_for_cfa_reg3()"
.DS
 \f(CWint dwarf_get_fde_info_for_cfa_reg3(Dwarf_Fde fde,
      Dwarf_Addr          pc_requested,
      Dwarf_Small *       value_type,
      Dwarf_Signed*       offset_relevant,
      Dwarf_Signed*       register_num,
      Dwarf_Signed*       offset_or_block_len,
      Dwarf_Ptr   *       block_ptr ,
      Dwarf_Addr  *       row_pc_out,
      Dwarf_Error *       error)\fP
.DE
.P
This is identical to
\f(CWdwarf_get_fde_info_for_reg3()\fP
except the returned values are for the CFA rule.
So register number
\f(CW*register_num\fP
will be set
to a real register, not one of the pseudo registers
(which are usually
DW_FRAME_CFA_COL3, DW_FRAME_SAME_VALUE, or
DW_FRAME_UNDEFINED_VALUE).
.P
Applications like dwarfdump
which access the register rules for every pc
value in a function may find the following
function a slight performance improvement
if the new arguments are used appropriately.
See \f(CWdwarfdump\fP for an example of use.

.H 3 "dwarf_get_fde_info_for_cfa_reg3_b()"
.DS
 \f(CWint dwarf_get_fde_info_for_cfa_reg3_b(Dwarf_Fde fde,
      Dwarf_Addr          pc_requested,
      Dwarf_Small *       value_type,
      Dwarf_Signed*       offset_relevant,
      Dwarf_Signed*       register_num,
      Dwarf_Signed*       offset_or_block_len,
      Dwarf_Ptr   *       block_ptr ,
      Dwarf_Addr  *       row_pc_out,
      Dwarf_Bool  *       has_more_rows,
      Dwarf_Addr  *       subsequent_pc,
      Dwarf_Error *       error)\fP
.DE
.P
This is identical to
\f(CWdwarf_get_fde_info_for_cfa_reg3()\fP
except for the new arguments
\f(CWhas_more_rows\fP
and
\f(CWsubsequent_pc\fP
which allow the caller to
know if there are more rows in the frame table
and what the next pc value is.
The two new arguments may be passed in as NULL
if their values are not needed by the caller.
.P
For a tool just wanting the frame information for a single
pc_value this interface is no more useful nore more
efficient than
\f(CWdwarf_get_fde_info_for_cfa_reg3()\fP.
.P
The essential difference is that when using
\f(CWdwarf_get_fde_info_for_cfa_reg3()\fP
for all pc values for a function the caller
has no idea what is the next pc value that might
have new frame data and iterating through
pc values (calling
\f(CWdwarf_get_fde_info_for_cfa_reg3()\fP on each)
is a waste of cpu cycles.
With
\f(CWdwarf_get_fde_info_for_cfa_reg3_b()\fP
the
\f(CWhas_more_rows\fP
and
\f(CWsubsequent_pc\fP
arguments let the caller know whether there
are further changes and if so at what pc value.

.P
If
\f(CWhas_more_rows\fP
is non-null
then 0 is returned through the pointer
if, for the
\f(CWpc_requested\fP
there is frame data for addresses after
\f(CWpc_requested\fP
in the frame.
And if there are no more rows in the frame data then
1 is set through the
\f(CWhas_more_rows\fP pointer.

.P
If
\f(CWsubsequent_pc\fP
is non-null
then the pc-value which has the next
frame operator is returned through the
pointer. If no more rows are present
zero is returned through the pointer, but
please use
\f(CWhas_more_rows\fP
to determine if there are more rows.

.H 3 "dwarf_get_fde_info_for_all_regs3()"
.DS
\f(CWint dwarf_get_fde_info_for_all_regs3(
        Dwarf_Fde fde,
        Dwarf_Addr pc_requested,
	Dwarf_Regtable3 *reg_table,
        Dwarf_Addr *row_pc,
        Dwarf_Error *error)\fP
.DE
\f(CWdwarf_get_fde_info_for_all_regs3()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*reg_table\fP
for the row specified by
\f(CWpc_requested\fP for the FDE specified by \f(CWfde\fP.
The intent is
to return the rules for decoding all the registers, given a pc
value.
\f(CWreg_table\fP is an array of rules, the
array size specified by the caller.
plus a rule for the CFA.
The rule for the cfa returned in  \f(CW*reg_table\fP
defines the CFA value at  \f(CWpc_requested\fP
The rule for each
register contains  several values that enable
the consumer to determine the previous value
of the register (see the earlier documentation of Dwarf_Regtable3).
\f(CWdwarf_get_fde_info_for_reg3()\fP  and
the Dwarf_Regtable3 documentation above for a description of
the values for each row.

\f(CWdwarf_get_fde_info_for_all_regs3\fP returns \f(CWDW_DLV_ERROR\fP if there is an error.

It is up to the caller to allocate space for
\f(CW*reg_table\fP and initialize it properly.



.H 3 "dwarf_get_fde_n()"
.DS
\f(CWint   dwarf_get_fde_n(
        Dwarf_Fde *fde_data,
        Dwarf_Unsigned fde_index,
	Dwarf_Fde      *returned_fde
        Dwarf_Error *error)\fP
.DE
\f(CWdwarf_get_fde_n()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CWreturned_fde\fP to
the \f(CWDwarf_Fde\fP descriptor whose
index is \f(CWfde_index\fP in the table of \f(CWDwarf_Fde\fP descriptors
pointed to by \fPfde_data\fP.
The index starts with 0.
The table pointed to by fde_data is required to contain
at least one entry. If the table has no entries at all
the error checks may refer to uninitialized memory.
Returns \f(CWDW_DLV_NO_ENTRY\fP if the index does not
exist in the table of \f(CWDwarf_Fde\fP
descriptors.
Returns \f(CWDW_DLV_ERROR\fP if there is an error.
This function cannot be used unless
the block of \f(CWDwarf_Fde\fP descriptors has been created by a call to
\f(CWdwarf_get_fde_list()\fP.

.H 3 "dwarf_get_fde_at_pc()"
.DS
\f(CWint   dwarf_get_fde_at_pc(
        Dwarf_Fde *fde_data,
        Dwarf_Addr pc_of_interest,
        Dwarf_Fde *returned_fde,
        Dwarf_Addr *lopc,
        Dwarf_Addr *hipc,
        Dwarf_Error *error)\fP
.DE
\f(CWdwarf_get_fde_at_pc()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CWreturned_fde\fP to
a \f(CWDwarf_Fde\fP descriptor
for a function which contains the pc value specified by \f(CWpc_of_interest\fP.
In addition, it sets the locations pointed to
by \f(CWlopc\fP and \f(CWhipc\fP to the low address and the high address
covered by this FDE, respectively.
The table pointed to by fde_data is required to contain
at least one entry. If the table has no entries at all
the error checks may refer to uninitialized memory.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP
if \f(CWpc_of_interest\fP is not in any of the
FDEs represented by the block of \f(CWDwarf_Fde\fP descriptors pointed
to by \f(CWfde_data\fP.
This function cannot be used unless
the block of \f(CWDwarf_Fde\fP descriptors has been created by a call to
\f(CWdwarf_get_fde_list()\fP.

.H 3 "dwarf_expand_frame_instructions()"
.DS
\f(CWint dwarf_expand_frame_instructions(
        Dwarf_Cie cie,
        Dwarf_Ptr instruction,
        Dwarf_Unsigned i_length,
        Dwarf_Frame_Op **returned_op_list,
        Dwarf_Signed   * returned_op_count,
        Dwarf_Error *error);\fP
.DE
\f(CWdwarf_expand_frame_instructions()\fP is a High-level interface
function which expands a frame instruction byte stream into an
array of \f(CWDwarf_Frame_Op\fP structures.
To indicate success, it returns \f(CWDW_DLV_OK\fP.
The address where
the byte stream begins is specified by \f(CWinstruction\fP, and
the length of the byte stream is specified by \f(CWi_length\fP.
The location pointed to by \f(CWreturned_op_list\fP is set to
point to a table of
\f(CWreturned_op_count\fP
pointers to \f(CWDwarf_Frame_Op\fP which
contain the frame instructions in the byte stream.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.
After a successful return, the
array of structures should be freed using
\f(CWdwarf_dealloc()\fP with the allocation type \f(CWDW_DLA_FRAME_BLOCK\fP
(when they are no longer of interest).
.P
Not all CIEs have the same address-size, so it is crucial
that a CIE pointer to the frame's CIE be passed in.

.in +2
.FG "Examples dwarf_expand_frame_instructions()"
.DS
\f(CW
void examples(Dwarf_Debug dbg,Dwarf_Cie cie,
    Dwarf_Ptr instruction,Dwarf_Unsigned len)
{
    Dwarf_Signed count = 0;
    Dwarf_Frame_Op *frameops = 0;
    Dwarf_Error error = 0;
    int res = 0;

    res = dwarf_expand_frame_instructions(cie,instruction,len,
        &frameops,&count, &error);
    if (res == DW_DLV_OK) {
        Dwarf_Signed i = 0;

        for (i = 0; i < count; ++i) {
            /* use frameops[i] */
        }
        dwarf_dealloc(dbg, frameops, DW_DLA_FRAME_BLOCK);
    }
}
\fP
.DE
.in -2
.H 3 "dwarf_get_fde_exception_info()"
.DS
\f(CWint dwarf_get_fde_exception_info(
    Dwarf_Fde fde,
    Dwarf_Signed * offset_into_exception_tables,
    Dwarf_Error * error);
.DE
\f(CWdwarf_get_fde_exception_info()\fP is an IRIX specific
function which returns an exception table signed offset
through \f(CWoffset_into_exception_tables\fP.
The function never returns \f(CWDW_DLV_NO_ENTRY\fP.
If \f(CWDW_DLV_NO_ENTRY\fP is NULL the function returns
\f(CWDW_DLV_ERROR\fP.
For non-IRIX objects the offset returned will always be zero.
For non-C++ objects the offset returned will always be zero.
The meaning of the offset and the content of the tables
is not defined in this document.
The applicable CIE augmentation string (see above)
determines whether the value returned has meaning.

.H 2 "Location Expression Evaluation"

An "interpreter" which evaluates a location expression
is required in any debugger.  There is no interface defined
here at this time.

.P
One problem with defining an interface is that operations are
machine dependent: they depend on the interpretation of
register numbers and the methods of getting values from the
environment the expression is applied to.

.P
It would be desirable to specify an interface.

.H 3 "Location List Internal-level Interface"

.H 4 "dwarf_get_loclist_entry()"
.DS
\f(CWint dwarf_get_loclist_entry(
        Dwarf_Debug dbg,
        Dwarf_Unsigned offset,
        Dwarf_Addr *hipc_offset,
        Dwarf_Addr *lopc_offset,
        Dwarf_Ptr *data,
        Dwarf_Unsigned *entry_len,
        Dwarf_Unsigned *next_entry,
        Dwarf_Error *error)\fP
.DE
This function is ill suited to use with
21st century DWARF as there is just not
enough data provided in the interface.
Do not use this interface.
.P
The function reads
a location list entry starting at \f(CWoffset\fP and returns
through pointers (when successful)
the high pc \f(CWhipc_offset\fP, low pc
\f(CWlopc_offset\fP, a pointer to the location description data
\f(CWdata\fP, the length of the location description data
\f(CWentry_len\fP, and the offset of the next location description
entry \f(CWnext_entry\fP.
.P
This function will often work correctly (meaning with most
objects compiled for DWARF3 or DWARF3)
but will not work correctly (and can crash
an application calling it) if either
some location list applies to a compilation unit with
an address_size different from the overall address_size
of the object file being read or if the .debug_loc section
being read has random padding bytes between loclists.
Neither of these characteristics necessarily represents
a bug in the compiler/linker toolset that produced the
object file being read.  The DWARF standard
allows both characteristics.
.P
\f(CWdwarf_dwarf_get_loclist_entry()\fP returns
\f(CWDW_DLV_OK\fP if successful.
\f(CWDW_DLV_NO_ENTRY\fP is returned when the offset passed
in is beyond the end of the .debug_loc section (expected if
you start at offset zero and proceed through all the entries).
\f(CWDW_DLV_ERROR\fP is returned on error.
.P
The \f(CWhipc_offset\fP,
low pc \f(CWlopc_offset\fP are offsets from the beginning of the
current procedure, not genuine pc values.
.in +2
.FG "Examples dwarf_get_loclist_entry()"
.DS
\f(CW
void examplet(Dwarf_Debug dbg,Dwarf_Unsigned offset)
{
    /*  Looping through the dwarf_loc section finding loclists:
        an example.  */
    int res;
    Dwarf_Unsigned next_entry = 0;
    Dwarf_Addr hipc_off = 0;
    Dwarf_Addr lowpc_off = 0;
    Dwarf_Ptr data = 0;
    Dwarf_Unsigned entry_len = 0;
    Dwarf_Error err = 0;

    for(;;) {
        res = dwarf_get_loclist_entry(dbg,offset,&hipc_off,
            &lowpc_off, &data, &entry_len,&next_entry,&err);
        if (res == DW_DLV_OK) {
            /* A valid entry. */
            offset = next_entry;
            continue;
        } else if (res ==DW_DLV_NO_ENTRY) {
            /* Done! */
            break;
        } else {
            /* Error! */
            break;
        }
    }
}
\fP
.DE
.in -2


.H 2 "Abbreviations access"
These are Internal-level Interface functions.
Debuggers can ignore this.

.H 3 "dwarf_get_abbrev()"
.DS
\f(CWint dwarf_get_abbrev(
        Dwarf_Debug dbg,
        Dwarf_Unsigned offset,
        Dwarf_Abbrev   *returned_abbrev,
        Dwarf_Unsigned *length,
        Dwarf_Unsigned *attr_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_abbrev()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_abbrev\fP to
\f(CWDwarf_Abbrev\fP
descriptor for an abbreviation at offset \f(CW*offset\fP in the abbreviations
section (i.e .debug_abbrev) on success.
The user is responsible for making sure that
a valid abbreviation begins at \f(CWoffset\fP in the abbreviations section.
The location pointed to by \f(CWlength\fP
is set to the length in bytes of the abbreviation in the abbreviations
section.
The location pointed to by \f(CWattr_count\fP is set to the
number of attributes in the abbreviation.
An abbreviation entry with a
length of 1 is the 0 byte of the last abbreviation entry of a compilation
unit.
\f(CWdwarf_get_abbrev()\fP returns \f(CWDW_DLV_ERROR\fP on error.
If the call succeeds, the storage pointed to
by \f(CW*returned_abbrev\fP
should be freed, using \f(CWdwarf_dealloc()\fP with the
allocation type \f(CWDW_DLA_ABBREV\fP when no longer needed.


.H 3 "dwarf_get_abbrev_tag()"
.DS
\f(CWint dwarf_get_abbrev_tag(
        Dwarf_abbrev abbrev,
	Dwarf_Half  *return_tag,
        Dwarf_Error *error);\fP
.DE
If successful,
\f(CWdwarf_get_abbrev_tag()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_tag\fP to
the \fItag\fP of
the given abbreviation.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_get_abbrev_code()"
.DS
\f(CWint dwarf_get_abbrev_code(
        Dwarf_abbrev     abbrev,
	Dwarf_Unsigned  *return_code,
        Dwarf_Error     *error);\fP
.DE
If successful,
\f(CWdwarf_get_abbrev_code()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*return_code\fP to
the abbreviation code of
the given abbreviation.
It returns \f(CWDW_DLV_ERROR\fP on error.
It never returns \f(CWDW_DLV_NO_ENTRY\fP.

.H 3 "dwarf_get_abbrev_children_flag()"
.DS
\f(CWint dwarf_get_abbrev_children_flag(
        Dwarf_Abbrev abbrev,
	Dwarf_Signed  *returned_flag,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_abbrev_children_flag()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CWreturned_flag\fP to
\f(CWDW_children_no\fP (if the given abbreviation indicates that
a die with that abbreviation has no children) or
\f(CWDW_children_yes\fP (if the given abbreviation indicates that
a die with that abbreviation has a child).
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_get_abbrev_entry()"
.DS
\f(CWint dwarf_get_abbrev_entry(
        Dwarf_Abbrev abbrev,
        Dwarf_Signed index,
        Dwarf_Half   *attr_num,
        Dwarf_Signed *form,
        Dwarf_Off *offset,
        Dwarf_Error *error)\fP

.DE
If successful,
\f(CWdwarf_get_abbrev_entry()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*attr_num\fP to the attribute code of
the attribute
whose index is specified by \f(CWindex\fP in the given abbreviation.
The index starts at 0.
The location pointed to by \f(CWform\fP is set
to the form of the attribute.
The location pointed to by \f(CWoffset\fP
is set to the byte offset of the attribute in the abbreviations section.
It returns \f(CWDW_DLV_NO_ENTRY\fP if the index specified is outside
the range of attributes in this abbreviation.
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 2 "String Section Operations"
The .debug_str section contains only strings.  Debuggers need
never use this interface: it is only for debugging problems with
the string section itself.

.H 3 "dwarf_get_string_section_name()"
.DS
\f(CWint dwarf_get_string_section_name(Dwarf_Debug dbg,
    const char ** sec_name,
    Dwarf_Error  *error)\fP
.DE
\f(CWdwarf_get_string_section_name()\fP lets consumers
access the object string section name.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
See  also \f(CWdwarf_get_die_section_name_b()\fP.
.P
The function
\f(CWdwarf_get_string_section_name()\fP operates on
the the .debug_string[.dwo] section.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.


.H 3 "dwarf_get_str()"
.DS
\f(CWint dwarf_get_str(
        Dwarf_Debug   dbg,
        Dwarf_Off     offset,
        char        **string,
	Dwarf_Signed *returned_str_len,
        Dwarf_Error  *error)\fP
.DE
The function \f(CWdwarf_get_str()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_str_len\fP to
the length of
the string, not counting the null terminator, that begins at the offset
specified by \f(CWoffset\fP in the .debug_str section.
The location
pointed to by \f(CWstring\fP is set to a pointer to this string.
The next string in the .debug_str
section begins at the previous \f(CWoffset\fP + 1 + \f(CW*returned_str_len\fP.
A zero-length string is NOT the end of the section.
If there is no .debug_str section, \f(CWDW_DLV_NO_ENTRY\fP is returned.
If there is an error, \f(CWDW_DLV_ERROR\fP is returned.
If we are at the end of the section (that is, \f(CWoffset\fP
is one past the end of the section) \f(CWDW_DLV_NO_ENTRY\fP is returned.
If the \f(CWoffset\fP is some other too-large value then
\f(CWDW_DLV_ERROR\fP is returned.

.H 2 "String Offsets Section Operations"
The .debug_str_offsets section contains only
table arrays (with headers) and Debuggers should never
need to use this interface.
The normal string access functions use the section tables
transparently.
The functions here are only intended
to allow dwarfdump (or the like) print the section
completely and to help compiler developers look
for bugs in the section.

.in +2
.FG "examplestringoffsets dwarf_open_str_offsets_table_access() etc"
.DS
\f(CW
void examplestringoffsets(Dwarf_Debug dbg)
{
    int res = 0;
    Dwarf_Str_Offsets_Table sot = 0;
    Dwarf_Unsigned wasted_byte_count = 0;
    Dwarf_Unsigned table_count = 0;
    Dwarf_Error error = 0;

    res = dwarf_open_str_offsets_table_access(dbg, &sot,&error);
    if(res == DW_DLV_NO_ENTRY) {
        /* No such table */
        return;
    }
    if(res == DW_DLV_ERROR) {
        /* Something is very wrong. Print the error? */
        return;
    }
    for(;;) {
        Dwarf_Unsigned unit_length =0;
        Dwarf_Unsigned unit_length_offset =0;
        Dwarf_Unsigned table_start_offset =0;
        Dwarf_Half     entry_size = 0;
        Dwarf_Half     version =0;
        Dwarf_Half     padding =0;
        Dwarf_Unsigned table_value_count =0;
        Dwarf_Unsigned i = 0;
        Dwarf_Unsigned table_entry_value = 0;

        res = dwarf_next_str_offsets_table(sot,
            &unit_length, &unit_length_offset,
            &table_start_offset,
            &entry_size,&version,&padding,
            &table_value_count,&error);
        if (res == DW_DLV_NO_ENTRY) {
            /* We have dealt with all tables */
            break;
        }
        if (res == DW_DLV_ERROR) {
            /* Something badly wrong. Do something. */
            return;
        }
        /*  One could call dwarf_str_offsets_statistics to
            get the wasted bytes so far, but we do not do that
            in this example. */
        /*  Possibly print the various table-related values
            returned just above. */
        for (i=0; i < table_value_count; ++i) {
            res = dwarf_str_offsets_value_by_index(sot,i,
                &table_entry_value,&error);
            if (res != DW_DLV_OK) {
                /* Something is badly wrong. Do something. */
                return;
            }
            /*  Do something with the table_entry_value
                at this index. Maybe just print it.
                It is an offset in .debug_str. */
        }
    }
    res = dwarf_str_offsets_statistics(sot,&wasted_byte_count,
        &table_count,&error);
    if (res == DW_DLV_OK) {
        /*  The wasted byte count is set. Print it or something.
            One hopes zero bytes are wasted.
            Print the table count if one is interested. */
    }
    res = dwarf_close_str_offsets_table_access(sot,&error);
    /*  There is little point in checking the return value
        as little can be done about any error. */
    sot = 0;
}
\fP
.DE
.in -2


.H 3 "dwarf_open_str_offsets_table_access()"
.DS
\f(CWint dwarf_open_str_offsets_table_access(
    Dwarf_Debug dbg,
    Dwarf_Str_Offsets_Table * table_data,
    Dwarf_Error             * error);\fP
.DE
\f(CWdwarf_open_str_offsets_table_access()\fP
creates an opaque struct and returns a pointer to
it on success.  That struct pointer is used
in all subsequent operations on the table.
Through the function
\f(CWdwarf_next_str_offsets_table()\fP
the caller can iterate through each of the
per-CU offset tables.
.P
If there is no such section, or if the section
is empty the function returns DW_DLV_NO_ENTRY.
.P
If there is an error (such as out-of-memory)
the function returns DW_DLV_ERROR and sets
an error value through the
\f(CWerror\fP
pointer.

.H 3 "dwarf_close_str_offsets_table_access()"
.DS
\f(CWint
dwarf_close_str_offsets_table_access(
    Dwarf_Str_Offsets_Table  table_data,
    Dwarf_Error             * error);\fP
.DE
On success,
\f(CWdwarf_close_str_offsets_table_access()\fP
frees any allocated data associated with the
struct pointed to by
\f(CWtable_data\fP
and returns DW_DLV_OK.
It is up to the caller to set the
\f(CWtable_data\fP
pointer to NULL if desired. The pointer
is unusable at that point and any other
calls to libdwarf using that pointer will fail.
.P
It returns DW_DLV_OK on error.
Any error suggests there is memory corruption
or an error in the call.
Something serious happened.
.P
It never returns DW_DLV_NO_ENTRY, but if it did
there would be nothing the caller could do anyway..
.P
If one forgets to call this function
the memory allocated will be freed automatically
by to call to
\f(CWdwarf_finish()\fP,
as is true of all other data allocated by libdwarf.

.H 3 "dwarf_next_str_offsets_table()"
.DS
\f(CWint dwarf_next_str_offsets_table(
    Dwarf_Str_Offsets_Table table,
    Dwarf_Unsigned *unit_length_out,
    Dwarf_Unsigned *unit_length_offset_out,
    Dwarf_Unsigned *table_start_offset_out,
    Dwarf_Half     *entry_size_out,
    Dwarf_Half     *version_out,
    Dwarf_Half     *padding_out,
    Dwarf_Unsigned *table_value_count_out,
    Dwarf_Error    * error);\fP
.DE
Each call to
\f(CWdwarf_next_str_offsets_table()\fP
returns the next String Offsets table
in the .debug_str_offsets section.
Typically there would be one such table
for each CU in .debug_info[.dwo] contributing
to .debug_str_offsets.
The
\f(CWtable\fP
contains (internally, hidden) the section offset
of the next table.
.P
On success it returns DW_DLV_OK and
sets various fields representing data about
the current table (fields described below).
.P
If there are no more tables it returns
DW_DLV_NO_ENTRY.
.P
On error it returns DW_DLV_ERROR and passes back
error details through the
\f(CWerror\fP
pointer.
.P
The returned values are intended to let the caller
understand the table header and the table in detail.
These pointers are only used if the call returned
DW_DLV_OK.
.P
\f(CWunit_length_out\fP
is set to the unit_length of
a String Offsets Table Header.
Which means it gives the length,
in bytes, of the data following
the length value that belongs
to this table.
.P
\f(CWunit_length_offset_out\fP
is set to the section offset of
the table header.
.P
\f(CWtable_start_offset_out\fP
is set to the section offset of
the array of offsets in this table.
.P
\f(CWentry_size_out\fP
is set to the size of a table entry.
Which is 4 for 32-bit offsets
in this table
and 8 for 64-bit offsets in this table.
.P
\f(CWversion_out\fP
is set to the version number in the
table header.
The only current valid value is 5.
.P
\f(CWpadding_out\fP
is set to the 16-bit padding value
in the table header.
In a correct table header the value is zero.
.P
\f(CWtable_value_count_out\fP
is set to the number of entries in
the array of offsets in this table.
Each entry is
\f(CWentry_size_out\fP
bytes long.
Use this value in calling
\f(CWdwarf_str_offsets_value_by_index()\fP.
.H 3 "dwarf_str_offsets_value_by_index()"
.DS
\f(CWint dwarf_str_offsets_value_by_index(
    Dwarf_Str_Offsets_Table sot,
    Dwarf_Unsigned index,
    Dwarf_Unsigned *stroffset,
    Dwarf_Error *error);\fP
.DE
On success,
\f(CWdwarf_str_offsets_value_by_index()\fP
returns DW_DLV_OK and sets
the offset from the array of string
offsets in the current table at the input
\f(CWindex\fP.
.P
Valid index values are zero through
\f(CWtable_value_count_out - 1\fP
.P
A function is used instead of simply
letting callers use pointers
as libdwarf correctly handles
endianness differences
(between the system running libdwarf
and the object file being inspected)
so offsets
can be reported properly.
.P
DW_DLV_ERROR is returned on error.
.P
DW_DLV_NO_ENTRY is never returned.

.H 3 "dwarf_str_offsets_statistics()"
.DS
\f(CWint dwarf_str_offsets_statistics(
    Dwarf_Str_Offsets_Table table_data,
    Dwarf_Unsigned * wasted_byte_count,
    Dwarf_Unsigned * table_count,
    Dwarf_Error    * error);\fP
.DE
Normally called after all tables have been inspected to
return (through a pointer)
the count of apparently-wasted bytes in the section.
It can be called at any point that the
\f(CWDwarf_Str_Offsets_Table\fP
pointer is valid.
.P
On error it returns DW_DLV_ERROR and sets
an error value through the pointer.
.P
DW_DLV_NO_ENTRY is never returned.
.P
On success it returns DW_DLV_OK
and sets values through the two pointers.
Calling just after each table is accessed by
\f(CWdwarf_next_str_offsets_table()\fP
will reveal the sum of all wasted bytes at
that point in iterating through the section.
.P
\f(CWtable_count\fP
is the count of table headers encountered so far.
.P
By wasted bytes we mean bytes in between tables.
libdwarf has no idea whether any
apparently-valid table data
is in fact useless.

.H 2 "Address Range Operations"
These functions provide information about address ranges.  Address
ranges map ranges of pc values to the corresponding compilation-unit
die that covers the address range.

.H 3 "dwarf_get_aranges_section_name()"
.DS
\f(CWint dwarf_get_aranges_section_name(Dwarf_Debug dbg,
        const char ** sec_name,
        Dwarf_Error *error)\fP
.DE
\f(CW*dwarf_get_aranges_section_name()\fP
retrieves the object file section name of
the applicable aranges section.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.



.H 3 "dwarf_get_aranges()"
.DS
\f(CWint dwarf_get_aranges(
        Dwarf_Debug dbg,
        Dwarf_Arange **aranges,
        Dwarf_Signed * returned_arange_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_aranges()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_arange_count\fP to
the count of the
number of address ranges in the .debug_aranges section
(for all compilation units).
It sets
\f(CW*aranges\fP to point to a block of \f(CWDwarf_Arange\fP
descriptors, one for each address range.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if there is no .debug_aranges
section.

.in +2
.FG "Exampleu dwarf_get_aranges()"
.DS
\f(CW
void exampleu(Dwarf_Debug dbg)
{
    Dwarf_Signed count = 0;
    Dwarf_Arange *arang = 0;
    int res = 0;
    Dwarf_Error error = 0;

    res = dwarf_get_aranges(dbg, &arang,&count, &error);
    if (res == DW_DLV_OK) {
        Dwarf_Signed i = 0;

        for (i = 0; i < count; ++i) {
            /* use arang[i] */
            dwarf_dealloc(dbg, arang[i], DW_DLA_ARANGE);
        }
        dwarf_dealloc(dbg, arang, DW_DLA_LIST);
    }
}
\fP
.DE
.in -2

.H 3 "dwarf_get_arange()"
.DS
\f(CWint dwarf_get_arange(
        Dwarf_Arange *aranges,
        Dwarf_Unsigned arange_count,
        Dwarf_Addr address,
	Dwarf_Arange *returned_arange,
        Dwarf_Error *error);\fP
.DE
The function \f(CWdwarf_get_arange()\fP takes as input a pointer
to a block of \f(CWDwarf_Arange\fP pointers, and a count of the
number of descriptors in the block.
It then searches for the
descriptor that covers the given \f(CWaddress\fP.
If it finds
one, it returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_arange\fP to
the descriptor.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if there is no .debug_aranges
entry covering that address.





.H 3 "dwarf_get_cu_die_offset()"
.DS
\f(CWint dwarf_get_cu_die_offset(
        Dwarf_Arange arange,
        Dwarf_Off   *returned_cu_die_offset,
        Dwarf_Error *error);\fP
.DE
The function \f(CWdwarf_get_cu_die_offset()\fP takes a
\f(CWDwarf_Arange\fP descriptor as input, and
if successful returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_cu_die_offset\fP to
the offset
in the .debug_info section of the compilation-unit DIE for the
compilation-unit represented by the given address range.
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 3 "dwarf_get_arange_cu_header_offset()"
.DS
\f(CWint dwarf_get_arange_cu_header_offset(
        Dwarf_Arange arange,
        Dwarf_Off   *returned_cu_header_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_arange_cu_header_offset()\fP takes a
\f(CWDwarf_Arange\fP descriptor as input, and
if successful returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_cu_header_offset\fP to
the offset
in the .debug_info section of the compilation-unit header for the
compilation-unit represented by the given address range.
It returns \f(CWDW_DLV_ERROR\fP on error.

This function added Rev 1.45, June, 2001.

This function is declared as 'optional' in libdwarf.h
on IRIX systems so the _MIPS_SYMBOL_PRESENT
predicate may be used at run time to determine if the version of
libdwarf linked into an application has this function.



.H 3 "dwarf_get_arange_info()"
.DS
\f(CWint dwarf_get_arange_info(
        Dwarf_Arange arange,
        Dwarf_Addr *start,
        Dwarf_Unsigned *length,
        Dwarf_Off *cu_die_offset,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_arange_info()\fP returns
\f(CWDW_DLV_OK\fP
and
stores the starting value of the address range in the location pointed
to by \f(CWstart\fP, the length of the address range in the location
pointed to by \f(CWlength\fP, and the offset in the .debug_info section
of the compilation-unit DIE for the compilation-unit represented by the
address range.
It returns \f(CWDW_DLV_ERROR\fP on error.

.H 2 "General Low Level Operations"
This function is low-level and intended for use only
by programs such as dwarf-dumpers.

.H 3 "dwarf_get_offset_size()"
.DS
\f(CWint dwarf_get_offset_size(Dwarf_Debug dbg,
	Dwarf_Half  *offset_size,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_offset_size()\fP
returns \f(CWDW_DLV_OK\fP on success and sets
the \f(CW*offset_size\fP
to the size in bytes of an offset.
In case of error, it returns \f(CWDW_DLV_ERROR\fP
and does not set \f(CW*offset_size\fP.

The offset size returned is the overall address size,
which can be misleading if different compilation
units have different address sizes.
Many ABIs have only a single address size per
executable, but differing address sizes are
becoming more common.

.H 3 "dwarf_get_address_size()"
.DS
\f(CWint dwarf_get_address_size(Dwarf_Debug dbg,
	Dwarf_Half  *addr_size,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_address_size()\fP
returns \f(CWDW_DLV_OK\fP on success and sets
the \f(CW*addr_size\fP
to the size in bytes of an address.
In case of error, it returns \f(CWDW_DLV_ERROR\fP
and does not set \f(CW*addr_size\fP.

The address size returned is the overall address size,
which can be misleading if different compilation
units have different address sizes.
Many ABIs have only a single address size per
executable, but differing address sizes are
becoming more common.

Use \f(CWdwarf_get_die_address_size()\fP
instead whenever possible.

.H 3 "dwarf_get_die_address_size()"
.DS
\f(CWint dwarf_get_die_address_size(Dwarf_Die die,
	Dwarf_Half  *addr_size,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_die_address_size()\fP
returns \f(CWDW_DLV_OK\fP on success and sets
the \f(CW*addr_size\fP
to the size in bytes of an address.
In case of error, it returns \f(CWDW_DLV_ERROR\fP
and does not set \f(CW*addr_size\fP.

The address size returned is the address size
of the compilation unit owning the \f(CWdie\fP

This is the preferred way to get address size when the
\f(CWDwarf_Die\fP is known.


.H 2 "Ranges Operations (.debug_ranges)"
These functions provide information about the address ranges
indicated by a  \f(CWDW_AT_ranges\fP attribute (the ranges are recorded
in the  \f(CW.debug_ranges\fP section) of a DIE.
Each call of \f(CWdwarf_get_ranges_a()\fP
or \f(CWdwarf_get_ranges()\fP
returns a an array
of Dwarf_Ranges structs, each of which represents a single ranges
entry.   The struct is defined in  \f(CWlibdwarf.h\fP.

.H 3 "dwarf_get_ranges_section_name()"
.DS
\f(CWint dwarf_get_ranges_section_name(Dwarf_Debug dbg,
        const char ** sec_name,
        Dwarf_Error *error)\fP
.DE
\f(CW*dwarf_get_ranges_section_name()\fP
retrieves the object file section name of
the applicable ranges section.
This is useful for applications wanting to print
the name, but of course the object section name is not
really a part of the DWARF information.
Most applications will
probably not call this function.
It can be called at any time
after the Dwarf_Debug initialization is done.
.P
If the function succeeds, \f(CW*sec_name\fP is set to
a pointer to a string with the object section name and
the function returns \f(CWDW_DLV_OK\fP.
Do not free the string whose pointer is returned.
For non-Elf objects it is possible the string pointer
returned will be NULL or will point to an empty string.
It is up to the calling application to recognize this
possibility and deal with it appropriately.
.P
If the section does not exist the function returns
DW_DLV_NO_ENTRY.
.P
If there is an internal error detected the
function returns \f(CWDW_DLV_ERROR\fP and sets the
\f(CW*error\fP pointer.


.H 3 "dwarf_get_ranges()"
This is the original call and it will work fine when
all compilation units have the same address_size.
There is no \f(CWdie\fP argument to this original
version of the function.
Other arguments (and deallocation) match the use
of  \f(CWdwarf_get_ranges_a()\fP ( described next).

.H 3 "dwarf_get_ranges_a()"
.DS
\f(CWint dwarf_get_ranges_a(
        Dwarf_Debug dbg,
        Dwarf_Off  offset,
        Dwarf_Die  die,
        Dwarf_Ranges **ranges,
        Dwarf_Signed * returned_ranges_count,
        Dwarf_Unsigned * returned_byte_count,
        Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_ranges_a()\fP returns
\f(CWDW_DLV_OK\fP and sets \f(CW*returned_ranges_count\fP to
the count of the
number of address ranges in the group of ranges
in the .debug_ranges section at offset  \f(CWoffset\fP
(which ends with a pair of zeros of pointer-size).
This function is new as of 27 April 2009.

The
\f(CWoffset\fP argument should be the value of
a \f(CWDW_AT_ranges\fP attribute of a Debugging Information Entry.

The
\f(CWdie\fP argument should be the value of
a \f(CWDwarf_Die\fP pointer of a \f(CWDwarf_Die\fP with
the attribute containing
this range set offset.  Because each compilation unit
has its own address_size field this argument is necessary to
to correctly read ranges. (Most executables have the
same address_size in every compilation unit, but
some ABIs allow multiple address sized in an executable).
If a NULL pointer is passed in libdwarf assumes
a single address_size is appropriate for all ranges records.

The call sets
\f(CW*ranges\fP to point to a block of \f(CWDwarf_Ranges\fP
structs, one for each address range.
It returns \f(CWDW_DLV_ERROR\fP on error.
It returns \f(CWDW_DLV_NO_ENTRY\fP if there is no  \f(CW.debug_ranges\fP
section or if \f(CWoffset\fP is past the end of the
\f(CW.debug_ranges\fP section.

If the \f(CW*returned_byte_count\fP pointer is passed as non-NULL
the number of bytes that the returned ranges
were taken from is returned through the pointer
(for example if the returned_ranges_count is 2 and the pointer-size
is 4, then returned_byte_count will be 8).
If the \f(CW*returned_byte_count\fP pointer is passed as NULL
the parameter is ignored.
The \f(CW*returned_byte_count\fP is only of use to certain
dumper applications, most applications will not use it.


.in +2
.FG "Examplev dwarf_get_ranges_a()"
.DS
\f(CW
void examplev(Dwarf_Debug dbg,Dwarf_Unsigned offset,Dwarf_Die die)
{
    Dwarf_Signed count = 0;
    Dwarf_Ranges *ranges = 0;
    Dwarf_Unsigned bytes = 0;
    Dwarf_Error error = 0;
    int res = 0;
    res = dwarf_get_ranges_a(dbg,offset,die,
        &ranges,&count,&bytes,&error);
    if (res == DW_DLV_OK) {
        Dwarf_Signed i;
        for( i = 0; i < count; ++i ) {
            Dwarf_Ranges *cur = ranges+i;
            /* Use cur. */
        }
        dwarf_ranges_dealloc(dbg,ranges,count);
    }
}
\fP
.DE
.in -2

.H 3 "dwarf_ranges_dealloc()"
.DS
\f(CWint dwarf_ranges_dealloc(
    Dwarf_Debug dbg,
    Dwarf_Ranges *ranges,
    Dwarf_Signed  range_count,
    );\fP
.DE
The function \f(CWdwarf_ranges_dealloc()\fP takes as input a pointer
to a block of \f(CWDwarf_Ranges\fP array and the
number of structures in the block.
It frees all the data in the array of structures.

.H 2 "Gdb Index operations"
These functions get access to the fast lookup tables
defined by gdb and gcc and stored in the
\f(CW.gdb_index\fP
section.
The section is of sufficient complexity that a number of function
interfaces are needed.
For additional information see
"https://sourceware.org/gdb/onlinedocs/gdb/Index-Section-Format.html#Index-Section-Format".


.H 3 "dwarf_gdbindex_header()"
.DS
int dwarf_gdbindex_header(Dwarf_Debug dbg,
    Dwarf_Gdbindex * gdbindexptr,
    Dwarf_Unsigned * version,
    Dwarf_Unsigned * cu_list_offset,
    Dwarf_Unsigned * types_cu_list_offset,
    Dwarf_Unsigned * address_area_offset,
    Dwarf_Unsigned * symbol_table_offset,
    Dwarf_Unsigned * constant_pool_offset,
    Dwarf_Unsigned * section_size,
    Dwarf_Unsigned * unused_reserved,
    const char    ** section_name,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_header()\fP takes as input a pointer
to a Dwarf_Debug structure and returns fields through
various pointers.
.P
If the function returns DW_DLV_NO_ENTRY
there is no .gdb_index section and none of the return-pointer argument
values are set.
.P
If the function returns DW_DLV_ERROR \f(CWerror\fP is set
to indicate the specific error, but no other return-pointer
arguments are touched.
.P
If successful, the function returns DW_DLV_OK and other values are set.
The other values are set as follows:
.P
The field  \f(CW*gdbindexptr\fP is set to an opaque
pointer to a libdwarf_internal structure used as an argument
to other .gdbindex functions below.
.P
The remaining fields are set to values that are
mostly of interest to a pretty-printer application.
See the detailed layout specification for specifics.
The values returned are recorded in the
Dwarf_Gdbindex opaque structure for the other gdbindex
functions documented below.
.P
The field  \f(CW*version\fP is set to the version
of the gdb index header (2)..
.P
The field  \f(CW*cu_list_offset\fP is set to
the offset (in the .gdb_index section) of the
cu-list.
.P
The field  \f(CW*types_cu_list_offset\fP is set to
the offset (in the .gdb_index section) of the
types-list.
.P
The field  \f(CW*address_area_offset\fP is set to
the offset (in the .gdb_index section) of the
address area.
.P
The field  \f(CW*symbol_table_offset\fP is set to
the offset (in the .gdb_index section) of the
symbol table.
.P
The field  \f(CW*constant_pool_offset\fP is set to
the offset (in the .gdb_index section) of the
constant pool.
.P
The field  \f(CW*section_size\fP is set to
the length  of the .gdb_index section.
.P
The field  \f(CW*unused_reserved\fP is set to
zero.
.P
The field  \f(CW*section_name\fP is set to
the Elf object file section name (.gdb_index).
If a non-Elf object file has such a section
the value set might be NULL or might point to
an empty string (NUL terminated), so
code to account for NULL or empty.
.P
The field  \f(CW*error\fP is not set.
.P
Here we show a use of the set of cu_list
functions (using all the functions in one
example makes it rather too long).

.in +2
.FG "Examplew dwarf_get_gdbindex_header()"
.DS
\f(CW
void examplew(Dwarf_Debug dbg,Dwarf_Unsigned offset,Dwarf_Die die)
{
    Dwarf_Gdbindex gindexptr = 0;
    Dwarf_Unsigned version = 0;
    Dwarf_Unsigned cu_list_offset = 0;
    Dwarf_Unsigned types_cu_list_offset = 0;
    Dwarf_Unsigned address_area_offset = 0;
    Dwarf_Unsigned symbol_table_offset = 0;
    Dwarf_Unsigned constant_pool_offset = 0;
    Dwarf_Unsigned section_size = 0;
    Dwarf_Unsigned reserved = 0;
    Dwarf_Error error = 0;
    const char * section_name = 0;
    int res = 0;
    res = dwarf_gdbindex_header(dbg,&gindexptr,
        &version,&cu_list_offset, &types_cu_list_offset,
        &address_area_offset,&symbol_table_offset,
        &constant_pool_offset, &section_size,
        &reserved,&section_name,&error);
    if (res == DW_DLV_NO_ENTRY) {
        return;
    } else if (res == DW_DLV_ERROR) {
        return;
    }
    {
        /* do something with the data */
        Dwarf_Unsigned length = 0;
        Dwarf_Unsigned typeslength = 0;
        Dwarf_Unsigned i = 0;
        res = dwarf_gdbindex_culist_array(gindexptr,
            &length,&error);
        /* Example actions. */
        if (res == DW_DLV_OK) {
            for(i = 0; i < length; ++i) {
                Dwarf_Unsigned cuoffset = 0;
                Dwarf_Unsigned culength = 0;
                res = dwarf_gdbindex_culist_entry(gindexptr,
                    i,&cuoffset,&culength,&error);
                if (res == DW_DLV_OK) {
                    /* Do something with cuoffset, culength */
                }
            }
        }
        res = dwarf_gdbindex_types_culist_array(gindexptr,
            &typeslength,&error);
        if (res == DW_DLV_OK) {
            for(i = 0; i < typeslength; ++i) {
                Dwarf_Unsigned cuoffset = 0;
                Dwarf_Unsigned tuoffset = 0;
                Dwarf_Unsigned culength = 0;
                Dwarf_Unsigned type_signature  = 0;
                res = dwarf_gdbindex_types_culist_entry(gindexptr,
                    i,&cuoffset,&tuoffset,&type_signature,&error);
                if (res == DW_DLV_OK) {
                    /* Do something with cuoffset etc. */
                }
            }
        }
        dwarf_gdbindex_free(gindexptr);
    }
}
\fP
.DE
.in -2





.H 3 "dwarf_gdbindex_culist_array()"
.DS
int dwarf_gdbindex_culist_array(Dwarf_Gdbindex gdbindexptr,
    Dwarf_Unsigned       * list_length,
    Dwarf_Error          * error);
.DE
The function \f(CWdwarf_gdbindex_culist_array()\fP takes as input
valid Dwarf_Gdbindex pointer.
.P
While currently only DW_DLV_OK is returned one should
test for DW_DLV_NO_ENTRY and DW_DLV_ERROR and do  something
sensible if either is returned.
.P
If successful, the function
returns DW_DLV_OK
and returns the number
of entries in the  culist through the\f(CWlist_length\fP
pointer.

.H 3 "dwarf_gdbindex_culist_entry()"
.DS
int dwarf_gdbindex_culist_entry(Dwarf_Gdbindex gdbindexptr,
    Dwarf_Unsigned   entryindex,
    Dwarf_Unsigned * cu_offset,
    Dwarf_Unsigned * cu_length,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_culist_entry()\fP takes as input
valid Dwarf_Gdbindex pointer and an index into the culist array.
Valid indexes are 0 through \f(CWlist_length -1\fP .
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind
and the  error is indicated by
the vale returned through the \f(CWerror\fP pointer.
.P
On success it returns DW_DLV_OK and returns
the \f(CWcu_offset\fP (the section global offset of the CU
in .debug_info))
and \f(CWcu_length\fP (the length of the CU
in .debug_info) values through the pointers.


.H 3 "dwarf_gdbindex_types_culist_array()"
.DS
int dwarf_gdbindex_types_culist_array(Dwarf_Gdbindex /*gdbindexptr*/,
    Dwarf_Unsigned            * /*types_list_length*/,
    Dwarf_Error               * /*error*/);
.DE
The function \f(CWdwarf_gdbindex_types_culist_array()\fP takes as input
valid Dwarf_Gdbindex pointer.
.P
While currently only DW_DLV_OK is returned one should
test for DW_DLV_NO_ENTRY and DW_DLV_ERROR and do  something
sensible if either is returned.
.P
If successful, the function
returns DW_DLV_OK
and returns the number
of entries in the  types culist through the\f(CWlist_length\fP
.P

.H 3 "dwarf_gdbindex_types_culist_entry()"
.DS
int dwarf_gdbindex_types_culist_entry(
    Dwarf_Gdbindex   gdbindexptr,
    Dwarf_Unsigned   entryindex,
    Dwarf_Unsigned * cu_offset,
    Dwarf_Unsigned * tu_offset,
    Dwarf_Unsigned * type_signature,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_types_culist_entry()\fP takes as input
valid Dwarf_Gdbindex pointer and an index into the types culist array.
Valid indexes are 0 through \f(CWtypes_list_length -1\fP .
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
On success it returns DW_DLV_OK and returns
the \f(CWtu_offset\fP (the section global offset of the CU
in .debug_types))
and \f(CWtu_length\fP (the length of the CU
in .debug_types) values through the pointers.
It also returns  the type signature (a 64bit value) through
the  \f(CWtype_signature\fP pointer.

.H 3 "dwarf_gdbindex_addressarea()"
.DS
int dwarf_gdbindex_addressarea(Dwarf_Gdbindex /*gdbindexptr*/,
    Dwarf_Unsigned            * /*addressarea_list_length*/,
    Dwarf_Error               * /*error*/);
.DE
The function \f(CWdwarf_addressarea()\fP takes as input
valid Dwarf_Gdbindex pointer and returns
the length of the address area through \f(CWaddressarea_list_length\fP.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns the number
of entries in the  address area through the
\f(CWaddressarea_list_length\fP  pointer.


.H 3 "dwarf_gdbindex_addressarea_entry()"
.DS
int dwarf_gdbindex_addressarea_entry(
    Dwarf_Gdbindex   gdbindexptr,
    Dwarf_Unsigned   entryindex,
    Dwarf_Unsigned * low_address,
    Dwarf_Unsigned * high_address,
    Dwarf_Unsigned * cu_index,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_addressarea_entry()\fP takes as input
valid Dwarf_Gdbindex pointer
and an index into the address area (valid indexes are
zero through \f(CWaddressarea_list_length - 1\fP.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns
The
\f(CWlow_address\fP
\f(CWhigh_address\fP
and
\f(CWcu_index\fP
through the pointers.
.P
Given an open Dwarf_Gdbindex one uses the function as follows:
.P
.DS
.FG "Examplewgdbindex dwarf_gdbindex_addressarea()"
\f(CW
void examplewgdbindex(Dwarf_Gdbindex gdbindex)
{
    Dwarf_Unsigned list_len = 0;
    Dwarf_Unsigned i = 0;
    int res = 0;
    Dwarf_Error err = 0;

    res = dwarf_gdbindex_addressarea(gdbindex, &list_len,&err);
    if (res != DW_DLV_OK) {
        /* Something wrong, ignore the addressarea */
    }
    /* Iterate through the address area. */
    for( i  = 0; i < list_len; i++) {
        Dwarf_Unsigned lowpc = 0;
        Dwarf_Unsigned highpc = 0;
        Dwarf_Unsigned cu_index = 0;
        res = dwarf_gdbindex_addressarea_entry(gdbindex,i,
            &lowpc,&highpc,
            &cu_index,
            &err);
        if (res != DW_DLV_OK) {
            /* Something wrong, ignore the addressarea */
            return;
        }
        /*  We have a valid address area entry, do something
            with it. */
    }
}
\fP
.DE


.H 3 "dwarf_gdbindex_symboltable_array()"
.DS
int dwarf_gdbindex_symboltable_array(Dwarf_Gdbindex gdbindexptr,
    Dwarf_Unsigned            * symtab_list_length,
    Dwarf_Error               * error);
.DE
One can look at the symboltable as a two-level table (with
The outer level indexes through symbol names and the inner level
indexes through all the compilation units that
define that symbol (each symbol having a different number
of compilation units, this is not a simple rectangular table).
.P
The function \f(CWdwarf_gdbindex_symboltable_array()\fP takes as input
valid Dwarf_Gdbindex pointer.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns
The \f(CWsymtab_list_length\fP through the pointer.
.P
Given a valid Dwarf_Gdbindex pointer, one can access the entire
symbol table as follows (using 'return' here to indicate
we are giving up due to a problem while keeping the
example code fairly short):
.DS
.FG "Examplex dwarf_gdbindex_symboltable_array()"
\f(CW
void examplex(Dwarf_Gdbindex gdbindex)
{
    Dwarf_Unsigned symtab_list_length = 0;
    Dwarf_Unsigned i = 0;
    Dwarf_Error err = 0;
    int res = 0;

    res = dwarf_gdbindex_symboltable_array(gdbindex,
        &symtab_list_length,&err);
    if (res != DW_DLV_OK) {
        return;
    }
    for( i  = 0; i < symtab_list_length; i++) {
        Dwarf_Unsigned symnameoffset = 0;
        Dwarf_Unsigned cuvecoffset = 0;
        Dwarf_Unsigned cuvec_len = 0;
        Dwarf_Unsigned ii = 0;
        const char *name = 0;
        res = dwarf_gdbindex_symboltable_entry(gdbindex,i,
            &symnameoffset,&cuvecoffset,
            &err);
        if (res != DW_DLV_OK) {
            return;
        }
        res = dwarf_gdbindex_string_by_offset(gdbindex,
            symnameoffset,&name,&err);
        if(res != DW_DLV_OK) {
            return;
        }
        res = dwarf_gdbindex_cuvector_length(gdbindex,
            cuvecoffset,&cuvec_len,&err);
        if( res != DW_DLV_OK) {
            return;
        }
        for(ii = 0; ii < cuvec_len; ++ii ) {
            Dwarf_Unsigned attributes = 0;
            Dwarf_Unsigned cu_index = 0;
            Dwarf_Unsigned reserved1 = 0;
            Dwarf_Unsigned symbol_kind = 0;
            Dwarf_Unsigned is_static = 0;

            res = dwarf_gdbindex_cuvector_inner_attributes(
                gdbindex,cuvecoffset,ii,
                &attributes,&err);
            if( res != DW_DLV_OK) {
                return;
            }
            /*  'attributes' is a value with various internal
                fields so we expand the fields. */
            res = dwarf_gdbindex_cuvector_instance_expand_value(gdbindex,
                attributes, &cu_index,&reserved1,&symbol_kind, &is_static,
                &err);
            if( res != DW_DLV_OK) {
                return;
            }
            /* Do something with the attributes. */
        }
    }
}
\fP
.DE

.H 3 "dwarf_gdbindex_symboltable_entry()"
.DS
int dwarf_gdbindex_symboltable_entry(
    Dwarf_Gdbindex   gdbindexptr,
    Dwarf_Unsigned   entryindex,
    Dwarf_Unsigned * string_offset,
    Dwarf_Unsigned * cu_vector_offset,
    Dwarf_Error    * error);
.DE
.P
The function \f(CWdwarf_gdbindex_symboltable_entry()\fP takes as input
valid Dwarf_Gdbindex pointer and
an entry index(valid index values being zero through
\f(CWsymtab_list_length -1\fP).
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns
The \f(CWstring_offset\fP
and \f(CWcu_vector_offset\fP through the pointers.
See the example above which uses this function.


.H 3 "dwarf_gdbindex_cuvector_length()"
.DS
int dwarf_gdbindex_cuvector_length(
    Dwarf_Gdbindex   gdbindex,
    Dwarf_Unsigned   cuvector_offset,
    Dwarf_Unsigned * innercount,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_cuvector_length()\fP takes as input
valid Dwarf_Gdbindex pointer and
an a cu vector offset.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns
the \f(CWinner_count\fP  through the pointer.
The \f(CWinner_count\fP  is the number of
compilation unit vectors for this array of vectors.
See the example above which uses this function.



.H 3 "dwarf_gdbindex_cuvector_inner_attributes()"
.DS
int dwarf_gdbindex_cuvector_inner_attributes(
    Dwarf_Gdbindex   gdbindex,
    Dwarf_Unsigned   cuvector_offset,
    Dwarf_Unsigned   innerindex,
    /* The attr_value is a field of bits. For expanded version
        use  dwarf_gdbindex_cuvector_expand_value() */
    Dwarf_Unsigned * attr_value,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_cuvector_inner_attributes()\fP takes as input
valid Dwarf_Gdbindex pointer and
an a cu vector offset and a \f(CWinner_index\fP (valid
\f(CWinner_index\fP values are zero through \f(CWinner_count - 1\fP.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns
The \f(CWattr_value\fP through the pointer.
The \f(CWattr_value\fP is actually composed of several fields,
see the next function which expands the value.
See the example above which uses this function.



.H 3 "dwarf_gdbindex_cuvector_instance_expand_value()"
.DS
int dwarf_gdbindex_cuvector_instance_expand_value(
    Dwarf_Gdbindex   gdbindex,
    Dwarf_Unsigned   attr_value,
    Dwarf_Unsigned * cu_index,
    Dwarf_Unsigned * reserved1,
    Dwarf_Unsigned * symbol_kind,
    Dwarf_Unsigned * is_static,
    Dwarf_Error    * error);
.DE
The function \f(CWdwarf_gdbindex_cuvector_instance_expand_value()\fP
takes as input
valid Dwarf_Gdbindex pointer and
an \f(CWattr_value\fP.
.P
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns the following values through the pointers:

The \f(CWcu_index\fP field is the index in the applicable
CU list of a compilation unit. For the purpose of
indexing the CU list and the types CU list form a single
array so the \f(CWcu_index\fP can be indicating either list.

The \f(CWsymbol_kind\fP field is a small integer with the symbol kind(
zero is reserved, one is a type, 2 is a variable or enum value, etc).

The \f(CWreserved1\fP field  should have the value zero
and is the value of a bit field defined as reserved for future use.

The \f(CWis_static\fP field is zero if the CU indexed is global
and one if the CU indexed is static.


See the example above which uses this function.


.H 3 "dwarf_gdbindex_string_by_offset()"
.DS
int dwarf_gdbindex_string_by_offset(
    Dwarf_Gdbindex   gdbindexptr,
    Dwarf_Unsigned   stringoffset,
    const char    ** string_ptr,
    Dwarf_Error   *  error);
.DE
The function \f(CWdwarf_gdbindex_string_by_offset()\fP
takes as input
valid Dwarf_Gdbindex pointer and
a \f(CWstringoffset\fP
If it returns DW_DLV_NO_ENTRY there is a coding error.
If it returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If it succeeds, the call returns a pointer to a string
from the 'constant pool' through the \f(CWstring_ptr\fP.
The string pointed to must never be free()d.
.P
See the example above which uses this function.


.H 2 "Debug Fission (.debug_tu_index, .debug_cu_index) operations"
We name things "xu" as these sections have the same format
so we let "x" stand for either section.
These functions get access to the  index functions needed
to access and print the contents of an object file
which is an aggregate of .dwo objects.
These
sections are implemented in gcc/gdb and are proposed
to be part of DWARF5 (As of July 2014 DWARF5 is not finished).
The idea is that much debug information can be separated
off into individual .dwo Elf objects and then aggregated
simply into a single .dwp object so the executable need not
have the complete debug information in it at runtime
yet allow good debugging.
.P
For additional information, see
"https://gcc.gnu.org/wiki/DebugFissionDWP",
"https://gcc.gnu.org/wiki/DebugFission",
and
"http://www.bayarea.net/~cary/dwarf/Accelerated%20Access%20Diagram.png"
and sometime in 2015, the DWARF5 standard.
.P
There are FORM access functions related
to Debug Fission.
See
\f(CWdwarf_formaddr()\fP
and
\f(CWdwarf_get_debug_addr_index()\fP
and
\f(CWdwarf_get_debug_str_index()\fP.
.P
The FORM with the hash value (for a reference
to  a type unit ) is
\f(CWDW_FORM_ref_sig8\fP.
.P
In a compilation unit of  Debug Fission object
(or a .dwp Package FIle)
\f(CWDW_AT_dwo_id\fP
the hash is expected to be
\f(CWDW_FORM_data8\fP.
.P
The \f(CWDWARF5\fP standard defines the hash as an 8 byte value
which we could use \f(CWDwarf_Unsigned\fP.
Instead (and mostly for type safety) we define
the value as  a structure whose type name is
\f(CWDwarf_Sig8\fP.

.P
To look up a name in the hash (to find
which CU(s) it exists in).
use
\f(CWdwarf_get_debugfission_for_key()fP,
defined below.
.P
The second group of interfaces here
beginning with
\f(CWdwarf_get_xu_index_header()\fP
are useful if one wants to print
a .debug_tu_index or .debug_cu_index section.
.P
To access DIE, macro, etc information
the support is built into DIE, Macro, etc operations
so applications usually won't need to use these
operations at all.

.H 3 "Dwarf_Debug_Fission_Per_CU"

.DS
#define DW_FISSION_SECT_COUNT 12
struct Dwarf_Debug_Fission_Per_CU_s  {
    /*  Do not free the string. It contains "cu" or "tu". */
    /*  If this is not set (ie, not a CU/TU in  DWP Package File)
        then pcu_type will be NULL.  */
    const char   * pcu_type;
    /*  pcu_index is the index (range 1 to N )
        into the tu/cu table of offsets and the table
        of sizes.  1 to N as the zero index is reserved
        for special purposes.  Not a value one
        actually needs. */
    Dwarf_Unsigned pcu_index;
    Dwarf_Sig8     pcu_hash;  /* 8 byte  */
    /*  [0] has offset and size 0.
        [1]-[8] are DW_SECT_* indexes and the
        values are  the offset and size
        of the respective section contribution
        of a single .dwo object. When pcu_size[n] is
        zero the corresponding section is not present. */
    Dwarf_Unsigned pcu_offset[DW_FISSION_SECT_COUNT];
    Dwarf_Unsigned pcu_size[DW_FISSION_SECT_COUNT];
    Dwarf_Unsigned unused1;
    Dwarf_Unsigned unused2;
};
.DE
.P
The structure is used to return data to callers with
the data from either .debug_tu_index or .debug_cu_index
that is applicable to a single compilation unit or type unit.
.P
Callers to the applicable functions (see below)
should allocate the structure and zero all the
bytes in it.  The structure has a few
fields that are presently unused. These
are reserved for future use since it is
impossible to alter the structure without
breaking binary compatibility.

.H 3 "dwarf_die_from_hash_signature()"
.DS
int dwarf_die_from_hash_signature(Dwarf_Debug dbg,
    Dwarf_Sig8 *     hash_sig,
    const char *     sig_type,
    Dwarf_Die*       returned_die,
    Dwarf_Error*     error);
.DE
The function
\f(CWdwarf_die_from_hash_signature()\fP
is the most direct way to go from
the hash data from a
\f(CWDW_FORM_ref_sig8\fP
or
a
\f(CWDW_AT_dwo_id\fP (form
\f(CWDW_FORM_data8\fP)
to a DIE from a .dwp package file
or a .dwo object file ( .dwo access
not supported yet).
.P
The caller passes in
\f(CWdbg\fP which should be
\f(CWDwarf_Debug\fP open/initialized
on a .dwp package file (or a .dwo object file).
.P
The caller also  passes in
\f(CWhash_sig\fP, a pointer to the
hash signature for which the caller
wishes to find a DIE.
.P
The caller also  passes in
\f(CWsig_type\fP which must
contain either "tu" (identifying
the hash referring to a type unit)
or "cu" (identifying the hash as referring
to a compilation unit).
.P
On success the function returns
\f(CWDW_DLV_OK\fP and sets
\f(CW*returned_die\fP to be a
pointer to a valid DIE for the
compilation unit or type unit.
If the type is "tu" the DIE returned
is the specific type DIE that the
hash refers to.
If the type is "cu" the DIE returned
is the compilation unit DIE of the
compilation unit referred to.
.P
When appropriate the caller should
free the space of the returned DIE
by a call something like
.DS
    dwarf_dealloc(dbg,die,DW_DLA_DIE);
.DE
.P
If there is no DWP Package File section
or the hash cannot be found
the function returns
\f(CWDW_DLV_NO_ENTRY\fP and leaves \f(CWreturned_die\fP
untouched.  Only .dwo objects and .dwp package files
have the package file index sections.
.P
If there is an error of some sort
the function returns
\f(CWDW_DLV_ERROR\fP, leaves \f(CWreturned_die\fP
untouched, and sets \f(CW*error\fP to indicate
the precise error encountered.
.P


.H 3 "dwarf_get_debugfission_for_die()"
.DS
int dwarf_get_debugfission_for_die(Dwarf_Die die,
    Dwarf_Debug_Fission_Per_CU * percu_out,
    Dwarf_Error *  error);
.DE
The function
\f(CWdwarf_get_debugfission_for_die()\fP
returns the debug fission for the compilation
unit the DIE is a part of.
Any DIE in the compilation (or type) unit
will get the same result.
.P
On a call to this function ensure the pointed-to
space is fully initialized.
.P
On success the function returns
\f(CWDW_DLV_OK\fP and fills in the
fields of \f(CW*percu_out\fP for which
it has data.
.P
If there is no DWP Package File section
the function returns
\f(CWDW_DLV_NO_ENTRY\fP and leaves \f(CW*percu_out\fP
untouched.  Only .dwp package files
have the package file index sections.
.P
If there is an error of some sort
the function returns
\f(CWDW_DLV_ERROR\fP, leaves \f(CW*percu_out\fP
untouched, and sets \f(CW*error\fP to indicate
the precise error encountered.


.H 3 "dwarf_get_debugfission_for_key()"
.DS
int dwarf_get_debugfission_for_key(Dwarf_Debug dbg,
    Dwarf_Sig8 *                 key,
    const char *                 key_type ,
    Dwarf_Debug_Fission_Per_CU * percu_out,
    Dwarf_Error *                error);
.DE

The function
\f(CWdwarf_get_debugfission_for_key()\fP
returns the debug fission data
for the
compilation unit in a .dwp  package file.
.P
If there is no DWP Package File section
the function returns
\f(CWDW_DLV_NO_ENTRY\fP and leaves \f(CW*percu_out\fP
untouched.  Only .dwp package files
have the package file index sections.
.P
If there is an error of some sort
the function returns
\f(CWDW_DLV_ERROR\fP, leaves \f(CW*percu_out\fP
untouched, and sets \f(CW*error\fP to indicate
the precise error encountered.

.H 3 "dwarf_get_xu_index_header()"
.DS
int dwarf_get_xu_index_header(Dwarf_Debug dbg,
    const char *  section_type, /* "tu" or "cu" */
    Dwarf_Xu_Index_Header *     xuhdr,
    Dwarf_Unsigned *            version_number,
    Dwarf_Unsigned *            offsets_count    /* L*/,
    Dwarf_Unsigned *            units_count      /* N*/,
    Dwarf_Unsigned *            hash_slots_count /* M*/,
    const char     **           sect_name,
    Dwarf_Error *               err);
.DE
The function \f(CWdwarf_get_xu_index_header()\fP
takes as input a
valid Dwarf_Debug pointer and
an \f(CWsection_type\fP value, which must one of the
strings \f(CWtu\fP or \f(CWcu\fP.
.P
It returns DW_DLV_NO_ENTRY if the section requested
is not in the object file.
.P
It returns DW_DLV_ERROR there is an error of some kind.
and the  error is indicated by
the value returned through the \f(CWerror\fP pointer.
.P
If successful, the function
returns DW_DLV_OK
and returns the following values through the pointers:
.P
The \f(CWxuhdr\fP field is a pointer usable in
other operations (see below).
.P
The \f(CWversion_number\fP field is a the index version
number.
For gcc before DWARF5 the version number is 2.
For DWARF5 the version number is 5.
.P
The \f(CWoffsets_count\fP field is a the number
of columns in the table of section offsets.
Sometimes known as \f(CWL\fP.
.P
The \f(CWunits_count\fP field is a the number
of compilation units or type units in the index.
Sometimes known as \f(CWN\fP.
.P
The \f(CWhash_slots_count\fP field is a the number
of slots in the hash table.
Sometimes known as \f(CWM\fP.
.P
The \f(CWsect_name\fP field is the name
of the section in the object file.
Because non-Elf objects may not use section names
callers must recognize that the sect_name may be
set to NULL (zero) or to point to the empty string
and this is not considered an error.

.P
An example of initializing and disposing
of a \f(CWDwarf_Xu_Index_Header\fP follows.
.DS
.FG "Exampley dwarf_get_xu_index_header()"
\f(CW
void exampley(Dwarf_Debug dbg, const char *type)
{
    /* type is "tu" or "cu" */
    int res = 0;
    Dwarf_Xu_Index_Header xuhdr = 0;
    Dwarf_Unsigned version_number = 0;
    Dwarf_Unsigned offsets_count = 0; /*L */
    Dwarf_Unsigned units_count = 0; /* M */
    Dwarf_Unsigned hash_slots_count = 0; /* N */
    Dwarf_Error err = 0;
    const char * ret_type = 0;
    const char * section_name = 0;

    res = dwarf_get_xu_index_header(dbg,
        type,
        &xuhdr,
        &version_number,
        &offsets_count,
        &units_count,
        &hash_slots_count,
        &section_name,
        &err);
    if (res == DW_DLV_NO_ENTRY) {
        /* No such section. */
        return;
    }
    if (res == DW_DLV_ERROR) {
        /* Something wrong. */
        return;
    }
    /* Do something with the xuhdr here . */
    dwarf_xu_header_free(xuhdr);
}
\fP
.DE
.H 3 "dwarf_get_xu_index_section_type()"
.DS
int dwarf_get_xu_index_section_type(
    Dwarf_Xu_Index_Header xuhdr,
    const char ** typename,
    const char ** sectionname,
    Dwarf_Error * error);
.DE
The function \f(CWdwarf_get_xu_section_type()\fP
takes as input a
valid  \f(CWDwarf_Xu_Index_Header\fP.
It is only useful when one already as an
open \f(CWxuhdr\fP but one does not know if
this is a type unit or compilation unit index section.
.P
If it returns DW_DLV_NO_ENTRY something is wrong
(should never happen).
If it returns DW_DLV_ERROR something is wrong and
the \f(CWerror\fP field is set to indicate a specific error.
.P
If successful, the function returns DW_DLV_OK
and sets the following arguments through the pointers:
.P
\f(CWtypename\fP is set to the string  \f(CWtu\fP
or  \f(CWcu\fP to indcate the index is of a type unit
or a compilation unit, respectively.
.P
\f(CWsectionname\fP is set to name of the object
file section.
Because non-Elf objects may not use section names
callers must recognize that the sect_name may be
set to NULL (zero) or to point to the empty string
and this is not considered an error.
.P
Neither string should be free()d.


.H 3 "dwarf_get_xu_header_free()"
.DS
void dwarf_xu_header_free(Dwarf_Xu_Index_Header xuhdr);
.DE
The function \f(CWdwarf_get_xu_header_free()\fP
takes as input a valid \f(CWDwarf_Xu_Index_Header\fP
and frees all the special data allocated for this
access type.  Once called, any pointers returned
by use of the  \f(CWxuhdr\fP should be considered
stale and unusable.

.H 3 "dwarf_get_xu_hash_entry()"
.DS
int dwarf_get_xu_hash_entry(
    Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned        index,
    Dwarf_Sig8     *      hash_value,
    Dwarf_Unsigned *      index_to_sections,
    Dwarf_Error *         error);
.DE
The function \f(CWdwarf_get_xu_hash_entry()\fP
takes as input a valid \f(CWDwarf_Xu_Index_Header\fP
and an  \f(CWindex\fP of a hash slot entry
(valid hash slot index values are zero (0) through
\f(CWhash_slots_count -1\fP (M-1)).
.P
If it returns DW_DLV_NO_ENTRY something is wrong
.P
If it returns DW_DLV_ERROR something is wrong and
the \f(CWerror\fP field is set to indicate a specific error.
.P
If successful, the function returns DW_DLV_OK
and sets the following arguments through the pointers:
.P
\f(CWhash_value\fP is set to the 64bit hash of of the symbol name.
.P
\f(CWindex_to_sections\fP is set to the index into offset-size tables
of this hash entry.
.P
If both \f(CWhash_value\fP and \f(CWindex_to_sections\fP are
zero (0)
then the hash slot is unused.
\f(CWindex_to_sections\fP is used in calls to
the function \f(CWdwarf_get_xu_section_offset()\fP
as the \f(CWrow_index\fP.
.P
An example of use follows.
.DS
.FG "Examplez dwarf_get_xu_hash_entry()"
\f(CW
void examplez( Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned hash_slots_count)
{
    /*  hash_slots_count returned by
        dwarf_get_xu_index_header(), see above. */
    static Dwarf_Sig8 zerohashval;

    Dwarf_Error err = 0;
    Dwarf_Unsigned h = 0;

    for( h = 0; h < hash_slots_count; h++) {
        Dwarf_Sig8 hashval;
        Dwarf_Unsigned index = 0;
        Dwarf_Unsigned col = 0;
        int res = 0;

        res = dwarf_get_xu_hash_entry(xuhdr,h,
            &hashval,&index,&err);
        if (res == DW_DLV_ERROR) {
            /* Oops. hash_slots_count wrong. */
            return;
        } else if (res == DW_DLV_NO_ENTRY) {
            /* Impossible */
            return;
        } else if (!memcmp(&hashval,&zerohashval,
            sizeof(Dwarf_Sig8))
            && index == 0 ) {
            /* An unused hash slot */
            continue;
        }
        /*  Here, hashval and index (a row index into
            offsets and lengths) are valid. */
    }
}
\fP

.DE


.H 3 "dwarf_get_xu_section_names()"
.DS
int dwarf_get_xu_section_names(
    Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned        column_index,
    Dwarf_Unsigned*       number,
    const char **         name,
    Dwarf_Error *         err);
.DE
The function \f(CWdwarf_get_xu_section_names()\fP
takes as input a valid \f(CWDwarf_Xu_Index_Header\fP
and a  \f(CWcolumn_index\fP of a hash slot entry
(valid column_index values are zero (0) through
\f(CWoffsets_count -1\fP (L-1)).
.P
If it returns DW_DLV_NO_ENTRY something is wrong
.P
If it returns DW_DLV_ERROR something is wrong and
the \f(CWerror\fP field is set to indicate a specific error.
.P
If successful, the function returns DW_DLV_OK
and sets the following arguments through the pointers:
.P
\f(CWnumber\fP is set to a number identifying which section
this column applies to. For example, if the value is
\f(CWDW_SECT_INFO\fP
(1) the column came from  a .debug_info.dwo section.
See the table of \f(CWDW_SECT_\fP identifiers and
asigned numbers in DWARF5.
.P
\f(CWname\fP is set to the applicable spelling of the
section identifier, for example \f(CWDW_SECT_INFO\fP.


.H 3 "dwarf_get_xu_section_offset()"
.DS
int dwarf_get_xu_section_offset(
    Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned        row_index,
    Dwarf_Unsigned        column_index,
    Dwarf_Unsigned*       sec_offset,
    Dwarf_Unsigned*       sec_size,
    Dwarf_Error *         error);
.DE
The function \f(CWdwarf_get_xu_section_offset()\fP
takes as input a valid \f(CWDwarf_Xu_Index_Header\fP
and a  \f(CWrow_index\fP
(see \f(CWdwarf_get_xu_hash_entry()\fP above)
and a  \f(CWcolumn_index\fP.
Valid row_index values are one (1) through
\f(CWunits_count\fP (N) but one uses
\f(CWdwarf_get_xu_hash_entry()\fP (above) to get
row index.
Valid column_index values are zero (0) through
\f(CWoffsets_count -1\fP (L-1).
.P
If it returns DW_DLV_NO_ENTRY something is wrong.
.P
If it returns DW_DLV_ERROR something is wrong and
the \f(CWerror\fP field is set to indicate a specific error.
.P
If successful, the function returns DW_DLV_OK
and sets the following arguments through the pointers:
.P
\f(CWsec_offset\fP, (\f(CWbase offset\fP) is set to
the base offset of the initial compilation-unit-header
section taken from  a .dwo object.
The  base offset is the data
from a single section of a .dwo object.
.P
\f(CWsec_size\fP is set to
the length of the  original section taken from
a .dwo object.
This is the length in the applicable
section in the .dwp over which the base offset applies.
.P
An example of use of
\f(CWdwarf_get_xu_section_names()\fP
and
\f(CWdwarf_get_xu_section_offset()\fP
follows.
.DS
.FG "Exampleza dwarf_get_xu_section_names()"
\f(CW
void exampleza(Dwarf_Xu_Index_Header xuhdr,
    Dwarf_Unsigned offsets_count, Dwarf_Unsigned index )
{
    Dwarf_Error err = 0;
    Dwarf_Unsigned col = 0;
    /*  We use  'offsets_count' returned by
        a dwarf_get_xu_index_header() call.
        We use 'index' returned by a
        dwarf_get_xu_hash_entry() call. */
    for (col = 0; col < offsets_count; col++) {
        Dwarf_Unsigned off = 0;
        Dwarf_Unsigned len = 0;
        const char * name = 0;
        Dwarf_Unsigned num = 0;
        int res = 0;

        res = dwarf_get_xu_section_names(xuhdr,
            col,&num,&name,&err);
        if (res != DW_DLV_OK) {
            break;
        }
        res = dwarf_get_xu_section_offset(xuhdr,
            index,col,&off,&len,&err);
        if (res != DW_DLV_OK) {
            break;
        }
        /*  Here we have the DW_SECT_ name and number
            and the base offset and length of the
            section data applicable to the hash
            that got us here.
            Use the values.*/
    }
}
.DE

.H 2 "TAG ATTR etc names as strings"
These functions turn a value into a string.
So applications wanting the string "DW_TAG_compile_unit"
given the value 0x11 (the value defined for this TAG) can do so easily.

The general form is
.in +2
.DS
\f(CWint dwarf_get_<something>_name(
    unsigned value,
    char **s_out,
    );\fP
.DE
.in -2

If the \f(CWvalue\fP passed in is known, the function
returns \f(CWDW_DLV_OK\fP and places a pointer to the appropriate string
into  \f(CW*s_out\fP.   The string is in static storage
and applications must never free the string.
If the \f(CWvalue\fP is not known, \f(CWDW_DLV_NO_ENTRY\fP is returned
and \f(CW*s_out\fP is not set.  \f(CWDW_DLV_ERROR\fP is never returned.

\f(CWLibdwarf\fP generates these functions at libdwarf build time
by reading dwarf.h.

All these follow this pattern rigidly, so the details of each
are not repeated for each function.

The choice of 'unsigned' for the value type argument (the code value)
argument is somewhat arbitrary, 'int' could have been used.

The library simply assumes the value passed in is applicable.
So, for example,
passing a TAG value code to  \f(CWdwarf_get_ACCESS_name()\fP
is a coding error which libdwarf will process as if it was
an accessibility code value.
Examples of bad and good usage are:

.in +2
.DS
.FG "Examplezb dwarf_get_TAG_name()"
\f(CW
void examplezb(void)
{
    const char * out = 0;
    int res = 0;

    /* The following is wrong, do not do it! */
    res = dwarf_get_ACCESS_name(DW_TAG_entry_point,&out);
    /*  Nothing one does here with 'res' or 'out'
        is meaningful. */

    /* The following is meaningful.*/
    res = dwarf_get_TAG_name(DW_TAG_entry_point,&out);
    if( res == DW_DLV_OK) {
        /*  Here 'out' is a pointer one can use which
            points to the string "DW_TAG_entry_point". */
    } else {
        /*  Here 'out' has not been touched, it is
            uninitialized.  Do not use it. */
    }
}
\fP
.DE
.in -2




.H 3 "dwarf_get_ACCESS_name()"
Returns an accessibility code name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_AT_name()"
Returns an attribute code name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_ATE_name()"
Returns a base type encoding name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_ADDR_name()"
Returns an address type encoding name  through the \f(CWs_out\fP pointer.
As of this writing only  \f(CWDW_ADDR_none\fP is defined in  \f(CWdwarf.h\fP.
.H 3 "dwarf_get_ATCF_name()"
Returns a SUN code flag encoding name  through the \f(CWs_out\fP pointer.
This code flag is entirely a DWARF extension.
.H 3 "dwarf_get_CHILDREN_name()"
Returns a child determination name (which
is seen in the abbreviations section data) through the \f(CWs_out\fP pointer.
The only value this recognizes for a 'yes' value is 1.
As a flag value this is not quite correct (any non-zero value means
yes) but dealing with this is left up to client code (normally
compilers really do emit a value of 1 for a flag).
.H 3 "dwarf_get_children_name()"
Returns a child determination name through the \f(CWs_out\fP pointer,
though this version is really a libdwarf artifact.
The standard function is  \f(CWdwarf_get_CHILDREN_name()\fP
which appears just above.
As a flag value this is not quite correct (any non-zero value means
yes) but dealing with this is left up to client code (normally
compilers really do emit a value of 1 for a flag).
.H 3 "dwarf_get_CC_name()"
Returns  a calling convention case code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_CFA_name()"
Returns  a call frame information instruction
name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_DS_name()"
Returns a decimal sign code name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_DSC_name()"
Returns  a discriminant descriptor code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_EH_name()"
Returns  a GNU exception header
code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_END_name()"
Returns an endian code name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_FORM_name()"
Returns an form code name  through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_FRAME_name()"
Returns a frame code name  through the \f(CWs_out\fP pointer.
These are dependent on the particular ABI, so unless the
\f(CWdwarf.h\fP used to generate libdwarf matches your ABI
these names are unlikely to be very useful and certainly
won't be entirely appropriate.
.H 3 "dwarf_get_ID_name()"
Returns an identifier case code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_INL_name()"
Returns an inline code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_LANG_name()"
Returns a language code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_LLE_name()"
Returns a split-dwarf loclist code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_LNE_name()"
Returns  a line table extended
opcode code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_LNS_name()"
Returns  a line table standard
opcode code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_MACINFO_name()"
Returns  a macro information macinfo
code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_MACRO_name()"
Returns  a DWARF5 macro information macro
code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_OP_name()"
Returns  a DWARF expression operation
code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_ORD_name()"
Returns  an array ordering code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_TAG_name()"
Returns  a TAG name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_VIRTUALITY_name()"
Returns  a virtuality code name through the \f(CWs_out\fP pointer.
.H 3 "dwarf_get_VIS_name()"
Returns a visibility code name  through the \f(CWs_out\fP pointer.


.H 2 "Section Operations"
In checking DWARF in linkonce sections for correctness
it has been found useful to have certain section-oriented
operations when processing object files.
Normally these operations are not needed or useful
in a fully-linked executable or shared library.

While the code is written with Elf sections in mind,
it is quite possible to process  non-Elf objects
with code that implements certain function pointers
(see \f(CWstruct Dwarf_Obj_Access_interface_s\fP).

So far no one with such non-elf code has come forward
to open-source it.

.H 3 "dwarf_get_section_count()"
.DS
\f(CWint dwarf_get_section_count(
   Dwarf_Debug dbg) \fP
.DE
.P
Returns a count of the number of object sections found.
.P
If there is an incomplete or damaged dbg passed in
this can return -1;



.H 3 "dwarf_get_section_info_by_name()"
.DS
\f(CWint dwarf_get_section_info_by_name(
   const char *section_name,
   Dwarf_Addr *section_addr,
   Dwarf_Unsigned *section_size,
   Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_section_info_by_name()\fP
returns \f(CWDW_DLV_OK\fP if the section given by \f(CWsection_name\fP
was seen by libdwarf.
On success it sets \f(CW*section_addr\fP to the virtual address
assigned to the section by the linker or compiler and \f(CW*section_size\fP
to the size of the object section.

It returns DW_DLV_ERROR on error.
.H 3 "dwarf_get_section_info_by_index()"
.DS
\f(CWint dwarf_get_section_info_by_index(
   int section_index,
   const char **section_name,
   Dwarf_Addr *section_addr,
   Dwarf_Unsigned *section_size,
   Dwarf_Error *error)\fP
.DE
The function \f(CWdwarf_get_section_info_by_index()\fP
returns \f(CWDW_DLV_OK\fP if the section given by
\f(CWsection_index\fP
was seen by libdwarf.
\f(CW*section_addr\fP to the virtual address
assigned to the section by the linker or compiler
and \f(CW*section_size\fP
to the size of the object section.

No free or deallocate of information returned should be done by
callers.

.H 2 "Utility Operations"
These functions aid in the management of errors encountered when using
functions in the \fIlibdwarf\fP library and releasing memory allocated
as a result of a \fIlibdwarf\fP operation.
.P
For clients that wish to encode LEB numbers two
interfaces are provided to the producer code's
internal LEB function.

.H 3 "dwarf_errno()"
.DS
\f(CWDwarf_Unsigned dwarf_errno(
    Dwarf_Error error)\fP
.DE
The function
\f(CWdwarf_errno()\fP returns the error number corresponding
to the error specified by
\f(CWerror\fP.

.H 3 "dwarf_errmsg()"
.DS
\f(CWconst char* dwarf_errmsg(
    Dwarf_Error error)\fP
.DE
The function \f(CWdwarf_errmsg()\fP returns a pointer to a
null-terminated error message string
corresponding to the error specified by
\f(CWerror\fP.
The string
should not be deallocated using \f(CWdwarf_dealloc()\fP.

The string should be considered to be a temporary string.
That is, the returned pointer may become stale if you do
libdwarf calls on the
\f(CWDwarf_Debug\fP
instance
other than
\f(CWdwarf_errmsg()\fP
or
\f(CWdwarf_errno()\fP.
So copy the errmsg string ( or print
it) but do not depend on the pointer remaining valid
past other libdwarf calls to the
\f(CWDwarf_Debug\fP instance that detected an error.

.H 3 "dwarf_errmsg_by_number()"
.DS
\f(CWconst char* dwarf_errmsg_by_number(
    Dwarf_Unside errcode)\fP
.DE
The function 
\f(CWdwarf_errmsg_by_number()\fP returns a pointer to a
null-terminated error message string
corresponding to the error number specified by
\f(CWerrcode\fP.
The string
should not be deallocated or freed.
If the 
\f(CWerrcode\fP
is too large for the table of static error strings
a string reflecting that fact is returned.
.P
For some places in the code a 
\f(CWDwarf_Error()\fP
is inconvenient and this function
lets dwarfdump report better information
in those cases.

Function new December 19, 2018.

.H 3 "dwarf_get_endian_copy_function()"
.DS
\f(CWvoid (*dwarf_get_endian_copy_function(Dwarf_Debug /*dbg*/))
    (void *, const void * /*src*/, unsigned long /*srclen*/)\fP
.DE
When reader client code wants to extract endian-dependent
integers from dwarf and the existing interfaces won't do
that (for example in printing frame instructions as
done by dwarfdump) 
\f(CWdwarf_get_endian_copy_function\fP
helps by returning the proper copy function
needed, the one libdwarf itself uses.
The client code needs a bit of glue to finish the job,
as demonstrated by the ASNAR macro in 
dwarfdump/print_frames.c
.P
On success this returns a pointer to the correct copy function.
.P
On failure it returns the null pointer.
It's up to the client code to decide how to deal
with the situation.
In no reasonable case will the null pointer be returned.
.P
New December 2018.

.H 3 "dwarf_get_harmless_error_list()"
.DS
\f(CWint dwarf_get_harmless_error_list(Dwarf_Debug dbg,
    unsigned  count,
    const char ** errmsg_ptrs_array,
    unsigned * newerr_count);\fP
.DE
The harmless errors are not denoted by error returns from
the other libdwarf functions.  Instead, this function
returns strings of any harmless errors that have been
seen in the current object.  Clients never need call this, but
if a client wishes to report any such errors it may call.

Only a fixed number of harmless errors are recorded. It
is a circular list, so if more than the current maximum
is encountered older harmless error messages are lost.

The caller passes in a pointer to an array of pointer-to-char
as the argument \f(CWerrmsg_ptrs_array\fP.  The caller
must provide this array, libdwarf does not provide it.
The caller need not initialize the array elements.

The caller passes in the number of elements of the array of
pointer-to-char thru \f(CWcount\fP.   Since the

If there are no unreported harmless errors the function
returns \f(CWDW_DLV_NO_ENTRY\fP and the function arguments
are ignored.
Otherwise the function returns \f(CWDW_DLV_OK\fP
and uses the arguments.

\f(CWlibdwarf\fP assigns error strings to the errmsg_ptrs_array.
The MININUM(count-1, number of messages recorded) pointers are assigned
to the array.  The array is terminated with a NULL pointer.
(That is, one array entry is reserved for a NULL pointer).
So if \f(CWcount\fP is 5 up to 4 strings may be returned through
the array, and one array entry is set to NULL.

Because the list is circular and messages may have been dropped
the function also returns the actual error count of harmless
errors encountered through \f(CWnewerr_count\fP
(unless the argument is NULL, in which case it is ignored).

Each call to this function resets the circular error buffer and
the error count.
So think of this call as reporting harmless errors since the
last call to it.

The pointers returned through \f(CWerrmsg_ptrs_array\fP
are only valid till the next call to libdwarf.
Do not save the pointers, they become invalid.  Copy the strings
if you wish to save them.

Calling this function neither allocates any space in memory nor
frees any space in memory.


.H 3 "dwarf_insert_harmless_error()"
.DS
void dwarf_insert_harmless_error(Dwarf_Debug dbg,
    char * newerror);
.DE
This function is used to test
\f(CWdwarf_get_harmless_error_list\fP. It simply adds
a harmless error string.
There is little reason client code should use this function.
It exists so that the harmless error functions can be
easily tested for correctness and leaks.

.H 3 "dwarf_set_harmless_error_list_size()"
.DS
\f(CWunsigned dwarf_set_harmless_error_list_size(Dwarf_Debug dbg,
    unsigned maxcount)\fP
.DE
\f(CWdwarf_set_harmless_error_list_size\fP returns the
number of harmless error strings the library is currently
set to hold.
If \f(CWmaxcount\fP is non-zero the library changes the
maximum it will record to be \f(CWmaxcount\fP.

It is extremely unwise to make \f(CWmaxcount\fP large because
\f(CWlibdwarf\fP allocates space for \f(CWmaxcount\fP
strings immediately.

.P
The set of errors
enumerated in Figure \n(aX below were defined in Dwarf 1.
These errors are not used by the \f(CWlibdwarf\fP implementation
for Dwarf 2 or later.
.DS

.TS
center box, tab(:);
lfB lfB
l l.
SYMBOLIC NAME:DESCRIPTION
_
DW_DLE_NE:No error (0)
DW_DLE_VMM:Version of DWARF information newer
:than libdwarf
DW_DLE_MAP:Memory map failure
DW_DLE_LEE:Propagation of libelf error
DW_DLE_NDS:No debug section
DW_DLE_NLS:No line section
DW_DLE_ID:Requested information not associated
:with descriptor
DW_DLE_IOF:I/O failure
DW_DLE_MAF:Memory allocation failure
DW_DLE_IA:Invalid argument
DW_DLE_MDE:Mangled debugging entry
DW_DLE_MLE:Mangled line number entry
DW_DLE_FNO:File descriptor does not refer
:to an open file
DW_DLE_FNR:File is not a regular file
DW_DLE_FWA:File is opened with wrong access
DW_DLE_NOB:File is not an object file
DW_DLE_MOF:Mangled object file header
DW_DLE_EOLL:End of location list entries
DW_DLE_NOLL:No location list section
DW_DLE_BADOFF:Invalid offset
DW_DLE_EOS:End of section
DW_DLE_ATRUNC:Abbreviations section appears
:truncated
DW_DLE_BADBITC:Address size passed to
:dwarf bad
.TE
.FG "Dwarf Error Codes"
.DE

The set of errors returned by \f(CWLibdwarf\fP functions
is listed below.
The list does lengthen: the ones listed
here are not really a complete list.
Some of the errors are SGI specific.

.DS

.TS
center box, tab(:);
lfB lfB
l l.
SYMBOLIC NAME:DESCRIPTION
_
DW_DLE_DBG_ALLOC:Could not allocate Dwarf_Debug struct
DW_DLE_FSTAT_ERROR:Error in fstat()-ing object
DW_DLE_FSTAT_MODE_ERROR:Error in mode of object file
DW_DLE_INIT_ACCESS_WRONG:Incorrect access to dwarf_init()
DW_DLE_ELF_BEGIN_ERROR:Error in elf_begin() on object
DW_DLE_ELF_GETEHDR_ERROR:Error in elf_getehdr() on object
DW_DLE_ELF_GETSHDR_ERROR:Error in elf_getshdr() on object
DW_DLE_ELF_STRPTR_ERROR:Error in elf_strptr() on object
DW_DLE_DEBUG_INFO_DUPLICATE:Multiple .debug_info sections
DW_DLE_DEBUG_INFO_NULL:No data in .debug_info section
DW_DLE_DEBUG_ABBREV_DUPLICATE:Multiple .debug_abbrev
:sections
DW_DLE_DEBUG_ABBREV_NULL:No data in .debug_abbrev section
DW_DLE_DEBUG_ARANGES_DUPLICATE:Multiple .debug_arange
:sections
DW_DLE_DEBUG_ARANGES_NULL:No data in .debug_arange section
DW_DLE_DEBUG_LINE_DUPLICATE:Multiple .debug_line sections
DW_DLE_DEBUG_LINE_NULL:No data in .debug_line section
DW_DLE_DEBUG_LOC_DUPLICATE:Multiple .debug_loc sections
DW_DLE_DEBUG_LOC_NULL:No data in .debug_loc section
DW_DLE_DEBUG_MACINFO_DUPLICATE:Multiple .debug_macinfo
:sections
DW_DLE_DEBUG_MACINFO_NULL:No data in .debug_macinfo section
DW_DLE_DEBUG_PUBNAMES_DUPLICATE:Multiple .debug_pubnames
:sections
DW_DLE_DEBUG_PUBNAMES_NULL:No data in .debug_pubnames
:section
DW_DLE_DEBUG_STR_DUPLICATE:Multiple .debug_str sections
DW_DLE_DEBUG_STR_NULL:No data in .debug_str section
DW_DLE_CU_LENGTH_ERROR:Length of compilation-unit bad
DW_DLE_VERSION_STAMP_ERROR:Incorrect Version Stamp
DW_DLE_ABBREV_OFFSET_ERROR:Offset in .debug_abbrev bad
DW_DLE_ADDRESS_SIZE_ERROR:Size of addresses in target bad
DW_DLE_DEBUG_INFO_PTR_NULL:Pointer into .debug_info in
:DIE null
DW_DLE_DIE_NULL:Null Dwarf_Die
DW_DLE_STRING_OFFSET_BAD:Offset in .debug_str bad
DW_DLE_DEBUG_LINE_LENGTH_BAD:Length of .debug_line
:segment bad
DW_DLE_LINE_PROLOG_LENGTH_BAD:Length of .debug_line
: prolog bad
DW_DLE_LINE_NUM_OPERANDS_BAD:Number of operands to line
:instr bad
DW_DLE_LINE_SET_ADDR_ERROR:Error in DW_LNE_set_address
: instruction
.TE
.FG "Dwarf 2 Error Codes (continued below)"
.DE

.DS
.TS
center box, tab(:);
lfB lfB
l l.
SYMBOLIC NAME:DESCRIPTION
_

DW_DLE_LINE_EXT_OPCODE_BAD:Error in DW_EXTENDED_OPCODE
: instruction
DW_DLE_DWARF_LINE_NULL:Null Dwarf_line argument
DW_DLE_INCL_DIR_NUM_BAD:Error in included directory for
:given line
DW_DLE_LINE_FILE_NUM_BAD:File number in .debug_line bad
DW_DLE_ALLOC_FAIL:Failed to allocate required structs
DW_DLE_DBG_NULL:Null Dwarf_Debug argument
DW_DLE_DEBUG_FRAME_LENGTH_BAD:Error in length of frame
DW_DLE_FRAME_VERSION_BAD:Bad version stamp for frame
DW_DLE_CIE_RET_ADDR_REG_ERROR:Bad register specified for
:return address
DW_DLE_FDE_NULL:Null Dwarf_Fde argument
DW_DLE_FDE_DBG_NULL:No Dwarf_Debug associated with FDE
DW_DLE_CIE_NULL:Null Dwarf_Cie argument
DW_DLE_CIE_DBG_NULL:No Dwarf_Debug associated with CIE
DW_DLE_FRAME_TABLE_COL_BAD:Bad column in frame table
:specified
DW_DLE_PC_NOT_IN_FDE_RANGE:PC requested not in address range of FDE
DW_DLE_CIE_INSTR_EXEC_ERROR:Error in executing instructions in CIE
DW_DLE_FRAME_INSTR_EXEC_ERROR:Error in executing instructions in FDE
DW_DLE_FDE_PTR_NULL:Null Pointer to Dwarf_Fde specified
DW_DLE_RET_OP_LIST_NULL:No location to store pointer to Dwarf_Frame_Op
DW_DLE_LINE_CONTEXT_NULL:Dwarf_Line has no context
DW_DLE_DBG_NO_CU_CONTEXT:dbg has no CU context for dwarf_siblingof()
DW_DLE_DIE_NO_CU_CONTEXT:Dwarf_Die has no CU context
DW_DLE_FIRST_DIE_NOT_CU:First DIE in CU not DW_TAG_compilation_unit
DW_DLE_NEXT_DIE_PTR_NULL:Error in moving to next DIE in .debug_info
DW_DLE_DEBUG_FRAME_DUPLICATE:Multiple .debug_frame sections
DW_DLE_DEBUG_FRAME_NULL:No data in .debug_frame section
DW_DLE_ABBREV_DECODE_ERROR:Error in decoding abbreviation
DW_DLE_DWARF_ABBREV_NULL:Null Dwarf_Abbrev specified
DW_DLE_ATTR_NULL:Null Dwarf_Attribute specified
DW_DLE_DIE_BAD:DIE bad
DW_DLE_DIE_ABBREV_BAD:No abbreviation found for code in DIE
DW_DLE_ATTR_FORM_BAD:Inappropriate attribute form for attribute
DW_DLE_ATTR_NO_CU_CONTEXT:No CU context for Dwarf_Attribute struct
DW_DLE_ATTR_FORM_SIZE_BAD:Size of block in attribute value bad
DW_DLE_ATTR_DBG_NULL:No Dwarf_Debug for Dwarf_Attribute struct
DW_DLE_BAD_REF_FORM:Inappropriate form for reference attribute
DW_DLE_ATTR_FORM_OFFSET_BAD:Offset reference attribute outside current CU
DW_DLE_LINE_OFFSET_BAD:Offset of lines for current CU outside .debug_line
DW_DLE_DEBUG_STR_OFFSET_BAD:Offset into .debug_str past its end
DW_DLE_STRING_PTR_NULL:Pointer to pointer into .debug_str NULL
DW_DLE_PUBNAMES_VERSION_ERROR:Version stamp of pubnames incorrect
DW_DLE_PUBNAMES_LENGTH_BAD:Read pubnames past end of .debug_pubnames
DW_DLE_GLOBAL_NULL:Null Dwarf_Global specified
DW_DLE_GLOBAL_CONTEXT_NULL:No context for Dwarf_Global given
DW_DLE_DIR_INDEX_BAD:Error in directory index read
.TE
.FG "Dwarf 2 Error Codes (continued below)"
.DE

.DS
.TS
center box, tab(:);
lfB lfB
l l.
SYMBOLIC NAME:DESCRIPTION
_
DW_DLE_LOC_EXPR_BAD:Bad operator read for location expression
DW_DLE_DIE_LOC_EXPR_BAD:Expected block value for attribute
:not found
DW_DLE_OFFSET_BAD:Offset for next compilation-unit in
:.debug_info bad
DW_DLE_MAKE_CU_CONTEXT_FAIL:Could not make CU context
DW_DLE_ARANGE_OFFSET_BAD:Offset into .debug_info in
:.debug_aranges bad
DW_DLE_SEGMENT_SIZE_BAD:Segment size will be 0 for MIPS
:processorsand should always be < 8.
DW_DLE_ARANGE_LENGTH_BAD:Length of arange section in
:.debug_arange bad
DW_DLE_ARANGE_DECODE_ERROR:Aranges do not end at end
:of .debug_aranges
DW_DLE_ARANGES_NULL:NULL pointer to Dwarf_Arange specified
DW_DLE_ARANGE_NULL:NULL Dwarf_Arange specified
DW_DLE_NO_FILE_NAME:No file name for Dwarf_Line struct
DW_DLE_NO_COMP_DIR:No Compilation directory for
:compilation-unit
DW_DLE_CU_ADDRESS_SIZE_BAD:CU header address size not
:match Elf class
DW_DLE_ELF_GETIDENT_ERROR:Error in elf_getident() on object
DW_DLE_NO_AT_MIPS_FDE:DIE does not have
:DW_AT_MIPS_fde attribute
DW_DLE_NO_CIE_FOR_FDE:No CIE specified for FDE
DW_DLE_DIE_ABBREV_LIST_NULL:No abbreviation for the code
:in DIE found
DW_DLE_DEBUG_FUNCNAMES_DUPLICATE:Multiple .debug_funcnames sections
DW_DLE_DEBUG_FUNCNAMES_NULL:No data in .debug_funcnames section
DW_DLE_DEBUG_FUNCNAMES_VERSION_ERROR:Version stamp in
:.debug_funcnames bad
DW_DLE_DEBUG_FUNCNAMES_LENGTH_BAD:Length error in reading
:.debug_funcnames
DW_DLE_FUNC_NULL:NULL Dwarf_Func specified
DW_DLE_FUNC_CONTEXT_NULL:No context for Dwarf_Func struct
DW_DLE_DEBUG_TYPENAMES_DUPLICATE:Multiple .debug_typenames sections
DW_DLE_DEBUG_TYPENAMES_NULL:No data in .debug_typenames section
DW_DLE_DEBUG_TYPENAMES_VERSION_ERROR:Version stamp in
:.debug_typenames bad
DW_DLE_DEBUG_TYPENAMES_LENGTH_BAD:Length error in reading
:.debug_typenames
DW_DLE_TYPE_NULL:NULL Dwarf_Type specified
DW_DLE_TYPE_CONTEXT_NULL:No context for Dwarf_Type given
DW_DLE_DEBUG_VARNAMES_DUPLICATE:Multiple .debug_varnames sections
DW_DLE_DEBUG_VARNAMES_NULL:No data in .debug_varnames section
DW_DLE_DEBUG_VARNAMES_VERSION_ERROR:Version stamp in
:.debug_varnames bad
DW_DLE_DEBUG_VARNAMES_LENGTH_BAD:Length error in reading
:.debug_varnames
.TE
.FG "Dwarf 2 Error Codes (continued below)"
.DE

.DS
.TS
center box, tab(:);
lfB lfB
l l.
SYMBOLIC NAME:DESCRIPTION
_
DW_DLE_VAR_NULL:NULL Dwarf_Var specified
DW_DLE_VAR_CONTEXT_NULL:No context for Dwarf_Var given
DW_DLE_DEBUG_WEAKNAMES_DUPLICATE:Multiple .debug_weaknames section
DW_DLE_DEBUG_WEAKNAMES_NULL:No data in .debug_varnames section
DW_DLE_DEBUG_WEAKNAMES_VERSION_ERROR:Version stamp in
:.debug_varnames bad
DW_DLE_DEBUG_WEAKNAMES_LENGTH_BAD:Length error in reading
:.debug_weaknames
DW_DLE_WEAK_NULL:NULL Dwarf_Weak specified
DW_DLE_WEAK_CONTEXT_NULL:No context for Dwarf_Weak given
.TE
.FG "Dwarf 2 Error Codes"
.DE

This list of errors is not complete;
additional errors have been added.
Some of the above errors may be unused.
Errors may not have the same meaning in different releases.
Since most error codes are returned from only one  place
(or a very small number of places) in the source
it is normally very useful to simply search the
\f(CWlibdwarf\fP source to find
out where a particular error code is generated.

.H 3 "dwarf_dealloc()"
.DS
\f(CWvoid dwarf_dealloc(
    Dwarf_Debug dbg,
    void* space,
    Dwarf_Unsigned type)\fP
.DE
The function \f(CWdwarf_dealloc\fP frees the dynamic storage pointed
to by \f(CWspace\fP, and allocated to the given \f(CWDwarf_Debug\fP.
The argument
\f(CWtype\fP is an integer code that specifies the allocation
type of the region pointed to by the \f(CWspace\fP.  Refer to section
4 for details on \fIlibdwarf\fP memory management.

.H 3 "dwarf_encode_leb128()"
.DS
int dwarf_encode_leb128(Dwarf_Unsigned val,
    int * nbytes,
    char * space,
    int   splen);
.DE
The function \f(CWdwarf_encode_leb128\fP
encodes the value \f(CWval\fP
in the caller-provided buffer that \f(CWspace\fP
points to.  The caller-provided buffer must
be at least \f(CWsplen\fP bytes long.

The function returns \f(CWDW_DLV_OK\fP
if the encoding succeeds.
If  \f(CWsplen\fP is too small to encode the
value, \f(CWDW_DLV_ERROR\fP will
be returned.

If the call succeeds,
the number of bytes of \f(CWspace\fP that are used
in the encoding are returned through the pointer \f(CWnbytes\fP


.H 3 "dwarf_encode_signed_leb128()"
.DS
int dwarf_encode_signed_leb128(Dwarf_Signed val,
    int * nbytes,
    char * space,
    int splen);
.DE

The function \f(CWdwarf_encode_signed_leb128\fP
is the same as \f(CWdwarf_encode_leb128\fP except that
the argument  \f(CWval\fP is signed.

.SK
.S
.TC 1 1 4
.CS
