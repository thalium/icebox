/*
  Copyright (C) 2010-2013 David Anderson.  All rights reserved.

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

// ireptodbg.cc

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
#include <string>
#include <list>
#include <map>
#include <vector>
#include <string.h> // For memset etc
#include "strtabdata.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "irepresentation.h"
#include "ireptodbg.h"
#include "irepattrtodbg.h"
#include "general.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;
using std::map;
using std::list;
using std::map;

static Dwarf_Error error;

typedef std::map<std::string,unsigned> pathToUnsignedType;


// The first special transformation is converting DW_AT_high_pc
// from FORM_addr to an offset and we choose FORM_uleb
// The attrs ref passed in is (sometimes) used to generate
// a new attrs list for the caller.
static void
specialAttrTransformations(Dwarf_P_Debug dbg UNUSEDARG,
    IRepresentation & Irep UNUSEDARG,
    Dwarf_P_Die ourdie UNUSEDARG,
    IRDie &inDie,
    list<IRAttr>& attrs,
    unsigned level UNUSEDARG)
{
    if(!cmdoptions.transformHighpcToConst) {
        // No transformation of this sort requested.
        return;
    }
    Dwarf_Half dietag = inDie.getTag();
    if(dietag != DW_TAG_subprogram) {
        return;
    }
    bool foundhipc= false;
    bool foundlopc= false;
    Dwarf_Addr lopcval = 0;
    Dwarf_Addr hipcval = 0;
    for (list<IRAttr>::iterator it = attrs.begin();
        it != attrs.end();
        it++) {
        IRAttr & attr = *it;
        Dwarf_Half attrnum = attr.getAttrNum();
        Dwarf_Half attrform = attr.getFinalForm();
        Dwarf_Form_Class formclass = attr.getFormClass();
        if(attrnum == DW_AT_high_pc) {
            if (attrform == DW_FORM_udata) {
                // Already the right form for the test.
                // Nothing to do.
                return;
            }
            if (formclass != DW_FORM_CLASS_ADDRESS) {
                return;
            }
            IRForm*f = attr.getFormData();
            IRFormAddress *f2 = dynamic_cast<IRFormAddress *>(f);
            hipcval = f2->getAddress();
            foundhipc = true;
            continue;
        }
        if(attrnum == DW_AT_low_pc) {
            if (formclass != DW_FORM_CLASS_ADDRESS) {
                return;
            }
            IRForm*f = attr.getFormData();
            IRFormAddress *f2 = dynamic_cast<IRFormAddress *>(f);
            lopcval = f2->getAddress();
            foundlopc = true;
            continue;
        }
        continue;
    }
    if(!foundlopc || !foundhipc) {
        return;
    }
    Dwarf_Addr hipcoffset = hipcval - lopcval;
    // Now we create a revised attribute.
    list<IRAttr> revisedattrs;
    for (list<IRAttr>::iterator it = attrs.begin();
        it != attrs.end();
        it++) {
        IRAttr & attr = *it;
        Dwarf_Half attrnum = attr.getAttrNum();
        if(attrnum == DW_AT_high_pc) {
            // Here we want to create a constant form
            // to test that a const high_pc works.
            // This is new in DWARF5.
            IRAttr attr2(attrnum,
                DW_FORM_udata,
                DW_FORM_udata);
            attr2.setFormClass(DW_FORM_CLASS_CONSTANT);
            IRFormConstant *f = new IRFormConstant(
                DW_FORM_udata,
                DW_FORM_udata,
                DW_FORM_CLASS_CONSTANT,
                IRFormConstant::UNSIGNED,
                hipcoffset,
                0);
            attr2.setFormData(f);
            revisedattrs.push_back(attr2);
            foundhipc = true;
            continue;
        }
        if(attrnum == DW_AT_low_pc) {
            foundlopc = true;
            revisedattrs.push_back(attr);
            continue;
        }
        revisedattrs.push_back(attr);
        continue;
    }
    // Now we make the attr list have the revised attribute.
    attrs = revisedattrs;
}

/* Create a data16 data item out of nothing... */
static void
addData16DataItem(Dwarf_P_Debug dbg UNUSEDARG,
    IRepresentation & Irep UNUSEDARG,
    Dwarf_P_Die ourdie UNUSEDARG,
    IRDie &inDie,
    IRDie &inParent,
    list<IRAttr>& attrs,
    unsigned level)
{
    static bool alreadydone = false;

    if (alreadydone) {
        return;
    }
    if(!cmdoptions.adddata16) {
        // No transformation of this sort requested.
        return;
    }
    if (level < 2) {
        return;
    }

    Dwarf_Half dietag = inDie.getTag();
    Dwarf_Half parenttag = inParent.getTag();
    if(dietag != DW_TAG_variable || parenttag != DW_TAG_subprogram) {
        return;
    }
    list<IRAttr> revisedattrs;
    for (list<IRAttr>::iterator it = attrs.begin();
        it != attrs.end();
        it++) {
        IRAttr & attr = *it;
        Dwarf_Half attrnum = attr.getAttrNum();
        if(attrnum == DW_AT_name){
            continue;
        }
        if(attrnum == DW_AT_const_value){
            continue;
        }
        revisedattrs.push_back(attr);
    }


    //    add two new attrs.
    Dwarf_Half attrnum = DW_AT_name;
    const char *attrname("vardata16");
    IRAttr attr2(attrnum,
        DW_FORM_string,
        DW_FORM_string);
    attr2.setFormClass(DW_FORM_CLASS_STRING);
    IRFormString *f = new IRFormString();
    f->setInitialForm(DW_FORM_string);
    f->setFinalForm(DW_FORM_string);
    f->setString(attrname);
    attr2.setFormData(f);
    revisedattrs.push_back(attr2);


    Dwarf_Form_Data16  data16 = {
        0x01,0x08,
        0x02,0x07,
        0x03,0x06,
        0x04,0x05,
        0x05,0x04,
        0x06,0x03,
        0x07,0x02,
        0x08,0x01
    };
    IRAttr attrc(DW_AT_const_value, DW_FORM_data16,DW_FORM_data16);
    attrc.setFormClass(DW_FORM_CLASS_CONSTANT);
    IRFormConstant *fc = new IRFormConstant(
        DW_FORM_data16,
        DW_FORM_data16,
        DW_FORM_CLASS_CONSTANT,
        data16);
    attrc.setFormData(fc);
    revisedattrs.push_back(attrc);
    attrs = revisedattrs;
    alreadydone = true;
}

