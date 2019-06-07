/*
  Copyright (c) 2009-2018 David Anderson.
  All rights reserved.

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
/*  simplereader.c
    This is an example of code reading dwarf .debug_frame.
    It is kept as simple as possible to expose essential features.
    It does not do all possible error reporting or error handling.

    It specifically calls dwarf_expand_frame_instructions()
    to verify that works without crashing!

    To use, try
        make
        ./frame1 frame1

    gcc/clang may produce .eh_frame without .debug_frame.
    To read .eh_frame call dwarf_get_fde_list_eh()
    below instead of dwarf_get_fde_list() .
*/
#include "config.h"
#include <sys/types.h> /* For open() */
#include <sys/stat.h>  /* For open() */
#include <fcntl.h>     /* For open() */
#include <stdlib.h>     /* For exit() */
#ifdef HAVE_UNISTD_H
#include <unistd.h>     /* For close() */
#endif
#include <string.h>     /* For strcmp* */
#include <stdio.h>
#include <errno.h>
#include "dwarf.h"
#include "libdwarf.h"


static void read_frame_data(Dwarf_Debug dbg,const char *sec);
static void print_fde_instrs(Dwarf_Debug dbg, Dwarf_Fde fde,
    Dwarf_Error *error);
static void print_regtable(Dwarf_Regtable3 *tab3);
static void print_cie_instrs(Dwarf_Cie cie,Dwarf_Error *error);
static void print_fde_selected_regs( Dwarf_Fde fde);
static void print_reg(int r);

static int just_print_selected_regs = 0;

/*  Depending on the ABI we set INITIAL_VAL
    differently.  For ia64 initial value is
    UNDEF_VAL, for MIPS and others initial
    value is SAME_VAL.
    Here we'll set it UNDEF_VAL
    as that way we'll see when first set. */
#define UNDEF_VAL 2000
#define SAME_VAL 2001
#define CFA_VAL 2002
#define INITIAL_VAL UNDEF_VAL

/*  Don't really want getopt here. yet.
    so hand do options. */

int
main(int argc, char **argv)
{

    Dwarf_Debug dbg = 0;
    int fd = -1;
    const char *filepath = "<stdin>";
    int res = DW_DLV_ERROR;
    Dwarf_Error error;
    Dwarf_Handler errhand = 0;
    Dwarf_Ptr errarg = 0;
    int regtabrulecount = 0;
    int curopt = 0;

    for(curopt = 1;curopt < argc; ++curopt) {
        if (strncmp(argv[curopt],"--",2)) {
            break;
        }
        if (!strcmp(argv[curopt],"--just-print-selected-regs")) {
            just_print_selected_regs++;
        }
    }

    if(curopt == argc ) {
        fd = 0; /* stdin */
    } else if (curopt == (argc-1)) {
        filepath = argv[curopt];
        fd = open(filepath,O_RDONLY);
    } else {
        printf("Too many args, giving up. \n");
        exit(1);
    }

    if(fd < 0) {
        printf("Failure attempting to open %s\n",filepath);
    }
    res = dwarf_init(fd,DW_DLC_READ,errhand,errarg, &dbg,&error);
    if(res != DW_DLV_OK) {
        printf("Giving up, dwarf_init failed, cannot do DWARF processing\n");
        exit(1);
    }
    /*  Do this setting after init before any real operations.
        These return the old values, but here we do not
        need to know the old values.  The sizes and
        values here are higher than most ABIs and entirely
        arbitrary.

        The setting of initial_value to
        the same as undefined-value (the other possible choice being
        same-value) is arbitrary, different ABIs do differ, and
        you have to know which is right.

        In dwarfdump we get the SAME_VAL, UNDEF_VAL,
        INITIAL_VAL CFA_VAL from dwconf_s struct.   */
    regtabrulecount=1999;
    dwarf_set_frame_undefined_value(dbg, UNDEF_VAL);
    dwarf_set_frame_rule_initial_value(dbg, INITIAL_VAL);
    dwarf_set_frame_same_value(dbg,SAME_VAL);
    dwarf_set_frame_cfa_value(dbg,CFA_VAL);
    dwarf_set_frame_rule_table_size(dbg,regtabrulecount);

    read_frame_data(dbg,".debug_frame");
    read_frame_data(dbg,".eh_frame");
    res = dwarf_finish(dbg,&error);
    if(res != DW_DLV_OK) {
        printf("dwarf_finish failed!\n");
    }
    close(fd);
    return 0;
}

