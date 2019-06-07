/*
  Copyright (C) 2010-2018 David Anderson.  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of the example nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY David Anderson ''AS IS'' AND ANY
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL David Anderson BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

// irepattrtodbg.cc

#include "config.h"
#ifdef HAVE_UNUSED_ATTRIBUTE
#define  UNUSEDARG __attribute__ ((unused))
#else
#define  UNUSEDARG
#endif


/* Windows specific header files */
#if defined(_WIN32) && defined(HAVE_STDAFX_H)
#include "stdafx.h"
#endif /* HAVE_STDAFX_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h> // for exit
#include <iostream>
#include <sstream> // For BldName
#include <iomanip> // iomanp for setw etc
#include <string>
#include <list>
#include <map>
#include <vector>
#include <string.h> // For memset etc
#include "general.h"
#include "strtabdata.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "irepresentation.h"
#include "ireptodbg.h"
#include "irepattrtodbg.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::list;
using std::map;

static Dwarf_Error error;
static unsigned fakeaddrnum;

// We are not going to 'validate' the FORM for this Attribute.
// or this Die.  We just assume that what we are handed is
// what we are to produce.  We do test the attribute
// at times, partly to ensure we use as many of the dwarf_add_AT*
// functions as possible.

// Correctness/appropriateness must be evaluated elsewhere.