int testvals[3] = {
-2018,
-2019,
-2018
};
const char *testnames[3] = {
"myimplicitconst1",
"myimplicitconst2newabbrev",
"myimplicitconst3share1abbrev"
};

static void
addImplicitConstItem(Dwarf_P_Debug dbg UNUSEDARG,
    IRepresentation & Irep UNUSEDARG,
    Dwarf_P_Die ourdie UNUSEDARG,
    IRDie &inDie,
    IRDie &inParent,
    list<IRAttr>& attrs,
    unsigned level)
{
    static int alreadydone = 0;

    if (alreadydone > 2) {
        // The limit here MUST be below the size of
        // testvals[] and testnames[] above.
        // Some abbrevs should match
        // if the test case is right. Others not.
        return;
    }
    if(!cmdoptions.addimplicitconst) {
        // No transformation of this sort requested.
        return;
    }
    if (level < 2) {
        return;
    }

    Dwarf_Half dietag = inDie.getTag();
    Dwarf_Half parenttag = inParent.getTag();
    if(dietag != DW_TAG_variable || parenttag != DW_TAG_subprogram) {
        return;
    }
    list<IRAttr> revisedattrs;
    for (list<IRAttr>::iterator it = attrs.begin();
        it != attrs.end();
        it++) {
        IRAttr & attr = *it;
        Dwarf_Half attrnum = attr.getAttrNum();
        if(attrnum == DW_AT_name){
            continue;
        }
        if(attrnum == DW_AT_const_value){
            continue;
        }
        revisedattrs.push_back(attr);
    }


    //    add two new attrs.
    Dwarf_Half attrnum = DW_AT_name;
    const char *attrname(testnames[alreadydone]);
    IRAttr attr2(attrnum,
        DW_FORM_string,
        DW_FORM_string);
    attr2.setFormClass(DW_FORM_CLASS_STRING);
    IRFormString *f = new IRFormString();
    f->setInitialForm(DW_FORM_implicit_const);
    f->setFinalForm(DW_FORM_implicit_const);
    f->setString(attrname);

    attr2.setFormData(f);
    revisedattrs.push_back(attr2);

    Dwarf_Signed myconstval = testvals[alreadydone];

    IRAttr attrc(DW_AT_const_value,
        DW_FORM_implicit_const,DW_FORM_implicit_const);
    attrc.setFormClass(DW_FORM_CLASS_CONSTANT);
    IRFormConstant *fc = new IRFormConstant(
        DW_FORM_implicit_const,
        DW_FORM_implicit_const,
        DW_FORM_CLASS_CONSTANT,
        IRFormConstant::SIGNED,
        0,myconstval);
    attrc.setFormData(fc);
    revisedattrs.push_back(attrc);
    attrs = revisedattrs;
    ++alreadydone;
}