static void
read_frame_data(Dwarf_Debug dbg,const char *sect)
{
    Dwarf_Error error;
    Dwarf_Signed cie_element_count = 0;
    Dwarf_Signed fde_element_count = 0;
    Dwarf_Cie *cie_data = 0;
    Dwarf_Fde *fde_data = 0;
    int res = DW_DLV_ERROR;
    Dwarf_Signed fdenum = 0;


    /*  If you wish to read .eh_frame data, use
        dwarf_get_fde_list_eh() instead.
        Get debug_frame with dwarf_get_fde_list. */
    printf(" Print %s\n",sect);
    if (!strcmp(sect,".eh_frame"))
    {
        res = dwarf_get_fde_list_eh(dbg,&cie_data,&cie_element_count,
            &fde_data,&fde_element_count,&error);
    } else {
        res = dwarf_get_fde_list(dbg,&cie_data,&cie_element_count,
            &fde_data,&fde_element_count,&error);
    }
    if(res == DW_DLV_NO_ENTRY) {
        printf("No %s data present\n",sect);
        return;
    }
    if( res == DW_DLV_ERROR) {
        printf("Error reading frame data ");
        exit(1);
    }
    printf( "%" DW_PR_DSd " cies present. "
        "%" DW_PR_DSd " fdes present. \n",
        cie_element_count,fde_element_count);
    /*if(fdenum >= fde_element_count) {
        printf("Want fde %d but only %" DW_PR_DSd " present\n",fdenum,
            fde_element_count);
        exit(1);
    }*/

    for(fdenum = 0; fdenum < fde_element_count; ++fdenum) {
        Dwarf_Cie cie = 0;

        res = dwarf_get_cie_of_fde(fde_data[fdenum],&cie,&error);
        if(res != DW_DLV_OK) {
            printf("Error accessing cie of fdenum %" DW_PR_DSd
                " to get its cie\n",fdenum);
            exit(1);
        }
        printf("Print cie of fde %" DW_PR_DSd  "\n",fdenum);
        print_cie_instrs(cie,&error);
        printf("Print fde %" DW_PR_DSd  "\n",fdenum);
        if (just_print_selected_regs) {
            print_fde_selected_regs(fde_data[fdenum]);
        } else {
            print_fde_instrs(dbg,fde_data[fdenum],&error);
        }
    }

    /* Done with the data. */
    dwarf_fde_cie_list_dealloc(dbg,cie_data,cie_element_count,
        fde_data, fde_element_count);
    return;
}




/* Simply shows the instructions at hand for this fde. */
static void
print_cie_instrs(Dwarf_Cie cie,Dwarf_Error *error)
{
    int res = DW_DLV_ERROR;
    Dwarf_Unsigned bytes_in_cie = 0;
    Dwarf_Small version = 0;
    char *augmentation = 0;
    Dwarf_Unsigned code_alignment_factor = 0;
    Dwarf_Signed data_alignment_factor = 0;
    Dwarf_Half return_address_register_rule = 0;
    Dwarf_Ptr instrp = 0;
    Dwarf_Unsigned instr_len = 0;

    res = dwarf_get_cie_info(cie,&bytes_in_cie,
        &version, &augmentation, &code_alignment_factor,
        &data_alignment_factor, &return_address_register_rule,
        &instrp,&instr_len,error);
    if(res != DW_DLV_OK) {
        printf("Unable to get cie info!\n");
        exit(1);
    }
}


/*  This was written with DWARF2 Dwarf_Frame_Op!
    FIXME need dwarf_expand_frame_instructions_b() */
