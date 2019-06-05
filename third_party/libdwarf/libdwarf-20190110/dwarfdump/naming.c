/*
   Copyright (C) 2000-2005 Silicon Graphics, Inc. All Rights Reserved.
   Portions Copyright (C) 2007-2012 David Anderson. All Rights Reserved.
   Portions Copyright (C) 2010-2012 SN Systems Ltd. All Rights Reserved.

   This program is free software; you can redistribute it and/or modify it
   under the terms of version 2 of the GNU General Public License as
   published by the Free Software Foundation.

   This program is distributed in the hope that it would be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

   Further, this software is distributed without any warranty that it is
   free of the rightful claim of any third person regarding infringement
   or the like.  Any license provided herein, whether implied or
   otherwise, applies only to this software file.  Patent licenses, if
   any, provided herein do not apply to combinations of this program with
   other software, or any other product whatsoever.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write the Free Software Foundation, Inc., 51
   Franklin Street - Fifth Floor, Boston MA 02110-1301, USA.

*/

/*  The address of the Free Software Foundation is
    Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
    Boston, MA 02110-1301, USA.
    SGI has moved from the Crittenden Lane address.
*/



/* naming.c */
#include "globals.h"
#include "dwarf.h"
#include "libdwarf.h"
#include "makename.h"
#include "naming.h"
#include "esb.h"

#ifndef TRIVIAL_NAMING
static const char *
skipunder(const char *v)
{
    const char *cp = v;
    int undercount = 0;
    for (; *cp ; ++cp) {
        if (*cp == '_') {
            ++undercount;
            if (undercount == 2) {
                return cp+1;
            }
        }
    }
    return "";
}
#endif /*  TRIVIAL_NAMING */

static const char *
ellipname(int res, int val_in, const char *v,const char *ty,int printonerr)
{
#ifndef TRIVIAL_NAMING
    if (glflags.gf_check_dwarf_constants && checking_this_compiler()) {
        DWARF_CHECK_COUNT(dwarf_constants_result,1);
    }
#endif
    if (res != DW_DLV_OK) {
        char buf[100];
        char *n;
#ifdef ORIGINAL_SPRINTF
        snprintf(buf,sizeof(buf),"<Unknown %s value 0x%x>",ty,val_in);
#else
        struct esb_s eb;

        esb_constructor_fixed(&eb,buf,sizeof(buf));
        esb_append_printf_s(&eb,
            "<Unknown %s",ty);
        esb_append_printf_u(&eb,
            " value 0x%x>",val_in);
#endif
        /* Capture any name error in DWARF constants */
#ifndef TRIVIAL_NAMING
        if (printonerr && glflags.gf_check_dwarf_constants &&
            checking_this_compiler()) {
            if (glflags.gf_check_verbose_mode) {
                fprintf(stderr,"%s of %d (0x%x) is unknown to dwarfdump. "
                    "Continuing. \n",ty,val_in,val_in );
            }
            DWARF_ERROR_COUNT(dwarf_constants_result,1);
            DWARF_CHECK_ERROR_PRINT_CU();
        }
#else
        /* This is for the tree-generation, not dwarfdump itself. */
        if (printonerr) {
            fprintf(stderr,"%s of %d (0x%x) is unknown to dwarfdump. "
                "Continuing. \n",ty,val_in,val_in );
        }
#endif

#ifdef ORIGINAL_SPRINTF
        n = makename(buf);
#else
        n = makename(esb_get_string(&eb));
        esb_destructor(&eb);
#endif
        return n;
    }
#ifndef TRIVIAL_NAMING
    if (glflags.ellipsis) {
        return skipunder(v);
    }
#endif
    return v;
}

const char * get_TAG_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_TAG_name(val_in,&v);
   return ellipname(res,val_in,v,"TAG",printonerr);
}
const char * get_children_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_children_name(val_in,&v);
   return ellipname(res,val_in,v,"children",printonerr);
}
const char * get_FORM_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_FORM_name(val_in,&v);
   return ellipname(res,val_in,v,"FORM",printonerr);
}
const char * get_AT_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_AT_name(val_in,&v);
   return ellipname(res,val_in,v,"AT",printonerr);
}
const char * get_OP_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_OP_name(val_in,&v);
   return ellipname(res,val_in,v,"OP",printonerr);
}
const char * get_ATE_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ATE_name(val_in,&v);
   return ellipname(res,val_in,v,"ATE",printonerr);
}
const char * get_DS_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_DS_name(val_in,&v);
   return ellipname(res,val_in,v,"DS",printonerr);
}
const char * get_END_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_END_name(val_in,&v);
   return ellipname(res,val_in,v,"END",printonerr);
}
const char * get_ATCF_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ATCF_name(val_in,&v);
   return ellipname(res,val_in,v,"ATCF",printonerr);
}
const char * get_ACCESS_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ACCESS_name(val_in,&v);
   return ellipname(res,val_in,v,"ACCESS",printonerr);
}
const char * get_VIS_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_VIS_name(val_in,&v);
   return ellipname(res,val_in,v,"VIS",printonerr);
}
const char * get_VIRTUALITY_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_VIRTUALITY_name(val_in,&v);
   return ellipname(res,val_in,v,"VIRTUALITY",printonerr);
}
const char * get_LANG_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_LANG_name(val_in,&v);
   return ellipname(res,val_in,v,"LANG",printonerr);
}
const char * get_ID_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ID_name(val_in,&v);
   return ellipname(res,val_in,v,"ID",printonerr);
}
const char * get_CC_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_CC_name(val_in,&v);
   return ellipname(res,val_in,v,"CC",printonerr);
}
const char * get_INL_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_INL_name(val_in,&v);
   return ellipname(res,val_in,v,"INL",printonerr);
}
const char * get_ORD_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ORD_name(val_in,&v);
   return ellipname(res,val_in,v,"ORD",printonerr);
}
const char * get_DSC_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_DSC_name(val_in,&v);
   return ellipname(res,val_in,v,"DSC",printonerr);
}
const char * get_LNS_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_LNS_name(val_in,&v);
   return ellipname(res,val_in,v,"LNS",printonerr);
}
const char * get_LNE_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_LNE_name(val_in,&v);
   return ellipname(res,val_in,v,"LNE",printonerr);
}
const char * get_MACINFO_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_MACINFO_name(val_in,&v);
   return ellipname(res,val_in,v,"MACINFO",printonerr);
}
const char * get_MACRO_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_MACRO_name(val_in,&v);
   return ellipname(res,val_in,v,"MACRO",printonerr);
}
const char * get_CFA_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_CFA_name(val_in,&v);
   return ellipname(res,val_in,v,"CFA",printonerr);
}
const char * get_EH_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_EH_name(val_in,&v);
   return ellipname(res,val_in,v,"EH",printonerr);
}
const char * get_FRAME_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_FRAME_name(val_in,&v);
   return ellipname(res,val_in,v,"FRAME",printonerr);
}
const char * get_CHILDREN_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_CHILDREN_name(val_in,&v);
   return ellipname(res,val_in,v,"CHILDREN",printonerr);
}
const char * get_ADDR_name(unsigned int val_in,int printonerr)
{
   const char *v = 0;
   int res = dwarf_get_ADDR_name(val_in,&v);
   return ellipname(res,val_in,v,"ADDR",printonerr);
}