// Here we emit all the DIEs for a single Die and
// its children.  When level == 0 the inDie is
// the CU die.
static Dwarf_P_Die
HandleOneDieAndChildren(Dwarf_P_Debug dbg,
    IRepresentation &Irep,
    IRCUdata &cu,
    IRDie    &inDie,
    IRDie    &inParent,
    unsigned level)
{
    list<IRDie>& children = inDie.getChildren();
    // We create our target DIE first so we can link
    // children to it, but add no content yet.
    Dwarf_P_Die gendie = dwarf_new_die(dbg,inDie.getTag(),NULL,NULL,
        NULL,NULL,&error);
    if (reinterpret_cast<Dwarf_Addr>(gendie) == DW_DLV_BADADDR) {
        cerr << "Die creation failure.  "<< endl;
        exit(1);
    }
    inDie.setGeneratedDie(gendie);

    Dwarf_P_Die lastch = 0;
    for ( list<IRDie>::iterator it = children.begin();
        it != children.end();
        it++) {
        IRDie & ch = *it;
        Dwarf_P_Die chp = HandleOneDieAndChildren(dbg,Irep,
            cu,ch,inDie,level+1);
        Dwarf_P_Die res = 0;
        if(lastch) {
            // Link to right of earlier sibling.
            res = dwarf_die_link(chp,NULL,NULL,lastch,NULL,&error);
        } else {
            // Link as first child.
            res  = dwarf_die_link(chp,gendie,NULL,NULL, NULL,&error);
        }
        if (reinterpret_cast<Dwarf_Addr>(res) == DW_DLV_BADADDR) {
            cerr << "Die link failure.  "<< endl;
            exit(1);
        }
        lastch = chp;
    }
    list<IRAttr>& attrs = inDie.getAttributes();

    // Now any special transformations to the attrs list.
    specialAttrTransformations(dbg,Irep,gendie,inDie,attrs,level);
    addData16DataItem(dbg,Irep,gendie,inDie,inParent,attrs,level);
    addImplicitConstItem(dbg,Irep,gendie,inDie,inParent,attrs,level);

    // Now we add attributes (content), if any, to the
    // output die 'gendie'.
    for (list<IRAttr>::iterator it = attrs.begin();
        it != attrs.end();
        it++) {
        IRAttr & attr = *it;

        AddAttrToDie(dbg,Irep,cu,gendie,inDie,attr);
    }
    return gendie;
}