static void
print_frame_instrs(Dwarf_Frame_Op *frame_op_array,
  Dwarf_Signed frame_op_count)
{
    Dwarf_Signed i = 0;
    printf("Base op. Ext op. Reg. Offset. Instr-offset.\n");
    for(i = 0; i < frame_op_count; ++i) {
        Dwarf_Frame_Op *fo = frame_op_array +i;

        printf("[%2" DW_PR_DSd "]", i);
        printf(" %2d. ", fo->fp_base_op);
        printf(" 0x%02x. ", fo->fp_extended_op);
        printf(" %3d. ", fo->fp_register);

        /*  DWARF3 and later:
            for DW_CFA_def_cfa_expression,
            DW_CFA_val_expression, and
            DW_CFA_expression the fp_offset is a pointer
            into runtime memory. Printing it is not helpful
            in a diff.  */
        if (fo->fp_extended_op == DW_CFA_def_cfa ||
            fo->fp_extended_op == DW_CFA_val_expression ||
            fo->fp_extended_op == DW_CFA_def_cfa_expression ) {
            /* The offset is really a pointer. */
            printf(" (pointer-value) ");
        } else {
            /* The offset is a real offset /value of some sort. */
            printf(" %6" DW_PR_DSd ". ", fo->fp_offset);
        }
        printf(" 0x%08" DW_PR_DUx ". ", fo->fp_instr_offset);
        printf("\n");
    }
}



static void
print_fde_col(Dwarf_Signed k,
    Dwarf_Addr   jsave,
    Dwarf_Small  value_type,
    Dwarf_Signed offset_relevant,
    Dwarf_Signed reg_used,
    Dwarf_Signed offset_or_block_len,
    Dwarf_Ptr    block_ptr,
    Dwarf_Addr   row_pc,
    Dwarf_Bool   has_more_rows,
    Dwarf_Addr   subsequent_pc)
{
    char *type_title = "";
    Dwarf_Unsigned rule_id = k;

    printf(" pc=0x%" DW_PR_DUx ,jsave);
    if (row_pc != jsave) {
        printf(" row_pc=0x%" DW_PR_DUx ,row_pc);
    }
    printf(" col=%" DW_PR_DSd " ",k);
    switch(value_type) {
    case DW_EXPR_OFFSET:
        type_title = "off";
        goto preg2;
    case DW_EXPR_VAL_OFFSET:
        type_title = "valoff";

        preg2:
        printf("<%s ", type_title);
        if (reg_used == SAME_VAL) {
            printf(" SAME_VAL");
            break;
        } else if (reg_used == INITIAL_VAL) {
            printf(" INITIAL_VAL");
            break;
        }
        print_reg(rule_id);

        printf("=");
        if (offset_relevant == 0) {
            print_reg(reg_used);
            printf(" ");
        } else {
            printf("%02" DW_PR_DSd , offset_or_block_len);
            printf("(");
            print_reg(reg_used);
            printf(") ");
        }
        break;
    case DW_EXPR_EXPRESSION:
        type_title = "expr";
        goto pexp2;
    case DW_EXPR_VAL_EXPRESSION:
        type_title = "valexpr";

        pexp2:
        printf("<%s ", type_title);
        print_reg(rule_id);
        printf("=");
        printf("expr-block-len=%" DW_PR_DSd , offset_or_block_len);
        printf(" block-ptr=%" DW_PR_DUx , (Dwarf_Unsigned)block_ptr);
#if 0
        {
            char pref[40];

            strcpy(pref, "<");
            strcat(pref, type_title);
            strcat(pref, "bytes:");
            /*  The data being dumped comes direct from
                libdwarf so libdwarf validated it. */
            dump_block(pref, block_ptr, offset_or_block_len);
            printf("%s", "> ");
            if (glflags.verbose) {
                struct esb_s exprstring;
                esb_constructor(&exprstring);
                get_string_from_locs(dbg,
                    block_ptr,offset,addr_size,
                    offset_size,version,&exprstring);
                printf("<expr:%s>",esb_get_string(&exprstring));
                esb_destructor(&exprstring);
            }
        }
#endif
        break;
    default:
        printf("Internal error in libdwarf, value type %d\n",
            value_type);
        exit(1);
    }
    printf(" more=%d",has_more_rows);
    printf(" next=0x%" DW_PR_DUx,subsequent_pc);
    printf("%s", "> ");
    printf("\n");
}