void
AddAttrToDie(Dwarf_P_Debug dbg,
    IRepresentation & Irep,
    IRCUdata  &cu,
    Dwarf_P_Die outdie,
    IRDie & irdie, //UNUSEDARG
    IRAttr &irattr)
{
    int attrnum = irattr.getAttrNum();
    enum Dwarf_Form_Class formclass = irattr.getFormClass();
    // IRForm is an abstract base class.
    IRForm *form_a = irattr.getFormData();

    switch(formclass) {
    case DW_FORM_CLASS_UNKNOWN:
        cerr << "ERROR AddAttrToDie: Impossible DW_FORM_CLASS_UNKNOWN, attrnum "
            <<attrnum << endl;
        break;
    case DW_FORM_CLASS_ADDRESS:
        {
        IRFormAddress *f = dynamic_cast<IRFormAddress *>(form_a);
        if (!f) {
            cerr << "ERROR Impossible DW_FORM_CLASS_ADDRESS cast fails, attrnum "
                <<attrnum << endl;
            break;
        }
        // FIXME: do better creating a symbol:  try to match original
        // or specified input.
        Dwarf_Addr addr = f->getAddress();

        string symname = BldName("addrsym",fakeaddrnum++);
        Dwarf_Addr pcval = addr;

        ElfSymbols& es = Irep.getElfSymbols();
        ElfSymIndex esi = es.addElfSymbol(pcval,symname);
        Dwarf_Unsigned sym_index =  esi.getSymIndex();

        // FIXME: we should  allow for DW_FORM_indirect here.
        // Relocation later will fix value.
        Dwarf_P_Attribute a = dwarf_add_AT_targ_address_b(dbg,
            outdie,attrnum,0,sym_index,&error);
        if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
            cerr << "ERROR dwarf_add_AT_targ_address fails, attrnum "
                <<attrnum << endl;

        }
        }
        break;
    case DW_FORM_CLASS_BLOCK:
        {
        //FIXME
        }
        break;
    case DW_FORM_CLASS_CONSTANT:
        {
        IRFormConstant *f = dynamic_cast<IRFormConstant *>(form_a);
        Dwarf_Half formv = f->getFinalForm();
        // FIXME: Handle form indirect
        IRFormConstant::Signedness sn = f->getSignedness();
        Dwarf_P_Attribute a = 0;

        if (formv == DW_FORM_data16) {
            Dwarf_Form_Data16 val = f->getData16Val();
            int res = dwarf_add_AT_data16(outdie,
                attrnum,&val,&a,&error);
            if (res != DW_DLV_OK) {
                cerr <<
                "ERROR AddAttrToDie: "
                "dwarf_add_AT_ data16 class constant fails,"
                " attrnum " << attrnum <<
                " res "<<res << endl;
            }
            break;
        } else if (formv == DW_FORM_implicit_const) {
            int res = 0;
            Dwarf_Signed sval = f->getSignedVal();

            res = dwarf_add_AT_implicit_const(outdie,attrnum,
                sval,&a,&error);
            if (res != DW_DLV_OK) {
                cerr <<
                    "ERROR AddAttrToDie: "
                    "dwarf_add_AT_implicit_const fails,"
                    " attrnum " << attrnum <<
                    " res "<<res << endl;
            }
            break;
        }
        if (sn == IRFormConstant::SIGNED) {
            Dwarf_Signed sval = f->getSignedVal();
            if (formv == DW_FORM_sdata) {
                a = dwarf_add_AT_any_value_sleb(
                    outdie,attrnum,
                    sval,&error);
            } else {
                //cerr << "ERROR how can we know "
                //    "a non-sdata const is signed?, attrnum " <<
                //    attrnum <<endl;
                a = dwarf_add_AT_signed_const(dbg,
                    outdie,attrnum,
                    sval,&error);
            }
        } else {
            Dwarf_Unsigned uval_i = f->getUnsignedVal();
            if (formv == DW_FORM_udata) {
                a = dwarf_add_AT_any_value_uleb(
                    outdie,attrnum,
                    uval_i,&error);
            } else {
                a = dwarf_add_AT_unsigned_const(dbg,
                    outdie,attrnum,
                    uval_i,&error);
            }
        }
        if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
            cerr << "ERROR dwarf_add_AT_ class constant fails,"
                " BADATTR on attrnum "
                << attrnum << " Continuing" << endl;

        }
        }
        break;
    case DW_FORM_CLASS_EXPRLOC:
        {
        //FIXME
        }
        break;
    case DW_FORM_CLASS_FLAG:
        {
        IRFormFlag *f = dynamic_cast<IRFormFlag *>(form_a);
        if (!f) {
            cerr << "ERROR Impossible DW_FORM_CLASS_FLAG cast fails, attrnum "
                <<attrnum << endl;
            break;
        }
        // FIXME: handle indirect form (libdwarf needs feature).
        // FIXME: handle implicit flag (libdwarf needs feature).
        // FIXME: rel type ok?
        Dwarf_P_Attribute a =
            dwarf_add_AT_flag(dbg,outdie,attrnum,f->getFlagVal(),&error);
        if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
            cerr << "ERROR dwarf_add_AT_flag fails, attrnum "
                <<attrnum << endl;
        }
        }
        break;

    case DW_FORM_CLASS_LINEPTR:
        {
        // The DW_AT_stmt_list attribute is generated
        // as a side effect of dwarf_transform_to_disk_form
        // if producer line-info-creating functions were called.
        // So we ignore this attribute here, it is
        // automatic.
        }
        break;
    case DW_FORM_CLASS_LOCLISTPTR:
        {
        //FIXME. Needs support in dwarf producer(libdwarf)
        }
        break;
    case DW_FORM_CLASS_MACPTR:
        {
        // The DW_AT_macro_info attribute is generated
        // as a side effect of dwarf_transform_to_disk_form
        // if producer macro-creating functions were called.
        // So we ignore this attribute here, it is
        // automatic.
        }
        break;
    case DW_FORM_CLASS_RANGELISTPTR:
        {
        //FIXME. Needs support in dwarf producer(libdwarf)
        }
        break;
    case DW_FORM_CLASS_REFERENCE:
        {
        // Can be a local CU  reference to a DIE, or a
        // global DIE reference  or a
        // sig8 reference.
        //FIXME
        IRFormReference *r = dynamic_cast<IRFormReference *>(form_a);
        if (!r) {
            cerr << "ERROR Impossible DW_FORM_CLASS_REFERENCE cast fails, attrnum "
                <<attrnum << endl;
            break;
        }

        IRFormReference::RefType reftype = r->getReferenceType();
        switch (reftype) {
        case IRFormReference::RT_NONE:
            cerr << "ERROR CLASS REFERENCE unknown reftype "
                <<attrnum << endl;
            break;
        case IRFormReference::RT_GLOBAL:
            // FIXME. Not handled.
            break;
        case IRFormReference::RT_CUREL:
            {
            IRDie *targetofref = r->getTargetInDie();
            Dwarf_P_Die targetoutdie = 0;
            if(targetofref) {
                targetoutdie = targetofref->getGeneratedDie();
            }
            if(!targetoutdie) {
                if(!targetofref) {
                    cerr << "ERROR CLASS REFERENCE targetdie of reference unknown"
                        <<attrnum << endl;
                    break;
                }
                // We must add the attribute when we have the
                // target Dwarf_P_Die, which should get set shortly.
                // And do the  dwarf_add_AT_reference() then.
                // Before transform_to_disk_form.
                // NULL targetoutdie allowed here.
                // Arranging DIE order so there were no forward-refs
                // could be difficult.
                // Another option would be two-pass: first create
                // all the DIEs then all the attributes for each.
                Dwarf_P_Attribute a =
                    dwarf_add_AT_reference_b(dbg,outdie,attrnum,
                    /*targetoutdie */NULL,&error);
                if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
                    cerr << "ERROR dwarf_add_AT_reference fails, "
                        "attrnum with not yet known targetoutdie "
                        << IToHex(attrnum) << endl;
                } else {
                    ClassReferenceFixupData x(dbg,attrnum,outdie,targetofref);
                    cu.insertClassReferenceFixupData(x);
                }
                break;
            }
            Dwarf_P_Attribute a =
                dwarf_add_AT_reference(dbg,outdie,attrnum,
                targetoutdie,&error);
            if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
                cerr << "ERROR dwarf_add_AT_reference fails, "
                    "attrnum with known targetoutdie "
                    << IToHex(attrnum) << endl;
            }
            }
            break;
        case IRFormReference::RT_SIG:
            {
            Dwarf_P_Attribute a =
                dwarf_add_AT_with_ref_sig8(outdie,attrnum,
                r->getSignature(),&error);
            if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
                cerr << "ERROR dwarf_add_AT_ref_sig8 fails, attrnum "
                    << IToHex(attrnum) << endl;
            }
            }
        }
        }
        break;
    case DW_FORM_CLASS_STRING:
        {
        IRFormString *f = dynamic_cast<IRFormString *>(form_a);
        if (!f) {
            cerr << "ERROR Impossible DW_FORM_CLASS_STRING cast fails, attrnum "
                <<attrnum << endl;
            break;
        }
        Dwarf_P_Attribute a = 0;
        // We know libdwarf does not change the string. Historical mistake
        // not making it a const char * argument.
        // Ugly cast.
        // FIXME: handle indirect form (libdwarf needs feature).
        // FIXME: rel type ok?
        char *mystr = const_cast<char *>(f->getString().c_str());

        switch(attrnum) {
        case DW_AT_name:
            a = dwarf_add_AT_name(outdie,mystr,&error);
            break;
        case DW_AT_producer:
            a = dwarf_add_AT_producer(outdie,mystr,&error);
            break;
        case DW_AT_comp_dir:
            a = dwarf_add_AT_comp_dir(outdie,mystr,&error);
            break;
        default:
            a = dwarf_add_AT_string(dbg,outdie,attrnum,mystr,
                &error);
            break;
        }
        if( reinterpret_cast<Dwarf_Addr>(a) == DW_DLV_BADADDR) {
            cerr << "ERROR dwarf_add_AT_string fails, attrnum "
                <<attrnum << endl;
        }
        }
        break;
    case DW_FORM_CLASS_FRAMEPTR: // SGI/MIPS/IRIX only.
        {
        //FIXME
        }
        break;
    default:
        cerr << "ERROR Impossible DW_FORM_CLASS  "<<
            static_cast<int>(formclass)
            <<attrnum << endl;
        //FIXME
    }
    return;
}


void
IRCUdata::updateClassReferenceTargets()
{
    for(std::list<ClassReferenceFixupData>::iterator it =
        classReferenceFixupList_.begin();
        it != classReferenceFixupList_.end();
        ++it) {
            IRDie* d = it->target_;
            Dwarf_P_Die sourcedie = it->sourcedie_;
            Dwarf_P_Die targetdie = d->getGeneratedDie();
            Dwarf_Error lerror = 0;

            int res = dwarf_fixup_AT_reference_die(it->dbg_,
                it->attrnum_,sourcedie,targetdie,&lerror);
            if(res != DW_DLV_OK) {
                cerr << "Improper dwarf_fixup_AT_reference_die call"
                    << endl;
            }
        }
}