static void
HandleLineData(Dwarf_P_Debug dbg,
    IRepresentation & Irep UNUSEDARG,
    IRCUdata&cu)
{
    Dwarf_Error lerror = 0;
    // We refer to files by fileno, this builds an index.
    pathToUnsignedType pathmap;

    IRCULineData& ld = cu.getCULines();
    std::vector<IRCULine> & cu_lines = ld.get_cu_lines();
    //std::vector<IRCUSrcfile> &cu_srcfiles  = ld.get_cu_srcfiles();
    if(cu_lines.empty()) {
        // No lines data to emit, do nothing.
        return;
    }
    // To start with, we are doing a trivial generation here.
    // To be refined 'soon'.  FIXME
    // Initially we don't worry about dwarf_add_directory_decl().

    bool firstline = true;
    bool addrsetincu = false;
    for(unsigned k = 0; k < cu_lines.size(); ++k) {
        IRCULine &li = cu_lines[k];
        const std::string&path = li.getpath();
        unsigned pathindex = 0;
        pathToUnsignedType::const_iterator it = pathmap.find(path);

        if(it == pathmap.end()) {
            Dwarf_Error l2error = 0;
            Dwarf_Unsigned idx = dwarf_add_file_decl(
                dbg,const_cast<char *>(path.c_str()),
                0,0,0,&l2error);
            if((Dwarf_Signed)idx == DW_DLV_NOCOUNT) {
                cerr << "Error from dwarf_add_file_decl() on " <<
                    path << endl;
                exit(1);
            }
            pathindex = idx;
            pathmap[path] = pathindex;
        } else {
            pathindex = it->second;
        }
        Dwarf_Addr a = li.getaddr();
        bool addrsetinline = li.getaddrset();
        bool endsequence = li.getendsequence();
        if(firstline || !addrsetincu) {
            // We fake an elf sym index here.
            Dwarf_Unsigned elfsymidx = 0;
            if(firstline && !addrsetinline) {
                cerr << "Error building line, first entry not addr set" <<
                    endl;
                exit(1);
            }
            Dwarf_Unsigned res = dwarf_lne_set_address(dbg,
                a,elfsymidx,&lerror);
            if((Dwarf_Signed)res == DW_DLV_NOCOUNT) {
                cerr << "Error building line, dwarf_lne_set_address" <<
                    endl;
                exit(1);
            }
            addrsetincu = true;
            firstline = false;
        } else if( endsequence) {
            Dwarf_Unsigned esres = dwarf_lne_end_sequence(dbg,
                a,&lerror);
            if((Dwarf_Signed)esres == DW_DLV_NOCOUNT) {
                cerr << "Error building line, dwarf_lne_end_sequence" <<
                    endl;
                exit(1);
            }
            addrsetincu = false;
            continue;
        }
        Dwarf_Signed linecol = li.getlinecol();
        // It's really the code address or (when in a proper compiler)
        // a section or function offset.
        // libdwarf subtracts the code_offset from the address passed
        // this way or from dwarf_lne_set_address() and writes a small
        // offset in a DW_LNS_advance_pc instruction.
        Dwarf_Addr code_offset = a;
        Dwarf_Unsigned lineno = li.getlineno();
        Dwarf_Bool isstmt = li.getisstmt()?1:0;
        Dwarf_Bool isblock = li.getisblock()?1:0;
        Dwarf_Bool isepiloguebegin = li.getepiloguebegin()?1:0;
        Dwarf_Bool isprologueend = li.getprologueend()?1:0;
        Dwarf_Unsigned isa = li.getisa();
        Dwarf_Unsigned discriminator = li.getdiscriminator();
        Dwarf_Unsigned lires = dwarf_add_line_entry_b(dbg,
            pathindex,
            code_offset,
            lineno,
            linecol,
            isstmt,
            isblock,
            isepiloguebegin,
            isprologueend,
            isa,
            discriminator,
            &lerror);
        if((Dwarf_Signed)lires == DW_DLV_NOCOUNT) {
            cerr << "Error building line, dwarf_add_line_entry" <<
                endl;
            exit(1);
        }
    }
    if(addrsetincu) {
        cerr << "CU Lines did not end in an end_sequence!" << endl;
    }
}
// This emits the DIEs for a single CU and possibly line data
// associated with the CU.
// The DIEs form a graph (which can be created and linked together
// in any order)  and which is emitted in tree preorder as
// defined by the DWARF spec.
//
static void
emitOneCU( Dwarf_P_Debug dbg,IRepresentation & Irep,
    IRCUdata&cu,
    int cu_of_input_we_output UNUSEDARG)
{
    // We descend the the tree, creating DIEs and linking
    // them in as we return back up the tree of recursing
    // on IRDie children.
    Dwarf_Error lerror;

    IRDie & basedie =  cu.baseDie();
    Dwarf_P_Die cudie = HandleOneDieAndChildren(dbg,Irep,
        cu,basedie,basedie,0);

    // Add base die to debug, this is the CU die.
    // This is not a good design as DWARF3/4 have
    // requirements of multiple CUs in a single creation,
    // which cannot be handled yet.
    Dwarf_Unsigned res = dwarf_add_die_to_debug(dbg,cudie,&lerror);
    if(res != DW_DLV_OK)  {
        cerr << "Unable to add_die_to_debug " << endl;
        exit(1);
    }

    // Does fixup of IRFormReference targets.
    cu.updateClassReferenceTargets();

    HandleLineData(dbg,Irep,cu);
}