/*  In dwarfdump we use
    dwarf_get_fde_info_for_cfa_reg3_b() to get subsequent pc
    and avoid incrementing pc by for the next cfa,
    using has_more_rows and subsequent_pc passed back.

    Here, to verify function added in May 2018,
    we instead use dwarf_get_fde_info_for_reg3_b()
    which has the has_more_rows and subsequent_pc functions
    for the case where one is tracking a particular register
    and not closely watching the CFA value itself. */

static void
print_fde_selected_regs( Dwarf_Fde fde)
{
    Dwarf_Error oneferr = 0;
    /* Arbitrary column numbers for testing. */
    static int selected_cols[] = {1,3,5};
    static int selected_cols_count =
        sizeof(selected_cols)/sizeof(selected_cols[0]);
    Dwarf_Signed k = 0;
    int fres = 0;

    Dwarf_Addr low_pc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Ptr fde_bytes = NULL;
    Dwarf_Unsigned fde_bytes_length = 0;
    Dwarf_Off cie_offset = 0;
    Dwarf_Signed cie_index = 0;
    Dwarf_Off fde_offset = 0;
    Dwarf_Fde curfde = fde;
    Dwarf_Cie cie = 0;
    Dwarf_Addr jsave = 0;
    Dwarf_Addr high_addr = 0;
    Dwarf_Addr next_jsave = 0;
    Dwarf_Bool has_more_rows = 0;
    Dwarf_Addr subsequent_pc = 0;
    Dwarf_Error error = 0;
    int res = 0;

    fres = dwarf_get_fde_range(curfde,
        &low_pc, &func_length,
        &fde_bytes,
        &fde_bytes_length,
        &cie_offset, &cie_index,
        &fde_offset, &oneferr);

    if (fres == DW_DLV_ERROR) {
        printf("FAIL: dwarf_get_fde_range err %" DW_PR_DUu " line %d\n",
            dwarf_errno(oneferr),__LINE__);
        exit(1);
    }
    if (fres == DW_DLV_NO_ENTRY) {
        printf("No fde range data available\n");
        return;
    }
    res = dwarf_get_cie_of_fde(fde,&cie,&error);
    if(res != DW_DLV_OK) {
        printf("Error getting cie from fde\n");
        exit(1);
    }

    high_addr = low_pc + func_length;
    /*  Could check has_more_rows here instead of high_addr,
        If we initialized has_more_rows to 1 above. */
    for(jsave = low_pc ; next_jsave < high_addr; jsave = next_jsave) {
        next_jsave = jsave+1;
        printf("\n");
        for(k = 0; k < selected_cols_count ; ++k ) {
            Dwarf_Signed reg = 0;
            Dwarf_Signed offset_relevant = 0;
            int fires = 0;
            Dwarf_Small value_type = 0;
            Dwarf_Ptr block_ptr = 0;
            Dwarf_Signed offset_or_block_len = 0;
            Dwarf_Addr row_pc = 0;

            fires = dwarf_get_fde_info_for_reg3_b(curfde,
                selected_cols[k],
                jsave,
                &value_type,
                &offset_relevant,
                &reg,
                &offset_or_block_len,
                &block_ptr,
                &row_pc,
                &has_more_rows,
                &subsequent_pc,
                &oneferr);
            if (fires == DW_DLV_ERROR) {
                printf("FAIL: reading reg err %" DW_PR_DUu " line %d",
                    dwarf_errno(oneferr),__LINE__);
                exit(1);
            }
            if (fires == DW_DLV_NO_ENTRY) {
                continue;
            }
            print_fde_col(
                selected_cols[k],jsave,
                value_type,offset_relevant,
                reg,offset_or_block_len,block_ptr,row_pc,
                has_more_rows, subsequent_pc);
            if (has_more_rows) {
                next_jsave = subsequent_pc;
            } else {
                next_jsave = high_addr;
            }
        }
    }
}