// .debug_info creation.
// Also creates .debug_line
static void
transform_debug_info(Dwarf_P_Debug dbg,
   IRepresentation & irep,int cu_of_input_we_output)
{
    int cu_number = 0;
    std::list<IRCUdata> &culist = irep.infodata().getCUData();
    // For now,  just one CU we write (as spoken by Yoda).

    for ( list<IRCUdata>::iterator it = culist.begin();
        it != culist.end();
        it++,cu_number++) {
        if(cu_number == cu_of_input_we_output) {
            IRCUdata & primecu = *it;
            emitOneCU(dbg,irep,primecu,cu_of_input_we_output);
            break;
        }
    }
}
static void
transform_cie_fde(Dwarf_P_Debug dbg,
    IRepresentation & Irep,
    int cu_of_input_we_output UNUSEDARG)
{
    Dwarf_Error err = 0;
    std::vector<IRCie> &cie_vec =
        Irep.framedata().get_cie_vec();
    std::vector<IRFde> &fde_vec =
        Irep.framedata().get_fde_vec();

    // cievecsize Signed as dwarf_add_frame_cie
    // returns signed to accomodate its error
    // value return
    Dwarf_Signed cievecsize = cie_vec.size();

    for(Dwarf_Signed i = 0; i < cievecsize ; ++i) {
        IRCie &ciein = cie_vec[i];
        Dwarf_Unsigned version = 0;
        string aug;
        Dwarf_Unsigned code_align = 0;
        Dwarf_Signed data_align = 0;
        Dwarf_Half ret_addr_reg = -1;
        void * bytes = 0;
        Dwarf_Unsigned bytes_len;
        ciein.get_basic_cie_data(&version, &aug,
            &code_align, &data_align, &ret_addr_reg);
        ciein.get_init_instructions(&bytes_len,&bytes);
        // version implied: FIXME, need to let user code set output
        // frame version.
        char *str = const_cast<char *>(aug.c_str());
        Dwarf_Signed out_cie_index =
            dwarf_add_frame_cie(dbg, str,
            code_align, data_align, ret_addr_reg,
            bytes,bytes_len,
            &err);
        if(out_cie_index == DW_DLV_NOCOUNT) {
            cerr << "Error creating cie from input cie " << i << endl;
            exit(1);
        }
        vector<int> fdeindex;
        // This inner loop is C*F so a bit slow.
        for(size_t j = 0; j < fde_vec.size(); ++j) {
            IRFde &fdein = fde_vec[j];
            Dwarf_Unsigned code_len = 0;
            Dwarf_Addr code_virt_addr = 0;
            Dwarf_Signed cie_input_index = 0;

            fdein.get_fde_base_data(&code_virt_addr,
                &code_len, &cie_input_index);
            if(cie_input_index != i) {
                // Wrong cie, ignore this fde right now.
                continue;
            }


            Dwarf_P_Fde fdeout = dwarf_new_fde(dbg,&err);
            if(reinterpret_cast<Dwarf_Addr>(fdeout) == DW_DLV_BADADDR) {
                cerr << "Error creating new fde " << j << endl;
                exit(1);
            }
            Dwarf_Unsigned ilen = 0;
            void *instrs = 0;
            fdein.get_fde_instructions(&ilen, &instrs);

            int res = dwarf_insert_fde_inst_bytes(dbg,
                fdeout, ilen, instrs,&err);
            if(res != DW_DLV_OK) {
                cerr << "Error inserting frame instr block " << j << endl;
                exit(1);
            }

            Dwarf_P_Die irix_die = 0;
            Dwarf_Signed irix_table_offset = 0;
            Dwarf_Unsigned irix_excep_sym = 0;
            Dwarf_Unsigned code_virt_addr_symidx =
                Irep.getBaseTextSymbol();
            Dwarf_Unsigned fde_index = dwarf_add_frame_info(
                dbg, fdeout,irix_die,
                out_cie_index, code_virt_addr,
                code_len,code_virt_addr_symidx,
                irix_table_offset,irix_excep_sym,
                &err);
            if(fde_index == DW_DLV_BADADDR) {
                cerr << "Error creating new fde " << j << endl;
                exit(1);
            }
        }
    }
}

static void
transform_macro_info(Dwarf_P_Debug dbg,
   IRepresentation & Irep,
   int cu_of_input_we_output UNUSEDARG)
{
    IRMacro &macrodata = Irep.macrodata();
    std::vector<IRMacroRecord> &macrov = macrodata.getMacroVec();
    for(size_t m = 0; m < macrov.size() ; m++ ) {
        // FIXME: we need to coordinate with generated
        // CUs .
        cout << "FIXME: macros not really output yet " <<
            m << " " <<
            macrov.size() << endl;
    }
    Dwarf_Unsigned reloc_count = 0;
    int drd_version = 0;
    int res = dwarf_get_relocation_info_count(dbg,&reloc_count,
        &drd_version,&error);
    if( res != DW_DLV_OK) {
        cerr << "Error getting relocation info count." << endl;
        exit(1);

    }
    for( Dwarf_Unsigned ct = 0; ct < reloc_count ; ++ct) {
    }
}

// Starting at a Die, look through its children
// in its input to find which one we have by
// comparing the input-die global offset.
static
Dwarf_P_Die findTargetDieByOffset(IRDie& indie,
    Dwarf_Unsigned targetglobaloff)
{
    Dwarf_Unsigned globoff = indie.getGlobalOffset();
    if(globoff == targetglobaloff) {
        return indie.getGeneratedDie();
    }
    std::list<IRDie> dielist =  indie.getChildren();
    for ( list<IRDie>::iterator it = dielist.begin();
        it != dielist.end();
        it++) {
        IRDie &ldie = *it;
        Dwarf_P_Die foundDie = findTargetDieByOffset(ldie,
            targetglobaloff);
        if(foundDie) {
            return foundDie;
        }
    }
    return NULL;
}