/* Just prints the instructions in the fde. */
static void
print_fde_instrs(Dwarf_Debug dbg,
    Dwarf_Fde fde, Dwarf_Error *error)
{
    int res;
    Dwarf_Addr lowpc = 0;
    Dwarf_Unsigned func_length = 0;
    Dwarf_Ptr fde_bytes;
    Dwarf_Unsigned fde_byte_length = 0;
    Dwarf_Off cie_offset = 0;
    Dwarf_Signed cie_index = 0;
    Dwarf_Off fde_offset = 0;
    Dwarf_Addr arbitrary_addr = 0;
    Dwarf_Addr actual_pc = 0;
    Dwarf_Regtable3 tab3;
    int oldrulecount = 0;
    Dwarf_Ptr outinstrs = 0;
    Dwarf_Unsigned instrslen = 0;
    Dwarf_Frame_Op * frame_op_array = 0;
    Dwarf_Signed frame_op_count = 0;
    Dwarf_Cie cie = 0;



    res = dwarf_get_fde_range(fde,&lowpc,&func_length,&fde_bytes,
        &fde_byte_length,&cie_offset,&cie_index,&fde_offset,error);
    if(res != DW_DLV_OK) {
        printf("Problem getting fde range \n");
        exit(1);
    }

    arbitrary_addr = lowpc + (func_length/2);
    printf("function low pc 0x%" DW_PR_DUx
        "  and length 0x%" DW_PR_DUx
        "  and midpoint addr we choose 0x%" DW_PR_DUx
        "\n",
        lowpc,func_length,arbitrary_addr);

    /*  1 is arbitrary. We are winding up getting the
        rule count here while leaving things unchanged. */
    oldrulecount = dwarf_set_frame_rule_table_size(dbg,1);
    dwarf_set_frame_rule_table_size(dbg,oldrulecount);

    tab3.rt3_reg_table_size = oldrulecount;
    tab3.rt3_rules = (struct Dwarf_Regtable_Entry3_s *) malloc(
        sizeof(struct Dwarf_Regtable_Entry3_s)* oldrulecount);
    if (!tab3.rt3_rules) {
        printf("Unable to malloc for %d rules\n",oldrulecount);
        exit(1);
    }

    res = dwarf_get_fde_info_for_all_regs3(fde,arbitrary_addr ,
        &tab3,&actual_pc,error);
    printf("function actual addr of row 0x%" DW_PR_DUx "\n",
        actual_pc);

    if(res != DW_DLV_OK) {
        printf("dwarf_get_fde_info_for_all_regs3 failed!\n");
        exit(1);
    }
    print_regtable(&tab3);

    res = dwarf_get_fde_instr_bytes(fde,&outinstrs,&instrslen,error);
    if(res != DW_DLV_OK) {
        printf("dwarf_get_fde_instr_bytes failed!\n");
        exit(1);
    }
    res = dwarf_get_cie_of_fde(fde,&cie,error);
    if(res != DW_DLV_OK) {
        printf("Error getting cie from fde\n");
        exit(1);
    }

    res = dwarf_expand_frame_instructions(cie,
        outinstrs,instrslen,&frame_op_array,
        &frame_op_count,error);
    if(res != DW_DLV_OK) {
        printf("dwarf_expand_frame_instructions failed!\n");
        exit(1);
    }
    printf("Frame op count: %" DW_PR_DUu "\n",frame_op_count);
    print_frame_instrs(frame_op_array,frame_op_count);

    dwarf_dealloc(dbg,frame_op_array, DW_DLA_FRAME_BLOCK);
    free(tab3.rt3_rules);
}