// If the pubnames/pubtypes entry is in the
// cu we are emitting, find the generated output
// Dwarf_P_Die in that CU
// and attach the pubname/type entry to it.
static void
transform_debug_pubnames_types_inner(Dwarf_P_Debug dbg,
    IRepresentation & Irep,
    int cu_of_input_we_output UNUSEDARG,
    IRCUdata&cu)
{
    // First, get the target CU. */
    Dwarf_Unsigned targetcuoff= cu.getCUdieOffset();
    IRDie &basedie = cu.baseDie();
    IRPubsData& pubs = Irep.pubnamedata();
    std::list<IRPub> &nameslist = pubs.getPubnames();

    if(!nameslist.empty()) {
        for ( list<IRPub>::iterator it = nameslist.begin();
        it != nameslist.end();
        it++) {
            IRPub &pub = *it;
            Dwarf_Unsigned pubcuoff= pub.getCUdieOffset();
            Dwarf_Unsigned ourdieoff= pub.getDieOffset();
            if (pubcuoff != targetcuoff) {
                continue;
            }
            Dwarf_P_Die targdie = findTargetDieByOffset(basedie,
                ourdieoff);
            if(targdie) {
                // Ugly. Old mistake in libdwarf declaration.
                char *mystr = const_cast<char *>(pub.getName().c_str());
                Dwarf_Unsigned res = dwarf_add_pubname(
                    dbg,targdie,
                    mystr,
                    &error);
                if(!res) {
                    cerr << "Failed to add pubname entry for offset"
                        << ourdieoff
                        << "in CU at offset " << pubcuoff << endl;
                    exit(1);
                }
            } else {
                cerr << "Did not find target pubname P_Die for offset "
                    << ourdieoff
                    << "in CU at offset " << pubcuoff << endl;
            }
        }
    }
    std::list<IRPub> &typeslist = pubs.getPubtypes();
    if(!typeslist.empty()) {
        for ( list<IRPub>::iterator it = typeslist.begin();
        it != typeslist.end();
        it++) {
            IRPub &pub = *it;
            Dwarf_Unsigned pubcuoff= pub.getCUdieOffset();
            Dwarf_Unsigned ourdieoff= pub.getDieOffset();
            if (pubcuoff != targetcuoff) {
                continue;
            }
            Dwarf_P_Die targdie = findTargetDieByOffset(basedie,
                ourdieoff);
            if(targdie) {
                // Ugly. Old mistake in libdwarf declaration.
                char *mystr = const_cast<char *>(pub.getName().c_str());
                Dwarf_Unsigned res = dwarf_add_pubtype(
                    dbg,targdie,
                    mystr,
                    &error);
                if(!res) {
                    cerr << "Failed to add pubtype entry for offset"
                        << ourdieoff
                        << "in CU at offset " << pubcuoff << endl;
                    exit(1);
                }
            } else {
                cerr << "Did not find target pubtype P_Die for offset "
                    << ourdieoff
                    << "in CU at offset " << pubcuoff << endl;
            }
        }
    }
}

// This looks for pubnames/pubtypes
// to generate based on an object file input.
static void
transform_debug_pubnames_types(Dwarf_P_Debug dbg,
   IRepresentation & Irep,int cu_of_input_we_output)
{
    int cu_number = 0;
    std::list<IRCUdata> &culist = Irep.infodata().getCUData();
    // For now,  just one CU we write (as spoken by Yoda).

    for ( list<IRCUdata>::iterator it = culist.begin();
        it != culist.end();
        it++,cu_number++) {
        if(cu_number == cu_of_input_we_output) {
            IRCUdata & primecu = *it;
            transform_debug_pubnames_types_inner(
                dbg,Irep,cu_of_input_we_output,primecu);
            break;
        }
    }
}

void
transform_irep_to_dbg(Dwarf_P_Debug dbg,
   IRepresentation & Irep,int cu_of_input_we_output)
{
    transform_debug_info(dbg,Irep,cu_of_input_we_output);
    transform_cie_fde(dbg,Irep,cu_of_input_we_output);
    transform_macro_info(dbg,Irep,cu_of_input_we_output);
    transform_debug_pubnames_types(dbg,Irep,cu_of_input_we_output);
}