static void
print_reg(int r)
{
   switch(r) {
   case SAME_VAL:
        printf(" %d SAME_VAL ",r);
        break;
   case UNDEF_VAL:
        printf(" %d UNDEF_VAL ",r);
        break;
   case CFA_VAL:
        printf(" %d (CFA) ",r);
        break;
   default:
        printf(" r%d ",r);
        break;
   }
}

static void
print_one_regentry(const char *prefix,
    struct Dwarf_Regtable_Entry3_s *entry)
{
    int is_cfa = !strcmp("cfa",prefix);
    printf("%s ",prefix);
    printf("type: %d %s ",
        entry->dw_value_type,
        (entry->dw_value_type == DW_EXPR_OFFSET)? "DW_EXPR_OFFSET":
        (entry->dw_value_type == DW_EXPR_VAL_OFFSET)? "DW_EXPR_VAL_OFFSET":
        (entry->dw_value_type == DW_EXPR_EXPRESSION)? "DW_EXPR_EXPRESSION":
        (entry->dw_value_type == DW_EXPR_VAL_EXPRESSION)?
            "DW_EXPR_VAL_EXPRESSION":
            "Unknown");
    switch(entry->dw_value_type) {
    case DW_EXPR_OFFSET:
        print_reg(entry->dw_regnum);
        printf(" offset_rel? %d ",entry->dw_offset_relevant);
        if(entry->dw_offset_relevant) {
            printf(" offset  %" DW_PR_DSd " " ,
                entry->dw_offset_or_block_len);
            if(is_cfa) {
                printf("defines cfa value");
            } else {
                printf("address of value is CFA plus signed offset");
            }
            if(!is_cfa  && entry->dw_regnum != CFA_VAL) {
                printf(" compiler botch, regnum != CFA_VAL");
            }
        } else {
            printf("value in register");
        }
        break;
    case DW_EXPR_VAL_OFFSET:
        print_reg(entry->dw_regnum);
        printf(" offset  %" DW_PR_DSd " " ,
            entry->dw_offset_or_block_len);
        if(is_cfa) {
            printf("does this make sense? No?");
        } else {
            printf("value at CFA plus signed offset");
        }
        if(!is_cfa  && entry->dw_regnum != CFA_VAL) {
            printf(" compiler botch, regnum != CFA_VAL");
        }
        break;
    case DW_EXPR_EXPRESSION:
        print_reg(entry->dw_regnum);
        printf(" offset_rel? %d ",entry->dw_offset_relevant);
        printf(" offset  %" DW_PR_DSd " " ,
            entry->dw_offset_or_block_len);
        printf("Block ptr set? %s ",entry->dw_block_ptr?"yes":"no");
        printf(" Value is at address given by expr val ");
        /* printf(" block-ptr  0x%" DW_PR_DUx " ",
            (Dwarf_Unsigned)entry->dw_block_ptr); */
        break;
    case DW_EXPR_VAL_EXPRESSION:
        printf(" expression byte len  %" DW_PR_DSd " " ,
            entry->dw_offset_or_block_len);
        printf("Block ptr set? %s ",entry->dw_block_ptr?"yes":"no");
        printf(" Value is expr val ");
        if(!entry->dw_block_ptr) {
            printf("Compiler botch. ");
        }
        /* printf(" block-ptr  0x%" DW_PR_DUx " ",
            (Dwarf_Unsigned)entry->dw_block_ptr); */
        break;
    }
    printf("\n");
}

static void
print_regtable(Dwarf_Regtable3 *tab3)
{
    int r;
    /* We won't print too much. A bit arbitrary. */
    int max = 10;
    if(max > tab3->rt3_reg_table_size) {
        max = tab3->rt3_reg_table_size;
    }
    print_one_regentry("cfa",&tab3->rt3_cfa_rule);

    for(r = 0; r < max; r++) {
        char rn[30];
        snprintf(rn,sizeof(rn),"reg %d",r);
        print_one_regentry(rn,tab3->rt3_rules+r);
    }


}
