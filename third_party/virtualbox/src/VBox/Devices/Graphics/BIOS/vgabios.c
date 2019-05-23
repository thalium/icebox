// ============================================================================================
/*
 * vgabios.c
 */
// ============================================================================================
//
//  Copyright (C) 2001,2002 the LGPL VGABios developers Team
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// ============================================================================================
//
//  This VGA Bios is specific to the plex86/bochs Emulated VGA card.
//  You can NOT drive any physical vga card with it.
//
// ============================================================================================
//
//  This file contains code ripped from :
//   - rombios.c of plex86
//
//  This VGA Bios contains fonts from :
//   - fntcol16.zip (c) by Joseph Gil avalable at :
//      ftp://ftp.simtel.net/pub/simtelnet/msdos/screen/fntcol16.zip
//     These fonts are public domain
//
//  This VGA Bios is based on information taken from :
//   - Kevin Lawton's vga card emulation for bochs/plex86
//   - Ralf Brown's interrupts list available at http://www.cs.cmu.edu/afs/cs/user/ralf/pub/WWW/files.html
//   - Finn Thogersons' VGADOC4b available at http://home.worldonline.dk/~finth/
//   - Michael Abrash's Graphics Programming Black Book
//   - Francois Gervais' book "programmation des cartes graphiques cga-ega-vga" edited by sybex
//   - DOSEMU 1.0.1 source code for several tables values and formulas
//
// Thanks for patches, comments and ideas to :
//   - techt@pikeonline.net
//
// ============================================================================================


/*
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Oracle elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#include <inttypes.h>
#include "vgabios.h"

#ifdef VBE
#include "vbe.h"
#endif

#include "inlines.h"

/* Declares */
extern void vgabios_int10_handler(void);
#pragma aux vgabios_int10_handler "*";

// Output
void __cdecl          unimplemented(void);
void __cdecl          unknown(void);

static uint8_t find_vga_entry();

#ifdef VBE
extern uint16_t __cdecl vbe_has_vbe_display(void);
extern void             vbe_init(void);
#endif

void set_int_vector(uint8_t int_vec, void *offset)
{
    void __far * __far *ivt = 0;

    ivt[int_vec] = 0xC000 :> offset;
}

//@todo!!
#if 0

vgabios_name:
#ifdef VBOX
.ascii  "VirtualBox VGA BIOS"
#else
.ascii  "Plex86/Bochs VGABios"
#endif
.ascii  " "
.byte   0x00

#ifndef VBOX
vgabios_version:
#ifndef VGABIOS_VERS
.ascii  "current-cvs"
#else
.ascii VGABIOS_VERS
#endif
.ascii  " "

vgabios_date:
.ascii  VGABIOS_DATE
.byte   0x0a,0x0d
.byte   0x00
#endif

#ifndef VBOX
char vgabios_copyright[] = "(C) 2003 the LGPL VGABios developers Team\r\n";
char vgabios_license[]   = "This VGA/VBE Bios is released under the GNU LGPL\r\n\r\n";
char vgabios_website[]   = "Please visit :\r\n" \
                           " . http://www.plex86.org\r\n" \
                           " . http://bochs.sourceforge.net\r\n" \
                           " . http://www.nongnu.org/vgabios\r\n\r\n"
#endif

#endif

extern void set_mode(int mode);
#pragma aux set_mode =  \
    "xor    ah, ah"     \
    "int    10h"        \
    parm [ax];

char msg_vga_init[] = "Oracle VM VirtualBox Version " VBOX_VERSION_STRING " VGA BIOS\r\n";

/*
 * Boot time harware inits
 */
void init_vga_card(void)
{
    /* Switch to color mode and enable CPU access 480 lines. */
    outb(0x3C2, 0xC3);
    /* More than 64k 3C4/04. */
    /// @todo 16-bit write
    outb(0x3C4, 0x04);
    outb(0x3C5, 0x02);

#ifdef DEBUG_VGA
    printf(msg_vga_init);
#endif
}

#include "vgatables.h"
#include "vgadefs.h"

// --------------------------------------------------------------------------------------------
/*
 *  Boot time bios area inits
 */
void init_bios_area(void)
{
    uint8_t __far   *bda;

    bda = 0x40 :> 0;

    /* Indicate 80x25 color was detected. */
    bda[BIOSMEM_INITIAL_MODE] = (bda[BIOSMEM_INITIAL_MODE] & 0xcf) | 0x20;
    /* Just for the first int10 find its children. */

    /* The default char height. */
    bda[BIOSMEM_CHAR_HEIGHT] = 16;
    /* Clear the screen. */
    bda[BIOSMEM_VIDEO_CTL]   = 0x60;
    /* Set the basic screen we have. */
    bda[BIOSMEM_SWITCHES]    = 0xf9;
    /* Set the basic mode set options. */
    bda[BIOSMEM_MODESET_CTL] = 0x51;
    /* Set the default MSR. */
    bda[BIOSMEM_CURRENT_MSR] = 0x09;
}

struct dcc {
    uint8_t     n_ent;
    uint8_t     version;
    uint8_t     max_code;
    uint8_t     reserved;
    uint16_t    dccs[16];
} dcc_table = {
    16,
    1,
    7,
    0
};

struct ssa {
    uint16_t    size;
    void __far  *dcc;
    void __far  *sacs;
    void __far  *pal;
    void __far  *resvd[3];

} secondary_save_area = {
    sizeof(struct ssa),
    &dcc_table
};

void __far *video_save_pointer_table[7] = {
    &video_param_table,
    0,
    0,
    0,
    &secondary_save_area
};

// ============================================================================================
//
// Init Entry point
//
// ============================================================================================
void __far __cdecl vgabios_init_func(void)
{
    init_vga_card();
    init_bios_area();
#ifdef VBE
    vbe_init();
#endif
    set_int_vector(0x10, vgabios_int10_handler);
#ifdef CIRRUS
    cirrus_init();
#endif

#ifndef VBOX
    display_splash_screen();

    // init video mode and clear the screen
    // @@AS: Do not remove this init, because it will break VESA graphics
    set_mode(3);

    display_info();

#ifdef VBE
    vbe_display_info();
#endif

#ifdef CIRRUS
    cirrus_display_info();
#endif

#else /* VBOX */

//#ifdef DEBUG_bird
    /* Init video mode and clear the screen */
    set_mode(3);
//#endif
#endif /* VBOX */
}

#include "vgafonts.h"

#ifndef VBOX
// --------------------------------------------------------------------------------------------
/*
 *  Boot time Splash screen
 */
static void display_splash_screen()
{
}

// --------------------------------------------------------------------------------------------
/*
 *  Tell who we are
 */

static void display_string(void)
{
 // Get length of string
ASM_START
 mov ax,ds
 mov es,ax
 mov di,si
 xor cx,cx
 not cx
 xor al,al
 cld
 repne
  scasb
 not cx
 dec cx
 push cx

 mov ax,#0x0300
 mov bx,#0x0000
 int #0x10

 pop cx
 mov ax,#0x1301
 mov bx,#0x000b
 mov bp,si
 int #0x10
ASM_END
}

static void display_info(void)
{
    display_string(vgabios_name);
    display_string(vgabios_version);
    display_string(vgabios_copyright);
    display_string(vgabios_license);
    display_string(vgabios_website);
}

#endif

// --------------------------------------------------------------------------------------------
#ifdef VGA_DEBUG
void __cdecl int10_debugmsg(uint16_t DI, uint16_t SI, uint16_t BP, uint16_t SP, uint16_t BX,
                            uint16_t DX, uint16_t CX, uint16_t AX, uint16_t DS, uint16_t ES, uint16_t FLAGS)
{
    /* Function 0Eh is write char and would generate way too much output. */
    if (GET_AH() != 0x0E)
        printf("vgabios call ah%02x al%02x bx%04x cx%04x dx%04x\n", GET_AH(), GET_AL(), BX, CX, DX);
}
#endif

static void vga_get_cursor_pos(uint8_t page, uint16_t STACK_BASED *scans, uint16_t STACK_BASED *loc)
{
    if (page > 7) {
        *scans = 0;
        *loc   = 0;
    } else {
        // FIXME should handle VGA 14/16 lines
        *scans = read_word(BIOSMEM_SEG,BIOSMEM_CURSOR_TYPE);
        *loc   = read_word(BIOSMEM_SEG,BIOSMEM_CURSOR_POS + page * 2);
    }
}


static void vga_read_char_attr(uint8_t page, uint16_t STACK_BASED *chr_atr)
{
    uint8_t     xcurs, ycurs, mode, line;
    uint16_t    nbcols, nbrows, address;
    uint16_t    cursor, dummy;

    // Get the mode
    mode = read_byte(BIOSMEM_SEG, BIOSMEM_CURRENT_MODE);
    line = find_vga_entry(mode);
    if (line == 0xFF)
        return;

    // Get the cursor pos for the page
    vga_get_cursor_pos(page, &dummy, &cursor);
    xcurs = cursor & 0x00ff;
    ycurs = (cursor & 0xff00) >> 8;

    // Get the dimensions
    nbrows = read_byte(BIOSMEM_SEG, BIOSMEM_NB_ROWS) + 1;
    nbcols = read_word(BIOSMEM_SEG, BIOSMEM_NB_COLS);

    if (vga_modes[line].class == TEXT) {
        // Compute the address
        address  = SCREEN_MEM_START(nbcols, nbrows, page) + (xcurs + ycurs * nbcols) * 2;
        *chr_atr = read_word(vga_modes[line].sstart, address);
    } else {
        /// @todo graphics modes (not so easy - or useful!)
#ifdef VGA_DEBUG
        unimplemented();
#endif
    }
}

static void vga_get_font_info (uint16_t func, uint16_t STACK_BASED *u_seg, uint16_t STACK_BASED *u_ofs,
                               uint16_t STACK_BASED *c_height, uint16_t STACK_BASED *max_row)
{
    void    __far   *ptr;

    switch (func) {
    case 0x00:
        ptr = (void __far *)read_dword(0x00, 0x1f * 4);
        break;
    case 0x01:
        ptr = (void __far *)read_dword(0x00, 0x43 * 4);
        break;
    case 0x02:
        ptr = 0xC000 :> vgafont14;
        break;
    case 0x03:
        ptr = 0xC000 :> vgafont8;
        break;
    case 0x04:
        ptr = 0xC000 :> (vgafont8 + 128 * 8);
        break;
    case 0x05:
        ptr = 0xC000 :> vgafont14alt;
        break;
    case 0x06:
        ptr = 0xC000 :> vgafont16;
        break;
    case 0x07:
        ptr = 0xC000 :> vgafont16alt;
        break;
    default:
#ifdef VGA_DEBUG
        printf("Get font info subfn(%02x) not implemented\n", func);
#endif
        return;
    }
    /* Split the far pointer and write it back. */
    *u_ofs = (uint16_t)ptr;
    *u_seg = (uint32_t)ptr >> 16;

    /* The character height (effectively bytes per glyph). */
    *c_height = read_byte(BIOSMEM_SEG, BIOSMEM_CHAR_HEIGHT);

    /* The highest row number. */
    *max_row = read_byte(BIOSMEM_SEG, BIOSMEM_NB_ROWS);
}

static void vga_read_pixel(uint8_t page, uint16_t col, uint16_t row, uint16_t STACK_BASED *pixel)
{
    uint8_t     mode, line, mask, attr, data, i;
    uint16_t    addr;

    /* Determine current mode characteristics. */
    mode = read_byte(BIOSMEM_SEG, BIOSMEM_CURRENT_MODE);
    line = find_vga_entry(mode);
    if (line == 0xFF)
        return;
    if (vga_modes[line].class == TEXT)
        return;

    /* Read data depending on memory model. */
    switch (vga_modes[line].memmodel) {
    case PLANAR4:
    case PLANAR1:
        addr = col / 8 + row * read_word(BIOSMEM_SEG, BIOSMEM_NB_COLS);
        mask = 0x80 >> (col & 0x07);
        attr = 0x00;
        for (i = 0; i < 4; i++) {
            outw(VGAREG_GRDC_ADDRESS, (i << 8) | 0x04);
            data = read_byte(0xa000,addr) & mask;
            if (data > 0)
                attr |= (0x01 << i);
        }
        break;
    case CGA:
        addr = (col >> 2) + (row >> 1) * 80;
        if (row & 1)
            addr += 0x2000;
        data = read_byte(0xb800, addr);
        if (vga_modes[line].pixbits == 2)
            attr = (data >> ((3 - (col & 0x03)) * 2)) & 0x03;
        else
            attr = (data >> (7 - (col & 0x07))) & 0x01;
        break;
    case LINEAR8:
        addr = col + row * (read_word(BIOSMEM_SEG, BIOSMEM_NB_COLS) * 8);
        attr = read_byte(0xa000, addr);
        break;
    default:
#ifdef VGA_DEBUG
       unimplemented();
#endif
        attr = 0;
    }
    *(uint8_t STACK_BASED *)pixel = attr;
}



// --------------------------------------------------------------------------------------------
/*static*/ void biosfn_perform_gray_scale_summing(uint16_t start, uint16_t count)
{uint8_t r,g,b;
 uint16_t i;
 uint16_t index;

 inb(VGAREG_ACTL_RESET);
 outb(VGAREG_ACTL_ADDRESS,0x00);

 for( index = 0; index < count; index++ )
  {
   // set read address and switch to read mode
   outb(VGAREG_DAC_READ_ADDRESS,start);
   // get 6-bit wide RGB data values
   r=inb( VGAREG_DAC_DATA );
   g=inb( VGAREG_DAC_DATA );
   b=inb( VGAREG_DAC_DATA );

   // intensity = ( 0.3 * Red ) + ( 0.59 * Green ) + ( 0.11 * Blue )
   i = ( ( 77*r + 151*g + 28*b ) + 0x80 ) >> 8;

   if(i>0x3f)i=0x3f;

   // set write address and switch to write mode
   outb(VGAREG_DAC_WRITE_ADDRESS,start);
   // write new intensity value
   outb( VGAREG_DAC_DATA, i&0xff );
   outb( VGAREG_DAC_DATA, i&0xff );
   outb( VGAREG_DAC_DATA, i&0xff );
   start++;
  }
 inb(VGAREG_ACTL_RESET);
 outb(VGAREG_ACTL_ADDRESS,0x20);
#ifdef VBOX
 inb(VGAREG_ACTL_RESET);
#endif /* VBOX */
}

// --------------------------------------------------------------------------------------------
static void biosfn_set_cursor_shape(uint8_t CH, uint8_t CL)
{uint16_t cheight,curs,crtc_addr;
 uint8_t modeset_ctl;

 CH&=0x3f;
 CL&=0x1f;

 curs=(CH<<8)+CL;
 write_word(BIOSMEM_SEG,BIOSMEM_CURSOR_TYPE,curs);

 modeset_ctl=read_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL);
 cheight = read_word(BIOSMEM_SEG,BIOSMEM_CHAR_HEIGHT);
 if((modeset_ctl&0x01) && (cheight>8) && (CL<8) && (CH<0x20))
  {
   if(CL!=(CH+1))
    {
     CH = ((CH+1) * cheight / 8) -1;
    }
   else
    {
     CH = ((CL+1) * cheight / 8) - 2;
    }
   CL = ((CL+1) * cheight / 8) - 1;
  }

 // CTRC regs 0x0a and 0x0b
 crtc_addr=read_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS);
 outb(crtc_addr,0x0a);
 outb(crtc_addr+1,CH);
 outb(crtc_addr,0x0b);
 outb(crtc_addr+1,CL);
}

// --------------------------------------------------------------------------------------------
static void biosfn_set_cursor_pos (uint8_t page, uint16_t cursor)
{
 uint8_t xcurs,ycurs,current;
 uint16_t nbcols,nbrows,address,crtc_addr;

 // Should not happen...
 if(page>7)return;

 // Bios cursor pos
 write_word(BIOSMEM_SEG, BIOSMEM_CURSOR_POS+2*page, cursor);

 // Set the hardware cursor
 current=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE);
 if(page==current)
  {
   // Get the dimensions
   nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);
   nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;

   xcurs=cursor&0x00ff;ycurs=(cursor&0xff00)>>8;

   // Calculate the address knowing nbcols nbrows and page num
   address=SCREEN_IO_START(nbcols,nbrows,page)+xcurs+ycurs*nbcols;

   // CRTC regs 0x0e and 0x0f
   crtc_addr=read_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS);
   outb(crtc_addr,0x0e);
   outb(crtc_addr+1,(address&0xff00)>>8);
   outb(crtc_addr,0x0f);
   outb(crtc_addr+1,address&0x00ff);
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_set_active_page(uint8_t page)
{
 uint16_t cursor,dummy,crtc_addr;
 uint16_t nbcols,nbrows,address;
 uint8_t mode,line;

 if(page>7)return;

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;

 // Get pos curs pos for the right page
 vga_get_cursor_pos(page,&dummy,&cursor);

 if(vga_modes[line].class==TEXT)
  {
   // Get the dimensions
   nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);
   nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;

   // Calculate the address knowing nbcols nbrows and page num
   address=SCREEN_MEM_START(nbcols,nbrows,page);
   write_word(BIOSMEM_SEG,BIOSMEM_CURRENT_START,address);

   // Start address
   address=SCREEN_IO_START(nbcols,nbrows,page);
  }
 else
  {
   address = page * (*(uint16_t *)&video_param_table[line_to_vpti[line]].slength_l);
  }

 // CRTC regs 0x0c and 0x0d
 crtc_addr=read_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS);
 outb(crtc_addr,0x0c);
 outb(crtc_addr+1,(address&0xff00)>>8);
 outb(crtc_addr,0x0d);
 outb(crtc_addr+1,address&0x00ff);

 // And change the BIOS page
 write_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE,page);

#ifdef VGA_DEBUG
 printf("Set active page %02x address %04x\n",page,address);
#endif

 // Display the cursor, now the page is active
 biosfn_set_cursor_pos(page,cursor);
}

/// @todo Evaluate whether executing INT 10h is the right thing here
extern void vga_font_set(uint8_t function, uint8_t data);
#pragma aux vga_font_set =  \
    "mov    ah, 11h"        \
    "int    10h"            \
    parm [al] [bl];

// ============================================================================================
//
// BIOS functions
//
// ============================================================================================

/* CGA-compatible MSR (0x3D8) register values for first modes 0-7. */
uint8_t cga_msr[8] = {
    0x2C, 0x28, 0x2D, 0x29, 0x2A, 0x2E, 0x1E, 0x29
};

void biosfn_set_video_mode(uint8_t mode)
{// mode: Bit 7 is 1 if no clear screen

 // Should we clear the screen ?
 uint8_t  noclearmem=mode&0x80;
 uint8_t  line,mmask,*palette,vpti;
 uint16_t i,twidth,theightm1,cheight;
 uint8_t  modeset_ctl,video_ctl,vga_switches;
 uint16_t crtc_addr;

#ifdef VBE
 if (vbe_has_vbe_display()) {
   // Force controller into VGA mode
   outb(VGAREG_SEQU_ADDRESS,7);
   outb(VGAREG_SEQU_DATA,0x00);
  }
#endif // def VBE

 // The real mode
 mode=mode&0x7f;

 // Display switching is not supported, and mono monitors aren't either.
 // Requests to set mode 7 (mono) must set mode 0 instead (color).
 if (mode == 7)
     mode = 0;

 // find the entry in the video modes
 line=find_vga_entry(mode);

#ifdef VGA_DEBUG
 printf("mode search %02x found line %02x\n",mode,line);
#endif

 if(line==0xFF)
  return;

 vpti=line_to_vpti[line];
 twidth=video_param_table[vpti].twidth;
 theightm1=video_param_table[vpti].theightm1;
 cheight=video_param_table[vpti].cheight;

 // Read the bios vga control
 video_ctl=read_byte(BIOSMEM_SEG,BIOSMEM_VIDEO_CTL);

 // Read the bios vga switches
 vga_switches=read_byte(BIOSMEM_SEG,BIOSMEM_SWITCHES);

 // Read the bios mode set control
 modeset_ctl=read_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL);

 // Then we know the number of lines
// FIXME

 // if palette loading (bit 3 of modeset ctl = 0)
 if((modeset_ctl&0x08)==0)
  {// Set the PEL mask
   outb(VGAREG_PEL_MASK,vga_modes[line].pelmask);

   // Set the whole dac always, from 0
   outb(VGAREG_DAC_WRITE_ADDRESS,0x00);

   // From which palette
   switch(vga_modes[line].dacmodel)
    {case 0:
      palette=&palette0[0];
      break;
     case 1:
      palette=&palette1[0];
      break;
     case 2:
      palette=&palette2[0];
      break;
     case 3:
      palette=&palette3[0];
      break;
    }
   // Always 256*3 values
   for(i=0;i<0x0100;i++)
    {if(i<=dac_regs[vga_modes[line].dacmodel])
      {outb(VGAREG_DAC_DATA,palette[(i*3)+0]);
       outb(VGAREG_DAC_DATA,palette[(i*3)+1]);
       outb(VGAREG_DAC_DATA,palette[(i*3)+2]);
      }
     else
      {outb(VGAREG_DAC_DATA,0);
       outb(VGAREG_DAC_DATA,0);
       outb(VGAREG_DAC_DATA,0);
      }
    }
   if((modeset_ctl&0x02)==0x02)
    {
     biosfn_perform_gray_scale_summing(0x00, 0x100);
    }
  }

 // Reset Attribute Ctl flip-flop
 inb(VGAREG_ACTL_RESET);

 // Set Attribute Ctl
 for(i=0;i<=0x13;i++)
  {outb(VGAREG_ACTL_ADDRESS,i);
   outb(VGAREG_ACTL_WRITE_DATA,video_param_table[vpti].actl_regs[i]);
  }
 outb(VGAREG_ACTL_ADDRESS,0x14);
 outb(VGAREG_ACTL_WRITE_DATA,0x00);

 // Set Sequencer Ctl
 outb(VGAREG_SEQU_ADDRESS,0);
 outb(VGAREG_SEQU_DATA,0x03);
 for(i=1;i<=4;i++)
  {outb(VGAREG_SEQU_ADDRESS,i);
   outb(VGAREG_SEQU_DATA,video_param_table[vpti].sequ_regs[i - 1]);
  }

 // Set Grafx Ctl
 for(i=0;i<=8;i++)
  {outb(VGAREG_GRDC_ADDRESS,i);
   outb(VGAREG_GRDC_DATA,video_param_table[vpti].grdc_regs[i]);
  }

 // Set CRTC address VGA or MDA
 crtc_addr=vga_modes[line].memmodel==MTEXT?VGAREG_MDA_CRTC_ADDRESS:VGAREG_VGA_CRTC_ADDRESS;

 // Disable CRTC write protection
 outw(crtc_addr,0x0011);
 // Set CRTC regs
 for(i=0;i<=0x18;i++)
  {outb(crtc_addr,i);
   outb(crtc_addr+1,video_param_table[vpti].crtc_regs[i]);
  }

 // Set the misc register
 outb(VGAREG_WRITE_MISC_OUTPUT,video_param_table[vpti].miscreg);

 // Enable video
 outb(VGAREG_ACTL_ADDRESS,0x20);
 inb(VGAREG_ACTL_RESET);

 if(noclearmem==0x00)
  {
   if(vga_modes[line].class==TEXT)
    {
     memsetw(vga_modes[line].sstart,0,0x0720,0x4000); // 32k
    }
   else
    {
     if(mode<0x0d)
      {
       memsetw(vga_modes[line].sstart,0,0x0000,0x4000); // 32k
      }
     else
      {
       outb( VGAREG_SEQU_ADDRESS, 0x02 );
       mmask = inb( VGAREG_SEQU_DATA );
       outb( VGAREG_SEQU_DATA, 0x0f ); // all planes
       memsetw(vga_modes[line].sstart,0,0x0000,0x8000); // 64k
       outb( VGAREG_SEQU_DATA, mmask );
      }
    }
  }

 // Set the BIOS mem
 write_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE,mode);
 write_word(BIOSMEM_SEG,BIOSMEM_NB_COLS,twidth);
 write_word(BIOSMEM_SEG,BIOSMEM_PAGE_SIZE,*(uint16_t *)&video_param_table[vpti].slength_l);
 write_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS,crtc_addr);
 write_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS,theightm1);
 write_word(BIOSMEM_SEG,BIOSMEM_CHAR_HEIGHT,cheight);
 write_byte(BIOSMEM_SEG,BIOSMEM_VIDEO_CTL,(0x60|noclearmem));
 write_byte(BIOSMEM_SEG,BIOSMEM_SWITCHES,0xF9);
 write_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL,read_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL)&0x7f);

 // FIXME We nearly have the good tables. to be reworked
 write_byte(BIOSMEM_SEG,BIOSMEM_DCC_INDEX,0x08);    // 8 is VGA should be ok for now
 write_dword(BIOSMEM_SEG,BIOSMEM_VS_POINTER, (uint32_t)(void __far *)video_save_pointer_table);

 if (mode <= 7)
 {
     write_byte(BIOSMEM_SEG, BIOSMEM_CURRENT_MSR, cga_msr[mode]);           /* Like CGA reg. 0x3D8 */
     write_byte(BIOSMEM_SEG, BIOSMEM_CURRENT_PAL, mode == 6 ? 0x3F : 0x30); /* Like CGA reg. 0x3D9*/
 }

 // Set cursor shape
 if(vga_modes[line].class==TEXT)
  {
   biosfn_set_cursor_shape(0x06,0x07);
  }

 // Set cursor pos for page 0..7
 for(i=0;i<8;i++)
  biosfn_set_cursor_pos(i,0x0000);

 // Set active page 0
 biosfn_set_active_page(0x00);

 // Write the fonts in memory
 if(vga_modes[line].class==TEXT)
  {
     vga_font_set(0x04, 0);     /* Load 8x16 font into page 0. */
     vga_font_set(0x03, 0);     /* Select font page mode 0. */
  }

 // Set the ints 0x1F and 0x43
 set_int_vector(0x1f, vgafont8+128*8);

  switch(cheight)
   {case 8:
     set_int_vector(0x43, vgafont8);
     break;
    case 14:
     set_int_vector(0x43, vgafont14);
     break;
    case 16:
     set_int_vector(0x43, vgafont16);
     break;
   }
}

// --------------------------------------------------------------------------------------------
static void vgamem_copy_pl4(uint8_t xstart, uint8_t ysrc, uint8_t ydest,
                            uint8_t cols, uint8_t nbcols, uint8_t cheight)
{
 uint16_t src,dest;
 uint8_t i;

 src=ysrc*cheight*nbcols+xstart;
 dest=ydest*cheight*nbcols+xstart;
 outw(VGAREG_GRDC_ADDRESS, 0x0105);
 for(i=0;i<cheight;i++)
  {
   memcpyb(0xa000,dest+i*nbcols,0xa000,src+i*nbcols,cols);
  }
 outw(VGAREG_GRDC_ADDRESS, 0x0005);
}

// --------------------------------------------------------------------------------------------
static void vgamem_fill_pl4(uint8_t xstart, uint8_t ystart, uint8_t cols,
                            uint8_t nbcols, uint8_t cheight, uint8_t attr)
{
 uint16_t dest;
 uint8_t i;

 dest=ystart*cheight*nbcols+xstart;
 outw(VGAREG_GRDC_ADDRESS, 0x0205);
 for(i=0;i<cheight;i++)
  {
   memsetb(0xa000,dest+i*nbcols,attr,cols);
  }
 outw(VGAREG_GRDC_ADDRESS, 0x0005);
}

// --------------------------------------------------------------------------------------------
static void vgamem_copy_cga(uint8_t xstart, uint8_t ysrc, uint8_t ydest,
                            uint8_t cols, uint8_t nbcols, uint8_t cheight)
{
 uint16_t src,dest;
 uint8_t i;

 src=((ysrc*cheight*nbcols)>>1)+xstart;
 dest=((ydest*cheight*nbcols)>>1)+xstart;
 for(i=0;i<cheight;i++)
  {
   if (i & 1)
     memcpyb(0xb800,0x2000+dest+(i>>1)*nbcols,0xb800,0x2000+src+(i>>1)*nbcols,cols);
   else
     memcpyb(0xb800,dest+(i>>1)*nbcols,0xb800,src+(i>>1)*nbcols,cols);
  }
}

// --------------------------------------------------------------------------------------------
static void vgamem_fill_cga(uint8_t xstart, uint8_t ystart, uint8_t cols,
                            uint8_t nbcols, uint8_t cheight, uint8_t attr)
{
 uint16_t dest;
 uint8_t i;

 dest=((ystart*cheight*nbcols)>>1)+xstart;
 for(i=0;i<cheight;i++)
  {
   if (i & 1)
     memsetb(0xb800,0x2000+dest+(i>>1)*nbcols,attr,cols);
   else
     memsetb(0xb800,dest+(i>>1)*nbcols,attr,cols);
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_scroll(uint8_t nblines, uint8_t attr, uint8_t rul, uint8_t cul,
                          uint8_t rlr, uint8_t clr, uint8_t page, uint8_t dir)
{
 // page == 0xFF if current

 uint8_t mode,line,cheight,bpp,cols;
 uint16_t nbcols,nbrows,i;
 uint16_t address;

 if(rul>rlr)return;
 if(cul>clr)return;

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;

 // Get the dimensions
 nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;
 nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);

 // Get the current page
 if(page==0xFF)
  page=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE);

 if(rlr>=nbrows)rlr=nbrows-1;
 if(clr>=nbcols)clr=nbcols-1;
 if(nblines>nbrows)nblines=0;
 cols=clr-cul+1;

 if(vga_modes[line].class==TEXT)
  {
   // Compute the address
   address=SCREEN_MEM_START(nbcols,nbrows,page);
#ifdef VGA_DEBUG
   printf("Scroll, address %04x (%04x %04x %02x)\n",address,nbrows,nbcols,page);
#endif

   if(nblines==0&&rul==0&&cul==0&&rlr==nbrows-1&&clr==nbcols-1)
    {
     memsetw(vga_modes[line].sstart,address,(uint16_t)attr*0x100+' ',nbrows*nbcols);
    }
   else
    {// if Scroll up
     if(dir==SCROLL_UP)
      {for(i=rul;i<=rlr;i++)
        {
         if((i+nblines>rlr)||(nblines==0))
          memsetw(vga_modes[line].sstart,address+(i*nbcols+cul)*2,(uint16_t)attr*0x100+' ',cols);
         else
          memcpyw(vga_modes[line].sstart,address+(i*nbcols+cul)*2,vga_modes[line].sstart,((i+nblines)*nbcols+cul)*2,cols);
        }
      }
     else
      {for(i=rlr;i>=rul;i--)
        {
         if((i<rul+nblines)||(nblines==0))
          memsetw(vga_modes[line].sstart,address+(i*nbcols+cul)*2,(uint16_t)attr*0x100+' ',cols);
         else
          memcpyw(vga_modes[line].sstart,address+(i*nbcols+cul)*2,vga_modes[line].sstart,((i-nblines)*nbcols+cul)*2,cols);
         if (i>rlr) break;
        }
      }
    }
  }
 else
  {
   // FIXME gfx mode not complete
   cheight=video_param_table[line_to_vpti[line]].cheight;
   switch(vga_modes[line].memmodel)
    {
     case PLANAR4:
     case PLANAR1:
       if(nblines==0&&rul==0&&cul==0&&rlr==nbrows-1&&clr==nbcols-1)
        {
         outw(VGAREG_GRDC_ADDRESS, 0x0205);
         memsetb(vga_modes[line].sstart,0,attr,nbrows*nbcols*cheight);
         outw(VGAREG_GRDC_ADDRESS, 0x0005);
        }
       else
        {// if Scroll up
         if(dir==SCROLL_UP)
          {for(i=rul;i<=rlr;i++)
            {
             if((i+nblines>rlr)||(nblines==0))
              vgamem_fill_pl4(cul,i,cols,nbcols,cheight,attr);
             else
              vgamem_copy_pl4(cul,i+nblines,i,cols,nbcols,cheight);
            }
          }
         else
          {for(i=rlr;i>=rul;i--)
            {
             if((i<rul+nblines)||(nblines==0))
              vgamem_fill_pl4(cul,i,cols,nbcols,cheight,attr);
             else
              vgamem_copy_pl4(cul,i,i-nblines,cols,nbcols,cheight);
             if (i>rlr) break;
            }
          }
        }
       break;
     case CGA:
       bpp=vga_modes[line].pixbits;
       if(nblines==0&&rul==0&&cul==0&&rlr==nbrows-1&&clr==nbcols-1)
        {
         memsetb(vga_modes[line].sstart,0,attr,nbrows*nbcols*cheight*bpp);
        }
       else
        {
         if(bpp==2)
          {
           cul<<=1;
           cols<<=1;
           nbcols<<=1;
          }
         // if Scroll up
         if(dir==SCROLL_UP)
          {for(i=rul;i<=rlr;i++)
            {
             if((i+nblines>rlr)||(nblines==0))
              vgamem_fill_cga(cul,i,cols,nbcols,cheight,attr);
             else
              vgamem_copy_cga(cul,i+nblines,i,cols,nbcols,cheight);
            }
          }
         else
          {for(i=rlr;i>=rul;i--)
            {
             if((i<rul+nblines)||(nblines==0))
              vgamem_fill_cga(cul,i,cols,nbcols,cheight,attr);
             else
              vgamem_copy_cga(cul,i,i-nblines,cols,nbcols,cheight);
             if (i>rlr) break;
            }
          }
        }
       break;
#ifdef VGA_DEBUG
     default:
       printf("Scroll in graphics mode ");
       unimplemented();
#endif
    }
  }
}

// --------------------------------------------------------------------------------------------
static void write_gfx_char_pl4(uint8_t car, uint8_t attr, uint8_t xcurs,
                               uint8_t ycurs, uint8_t nbcols, uint8_t cheight)
{
 uint8_t i,j,mask;
 uint8_t *fdata;
 uint16_t addr,dest,src;

 switch(cheight)
  {case 14:
    fdata = &vgafont14;
    break;
   case 16:
    fdata = &vgafont16;
    break;
   default:
    fdata = &vgafont8;
  }
 addr=xcurs+ycurs*cheight*nbcols;
 src = car * cheight;
 outw(VGAREG_SEQU_ADDRESS, 0x0f02);
 outw(VGAREG_GRDC_ADDRESS, 0x0205);
 if(attr&0x80)
  {
   outw(VGAREG_GRDC_ADDRESS, 0x1803);
  }
 else
  {
   outw(VGAREG_GRDC_ADDRESS, 0x0003);
  }
 for(i=0;i<cheight;i++)
  {
   dest=addr+i*nbcols;
   for(j=0;j<8;j++)
    {
     mask=0x80>>j;
     outw(VGAREG_GRDC_ADDRESS, (mask << 8) | 0x08);
     read_byte(0xa000,dest);
     if(fdata[src+i]&mask)
      {
       write_byte(0xa000,dest,attr&0x0f);
      }
     else
      {
       write_byte(0xa000,dest,0x00);
      }
    }
  }
  outw(VGAREG_GRDC_ADDRESS, 0xff08);
  outw(VGAREG_GRDC_ADDRESS, 0x0005);
  outw(VGAREG_GRDC_ADDRESS, 0x0003);
}

// --------------------------------------------------------------------------------------------
static void write_gfx_char_cga(uint8_t car, uint8_t attr, uint8_t xcurs,
                               uint8_t ycurs, uint8_t nbcols, uint8_t bpp)
{
 uint8_t i,j,mask,data;
 uint8_t *fdata;
 uint16_t addr,dest,src;

 fdata = &vgafont8;
 addr=(xcurs*bpp)+ycurs*320;
 src = car * 8;
 for(i=0;i<8;i++)
  {
   dest=addr+(i>>1)*80;
   if (i & 1) dest += 0x2000;
   mask = 0x80;
   if (bpp == 1)
    {
     if (attr & 0x80)
      {
       data = read_byte(0xb800,dest);
      }
     else
      {
       data = 0x00;
      }
     for(j=0;j<8;j++)
      {
       if (fdata[src+i] & mask)
        {
         if (attr & 0x80)
          {
           data ^= (attr & 0x01) << (7-j);
          }
         else
          {
           data |= (attr & 0x01) << (7-j);
          }
        }
       mask >>= 1;
      }
     write_byte(0xb800,dest,data);
    }
   else
    {
     while (mask > 0)
      {
       if (attr & 0x80)
        {
         data = read_byte(0xb800,dest);
        }
       else
        {
         data = 0x00;
        }
       for(j=0;j<4;j++)
        {
         if (fdata[src+i] & mask)
          {
           if (attr & 0x80)
            {
             data ^= (attr & 0x03) << ((3-j)*2);
            }
           else
            {
             data |= (attr & 0x03) << ((3-j)*2);
            }
          }
         mask >>= 1;
        }
       write_byte(0xb800,dest,data);
       dest += 1;
      }
    }
  }
}

// --------------------------------------------------------------------------------------------
static void write_gfx_char_lin(uint8_t car, uint8_t attr, uint8_t xcurs,
                               uint8_t ycurs, uint8_t nbcols)
{
 uint8_t i,j,mask,data;
 uint8_t *fdata;
 uint16_t addr,dest,src;

 fdata = &vgafont8;
 addr=xcurs*8+ycurs*nbcols*64;
 src = car * 8;
 for(i=0;i<8;i++)
  {
   dest=addr+i*nbcols*8;
   mask = 0x80;
   for(j=0;j<8;j++)
    {
     data = 0x00;
     if (fdata[src+i] & mask)
      {
       data = attr;
      }
     write_byte(0xa000,dest+j,data);
     mask >>= 1;
    }
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_write_char_attr(uint8_t car, uint8_t page, uint8_t attr, uint16_t count)
{
 uint8_t cheight,xcurs,ycurs,mode,line,bpp;
 uint16_t nbcols,nbrows,address;
 uint16_t cursor,dummy;

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;

 // Get the cursor pos for the page
 vga_get_cursor_pos(page,&dummy,&cursor);
 xcurs=cursor&0x00ff;ycurs=(cursor&0xff00)>>8;

 // Get the dimensions
 nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;
 nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);

 if(vga_modes[line].class==TEXT)
  {
   // Compute the address
   address=SCREEN_MEM_START(nbcols,nbrows,page)+(xcurs+ycurs*nbcols)*2;

   dummy=((uint16_t)attr<<8)+car;
   memsetw(vga_modes[line].sstart,address,dummy,count);
  }
 else
  {
   // FIXME gfx mode not complete
   cheight=video_param_table[line_to_vpti[line]].cheight;
   bpp=vga_modes[line].pixbits;
   while((count-->0) && (xcurs<nbcols))
    {
     switch(vga_modes[line].memmodel)
      {
       case PLANAR4:
       case PLANAR1:
         write_gfx_char_pl4(car,attr,xcurs,ycurs,nbcols,cheight);
         break;
       case CGA:
         write_gfx_char_cga(car,attr,xcurs,ycurs,nbcols,bpp);
         break;
       case LINEAR8:
         write_gfx_char_lin(car,attr,xcurs,ycurs,nbcols);
         break;
#ifdef VGA_DEBUG
       default:
         unimplemented();
#endif
      }
     xcurs++;
    }
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_write_char_only(uint8_t car, uint8_t page, uint8_t attr, uint16_t count)
{
 uint8_t cheight,xcurs,ycurs,mode,line,bpp;
 uint16_t nbcols,nbrows,address;
 uint16_t cursor,dummy;

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;

 // Get the cursor pos for the page
 vga_get_cursor_pos(page,&dummy,&cursor);
 xcurs=cursor&0x00ff;ycurs=(cursor&0xff00)>>8;

 // Get the dimensions
 nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;
 nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);

 if(vga_modes[line].class==TEXT)
  {
   // Compute the address
   address=SCREEN_MEM_START(nbcols,nbrows,page)+(xcurs+ycurs*nbcols)*2;

   while(count-->0)
    {write_byte(vga_modes[line].sstart,address,car);
     address+=2;
    }
  }
 else
  {
   // FIXME gfx mode not complete
   cheight=video_param_table[line_to_vpti[line]].cheight;
   bpp=vga_modes[line].pixbits;
   while((count-->0) && (xcurs<nbcols))
    {
     switch(vga_modes[line].memmodel)
      {
       case PLANAR4:
       case PLANAR1:
         write_gfx_char_pl4(car,attr,xcurs,ycurs,nbcols,cheight);
         break;
       case CGA:
         write_gfx_char_cga(car,attr,xcurs,ycurs,nbcols,bpp);
         break;
       case LINEAR8:
         write_gfx_char_lin(car,attr,xcurs,ycurs,nbcols);
         break;
#ifdef VGA_DEBUG
       default:
         unimplemented();
#endif
      }
     xcurs++;
    }
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_write_pixel(uint8_t BH, uint8_t AL, uint16_t CX, uint16_t DX)
{
 uint8_t mode,line,mask,attr,data;
 uint16_t addr;

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;
 if(vga_modes[line].class==TEXT)return;

 switch(vga_modes[line].memmodel)
  {
   case PLANAR4:
   case PLANAR1:
     addr = CX/8+DX*read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);
     mask = 0x80 >> (CX & 0x07);
     outw(VGAREG_GRDC_ADDRESS, (mask << 8) | 0x08);
     outw(VGAREG_GRDC_ADDRESS, 0x0205);
     data = read_byte(0xa000,addr);
     if (AL & 0x80)
      {
       outw(VGAREG_GRDC_ADDRESS, 0x1803);
      }
     write_byte(0xa000,addr,AL);
     outw(VGAREG_GRDC_ADDRESS, 0xff08);
     outw(VGAREG_GRDC_ADDRESS, 0x0005);
     outw(VGAREG_GRDC_ADDRESS, 0x0003);
     break;
   case CGA:
     if(vga_modes[line].pixbits==2)
      {
       addr=(CX>>2)+(DX>>1)*80;
      }
     else
      {
       addr=(CX>>3)+(DX>>1)*80;
      }
     if (DX & 1) addr += 0x2000;
     data = read_byte(0xb800,addr);
     if(vga_modes[line].pixbits==2)
      {
       attr = (AL & 0x03) << ((3 - (CX & 0x03)) * 2);
       mask = 0x03 << ((3 - (CX & 0x03)) * 2);
      }
     else
      {
       attr = (AL & 0x01) << (7 - (CX & 0x07));
       mask = 0x01 << (7 - (CX & 0x07));
      }
     if (AL & 0x80)
      {
       data ^= attr;
      }
     else
      {
       data &= ~mask;
       data |= attr;
      }
     write_byte(0xb800,addr,data);
     break;
   case LINEAR8:
     addr=CX+DX*(read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS)*8);
     write_byte(0xa000,addr,AL);
     break;
#ifdef VGA_DEBUG
   default:
     unimplemented();
#endif
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_write_teletype(uint8_t car, uint8_t page, uint8_t attr, uint8_t flag)
{// flag = WITH_ATTR / NO_ATTR

 uint8_t cheight,xcurs,ycurs,mode,line,bpp;
 uint16_t nbcols,nbrows,address;
 uint16_t cursor,dummy;

 // special case if page is 0xff, use current page
 if(page==0xff)
  page=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE);

 // Get the mode
 mode=read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE);
 line=find_vga_entry(mode);
 if(line==0xFF)return;

 // Get the cursor pos for the page
 vga_get_cursor_pos(page,&dummy,&cursor);
 xcurs=cursor&0x00ff;ycurs=(cursor&0xff00)>>8;

 // Get the dimensions
 nbrows=read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)+1;
 nbcols=read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);

 switch(car)
  {
   case '\a':   // ASCII 0x07, BEL
    //FIXME should beep
    break;

   case '\b':   // ASCII 0x08, BS
    if(xcurs>0)xcurs--;
    break;

   case '\n':   // ASCII 0x0A, LF
    ycurs++;
    break;

   case '\r':   // ASCII 0x0D, CR
    xcurs=0;
    break;

   default:

    if(vga_modes[line].class==TEXT)
     {
      // Compute the address
      address=SCREEN_MEM_START(nbcols,nbrows,page)+(xcurs+ycurs*nbcols)*2;

      // Write the char
      write_byte(vga_modes[line].sstart,address,car);

      if(flag==WITH_ATTR)
       write_byte(vga_modes[line].sstart,address+1,attr);
     }
    else
     {
      // FIXME gfx mode not complete
      cheight=video_param_table[line_to_vpti[line]].cheight;
      bpp=vga_modes[line].pixbits;
      switch(vga_modes[line].memmodel)
       {
        case PLANAR4:
        case PLANAR1:
          write_gfx_char_pl4(car,attr,xcurs,ycurs,nbcols,cheight);
          break;
        case CGA:
          write_gfx_char_cga(car,attr,xcurs,ycurs,nbcols,bpp);
          break;
        case LINEAR8:
          write_gfx_char_lin(car,attr,xcurs,ycurs,nbcols);
          break;
#ifdef VGA_DEBUG
        default:
          unimplemented();
#endif
       }
     }
    xcurs++;
    // Do we need to wrap ?
    if(xcurs==nbcols)
     {xcurs=0;
      ycurs++;
     }
  }

 // Do we need to scroll ?
 if(ycurs==nbrows)
  {
   if(vga_modes[line].class==TEXT)
    {
     address=SCREEN_MEM_START(nbcols,nbrows,page)+(xcurs+(ycurs-1)*nbcols)*2;
     attr=read_byte(vga_modes[line].sstart,address+1);
     biosfn_scroll(0x01,attr,0,0,nbrows-1,nbcols-1,page,SCROLL_UP);
    }
   else
    {
     biosfn_scroll(0x01,0x00,0,0,nbrows-1,nbcols-1,page,SCROLL_UP);
    }
   ycurs-=1;
  }

 // Set the cursor for the page
 cursor=ycurs; cursor<<=8; cursor+=xcurs;
 biosfn_set_cursor_pos(page,cursor);
}

// --------------------------------------------------------------------------------------------
static void get_font_access(void)
{
    outw(VGAREG_SEQU_ADDRESS, 0x0100);
    outw(VGAREG_SEQU_ADDRESS, 0x0402);
    outw(VGAREG_SEQU_ADDRESS, 0x0704);
    outw(VGAREG_SEQU_ADDRESS, 0x0300);
    outw(VGAREG_GRDC_ADDRESS, 0x0204);
    outw(VGAREG_GRDC_ADDRESS, 0x0005);
    outw(VGAREG_GRDC_ADDRESS, 0x0406);
}

static void release_font_access(void)
{
    outw(VGAREG_SEQU_ADDRESS, 0x0100);
    outw(VGAREG_SEQU_ADDRESS, 0x0302);
    outw(VGAREG_SEQU_ADDRESS, 0x0304);
    outw(VGAREG_SEQU_ADDRESS, 0x0300);
    outw(VGAREG_GRDC_ADDRESS, (((0x0a | ((inb(VGAREG_READ_MISC_OUTPUT) & 0x01) << 2)) << 8) | 0x06));
    outw(VGAREG_GRDC_ADDRESS, 0x0004);
    outw(VGAREG_GRDC_ADDRESS, 0x1005);
}

static void set_scan_lines(uint8_t lines)
{
 uint16_t crtc_addr,cols,vde;
 uint8_t crtc_r9,ovl,rows;

 crtc_addr = read_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS);
 outb(crtc_addr, 0x09);
 crtc_r9 = inb(crtc_addr+1);
 crtc_r9 = (crtc_r9 & 0xe0) | (lines - 1);
 outb(crtc_addr+1, crtc_r9);
 if(lines==8)
  {
   biosfn_set_cursor_shape(0x06,0x07);
  }
 else
  {
   biosfn_set_cursor_shape(lines-4,lines-3);
  }
 write_word(BIOSMEM_SEG,BIOSMEM_CHAR_HEIGHT, lines);
 outb(crtc_addr, 0x12);
 vde = inb(crtc_addr+1);
 outb(crtc_addr, 0x07);
 ovl = inb(crtc_addr+1);
 vde += (((ovl & 0x02) << 7) + ((ovl & 0x40) << 3) + 1);
 rows = vde / lines;
 write_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS, rows-1);
 cols = read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS);
 write_word(BIOSMEM_SEG,BIOSMEM_PAGE_SIZE, rows * cols * 2);
}

static void biosfn_load_text_user_pat(uint8_t AL, uint16_t ES, uint16_t BP, uint16_t CX,
                                      uint16_t DX, uint8_t BL, uint8_t BH)
{
 uint16_t blockaddr,dest,i,src;

 get_font_access();
 blockaddr = ((BL & 0x03) << 14) + ((BL & 0x04) << 11);
 for(i=0;i<CX;i++)
  {
   src = BP + i * BH;
   dest = blockaddr + (DX + i) * 32;
   memcpyb(0xA000, dest, ES, src, BH);
  }
 release_font_access();
 if(AL>=0x10)
  {
   set_scan_lines(BH);
  }
}

static void biosfn_load_text_8_14_pat(uint8_t AL, uint8_t BL)
{
 uint16_t blockaddr,dest,i,src;

 get_font_access();
 blockaddr = ((BL & 0x03) << 14) + ((BL & 0x04) << 11);
 for(i=0;i<0x100;i++)
  {
   src = i * 14;
   dest = blockaddr + i * 32;
   memcpyb(0xA000, dest, 0xC000, (uint16_t)vgafont14+src, 14);
  }
 release_font_access();
 if(AL>=0x10)
  {
   set_scan_lines(14);
  }
}

static void biosfn_load_text_8_8_pat(uint8_t AL, uint8_t BL)
{
 uint16_t blockaddr,dest,i,src;

 get_font_access();
 blockaddr = ((BL & 0x03) << 14) + ((BL & 0x04) << 11);
 for(i=0;i<0x100;i++)
  {
   src = i * 8;
   dest = blockaddr + i * 32;
   memcpyb(0xA000, dest, 0xC000, (uint16_t)vgafont8+src, 8);
  }
 release_font_access();
 if(AL>=0x10)
  {
   set_scan_lines(8);
  }
}

// --------------------------------------------------------------------------------------------
static void biosfn_load_text_8_16_pat(uint8_t AL, uint8_t BL)
{
 uint16_t blockaddr,dest,i,src;

 get_font_access();
 blockaddr = ((BL & 0x03) << 14) + ((BL & 0x04) << 11);
 for(i=0;i<0x100;i++)
  {
   src = i * 16;
   dest = blockaddr + i * 32;
   memcpyb(0xA000, dest, 0xC000, (uint16_t)vgafont16+src, 16);
  }
 release_font_access();
 if(AL>=0x10)
  {
   set_scan_lines(16);
  }
}

static void biosfn_load_gfx_8_8_chars(uint16_t ES, uint16_t BP)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
static void biosfn_load_gfx_user_chars(uint16_t ES, uint16_t BP, uint16_t CX,
                                       uint8_t BL, uint8_t DL)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
static void biosfn_load_gfx_8_14_chars(uint8_t BL)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
static void biosfn_load_gfx_8_8_dd_chars(uint8_t BL)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
static void biosfn_load_gfx_8_16_chars(uint8_t BL)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
// --------------------------------------------------------------------------------------------
static void biosfn_alternate_prtsc(void)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}

// --------------------------------------------------------------------------------------------
static void biosfn_switch_video_interface (AL,ES,DX) uint8_t AL;uint16_t ES;uint16_t DX;
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}
static void biosfn_enable_video_refresh_control(uint8_t AL)
{
#ifdef VGA_DEBUG
 unimplemented();
#endif
}

// --------------------------------------------------------------------------------------------
static void biosfn_write_string(uint8_t flag, uint8_t page, uint8_t attr, uint16_t count,
                                uint8_t row, uint8_t col, uint16_t seg, uint16_t offset)
{
 uint16_t newcurs,oldcurs,dummy;
 uint8_t car;

 // Read curs info for the page
 vga_get_cursor_pos(page,&dummy,&oldcurs);

 // if row=0xff special case : use current cursor position
 if(row==0xff)
  {col=oldcurs&0x00ff;
   row=(oldcurs&0xff00)>>8;
  }

 newcurs=row; newcurs<<=8; newcurs+=col;
 biosfn_set_cursor_pos(page,newcurs);

 while(count--!=0)
  {
   car=read_byte(seg,offset++);
   if((flag&0x02)!=0)
    attr=read_byte(seg,offset++);

   biosfn_write_teletype(car,page,attr,WITH_ATTR);
  }

 // Set back curs pos
 if((flag&0x01)==0)
  biosfn_set_cursor_pos(page,oldcurs);
}

// --------------------------------------------------------------------------------------------
static void biosfn_read_state_info(uint16_t BX, uint16_t ES, uint16_t DI)
{
 // Address of static functionality table
 write_dword(ES,DI+0x00, (uint32_t)(void __far *)static_functionality);

 // Hard coded copy from BIOS area. Should it be cleaner ?
 memcpyb(ES,DI+0x04,BIOSMEM_SEG,0x49,30);
 memcpyb(ES,DI+0x22,BIOSMEM_SEG,0x84,3);

 write_byte(ES,DI+0x25,read_byte(BIOSMEM_SEG,BIOSMEM_DCC_INDEX));
 write_byte(ES,DI+0x26,0);
 write_byte(ES,DI+0x27,16);
 write_byte(ES,DI+0x28,0);
 write_byte(ES,DI+0x29,8);
 write_byte(ES,DI+0x2a,2);
 write_byte(ES,DI+0x2b,0);
 write_byte(ES,DI+0x2c,0);
 write_byte(ES,DI+0x31,3);
 write_byte(ES,DI+0x32,0);

 memsetb(ES,DI+0x33,0,13);
}

// --------------------------------------------------------------------------------------------
uint16_t biosfn_read_video_state_size2(uint16_t state)
{
    uint16_t    size;

    size = 0;
    if (state & 1)
        size += 0x46;

    if (state & 2)
        size += (5 + 8 + 5) * 2 + 6;

    if (state & 4)
        size += 3 + 256 * 3 + 1;

    /// @todo Is this supposed to be in 1-byte or 64-byte units?
    return size;
}

static void vga_get_video_state_size(uint16_t state, uint16_t STACK_BASED *size)
{
    *size = biosfn_read_video_state_size2(state);
}

uint16_t biosfn_save_video_state(uint16_t CX, uint16_t ES, uint16_t BX)
{
    uint16_t i, crtc_addr, ar_index;

    crtc_addr = read_word(BIOSMEM_SEG, BIOSMEM_CRTC_ADDRESS);
    if (CX & 1) {
        write_byte(ES, BX, inb(VGAREG_SEQU_ADDRESS)); BX++;
        write_byte(ES, BX, inb(crtc_addr)); BX++;
        write_byte(ES, BX, inb(VGAREG_GRDC_ADDRESS)); BX++;
        inb(VGAREG_ACTL_RESET);
        ar_index = inb(VGAREG_ACTL_ADDRESS);
        write_byte(ES, BX, ar_index); BX++;
        write_byte(ES, BX, inb(VGAREG_READ_FEATURE_CTL)); BX++;

        for(i=1;i<=4;i++){
            outb(VGAREG_SEQU_ADDRESS, i);
            write_byte(ES, BX, inb(VGAREG_SEQU_DATA)); BX++;
        }
        outb(VGAREG_SEQU_ADDRESS, 0);
        write_byte(ES, BX, inb(VGAREG_SEQU_DATA)); BX++;

        for(i=0;i<=0x18;i++) {
            outb(crtc_addr,i);
            write_byte(ES, BX, inb(crtc_addr+1)); BX++;
        }

        for(i=0;i<=0x13;i++) {
            inb(VGAREG_ACTL_RESET);
            outb(VGAREG_ACTL_ADDRESS, i | (ar_index & 0x20));
            write_byte(ES, BX, inb(VGAREG_ACTL_READ_DATA)); BX++;
        }
        inb(VGAREG_ACTL_RESET);

        for(i=0;i<=8;i++) {
            outb(VGAREG_GRDC_ADDRESS,i);
            write_byte(ES, BX, inb(VGAREG_GRDC_DATA)); BX++;
        }

        write_word(ES, BX, crtc_addr); BX+= 2;

        /* XXX: read plane latches */
        write_byte(ES, BX, 0); BX++;
        write_byte(ES, BX, 0); BX++;
        write_byte(ES, BX, 0); BX++;
        write_byte(ES, BX, 0); BX++;
    }
    if (CX & 2) {
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE)); BX++;
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_NB_COLS)); BX += 2;
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_PAGE_SIZE)); BX += 2;
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS)); BX += 2;
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS)); BX++;
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_CHAR_HEIGHT)); BX += 2;
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_VIDEO_CTL)); BX++;
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_SWITCHES)); BX++;
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL)); BX++;
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_CURSOR_TYPE)); BX += 2;
        for(i=0;i<8;i++) {
            write_word(ES, BX, read_word(BIOSMEM_SEG, BIOSMEM_CURSOR_POS+2*i));
            BX += 2;
        }
        write_word(ES, BX, read_word(BIOSMEM_SEG,BIOSMEM_CURRENT_START)); BX += 2;
        write_byte(ES, BX, read_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE)); BX++;
        /* current font */
        write_word(ES, BX, read_word(0, 0x1f * 4)); BX += 2;
        write_word(ES, BX, read_word(0, 0x1f * 4 + 2)); BX += 2;
        write_word(ES, BX, read_word(0, 0x43 * 4)); BX += 2;
        write_word(ES, BX, read_word(0, 0x43 * 4 + 2)); BX += 2;
    }
    if (CX & 4) {
        /* XXX: check this */
        write_byte(ES, BX, inb(VGAREG_DAC_STATE)); BX++; /* read/write mode dac */
        write_byte(ES, BX, inb(VGAREG_DAC_WRITE_ADDRESS)); BX++; /* pix address */
        write_byte(ES, BX, inb(VGAREG_PEL_MASK)); BX++;
        // Set the whole dac always, from 0
        outb(VGAREG_DAC_WRITE_ADDRESS,0x00);
        for(i=0;i<256*3;i++) {
            write_byte(ES, BX, inb(VGAREG_DAC_DATA)); BX++;
        }
        write_byte(ES, BX, 0); BX++; /* color select register */
    }
    return BX;
}

uint16_t biosfn_restore_video_state(uint16_t CX, uint16_t ES, uint16_t BX)
{
    uint16_t i, crtc_addr, v, addr1, ar_index;

    if (CX & 1) {
        // Reset Attribute Ctl flip-flop
        inb(VGAREG_ACTL_RESET);

        crtc_addr = read_word(ES, BX + 0x40);
        addr1 = BX;
        BX += 5;

        for(i=1;i<=4;i++){
            outb(VGAREG_SEQU_ADDRESS, i);
            outb(VGAREG_SEQU_DATA, read_byte(ES, BX)); BX++;
        }
        outb(VGAREG_SEQU_ADDRESS, 0);
        outb(VGAREG_SEQU_DATA, read_byte(ES, BX)); BX++;

        // Disable CRTC write protection
        outw(crtc_addr,0x0011);
        // Set CRTC regs
        for(i=0;i<=0x18;i++) {
            if (i != 0x11) {
                outb(crtc_addr,i);
                outb(crtc_addr+1, read_byte(ES, BX));
            }
            BX++;
        }
        // select crtc base address
        v = inb(VGAREG_READ_MISC_OUTPUT) & ~0x01;
        if (crtc_addr == 0x3d4)
            v |= 0x01;
        outb(VGAREG_WRITE_MISC_OUTPUT, v);

        // enable write protection if needed
        outb(crtc_addr, 0x11);
        outb(crtc_addr+1, read_byte(ES, BX - 0x18 + 0x11));

        // Set Attribute Ctl
        ar_index = read_byte(ES, addr1 + 0x03);
        inb(VGAREG_ACTL_RESET);
        for(i=0;i<=0x13;i++) {
            outb(VGAREG_ACTL_ADDRESS, i | (ar_index & 0x20));
            outb(VGAREG_ACTL_WRITE_DATA, read_byte(ES, BX)); BX++;
        }
        outb(VGAREG_ACTL_ADDRESS, ar_index);
        inb(VGAREG_ACTL_RESET);

        for(i=0;i<=8;i++) {
            outb(VGAREG_GRDC_ADDRESS,i);
            outb(VGAREG_GRDC_DATA, read_byte(ES, BX)); BX++;
        }
        BX += 2; /* crtc_addr */
        BX += 4; /* plane latches */

        outb(VGAREG_SEQU_ADDRESS, read_byte(ES, addr1)); addr1++;
        outb(crtc_addr, read_byte(ES, addr1)); addr1++;
        outb(VGAREG_GRDC_ADDRESS, read_byte(ES, addr1)); addr1++;
        addr1++;
        outb(crtc_addr - 0x4 + 0xa, read_byte(ES, addr1)); addr1++;
    }
    if (CX & 2) {
        write_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_MODE, read_byte(ES, BX)); BX++;
        write_word(BIOSMEM_SEG,BIOSMEM_NB_COLS, read_word(ES, BX)); BX += 2;
        write_word(BIOSMEM_SEG,BIOSMEM_PAGE_SIZE, read_word(ES, BX)); BX += 2;
        write_word(BIOSMEM_SEG,BIOSMEM_CRTC_ADDRESS, read_word(ES, BX)); BX += 2;
        write_byte(BIOSMEM_SEG,BIOSMEM_NB_ROWS, read_byte(ES, BX)); BX++;
        write_word(BIOSMEM_SEG,BIOSMEM_CHAR_HEIGHT, read_word(ES, BX)); BX += 2;
        write_byte(BIOSMEM_SEG,BIOSMEM_VIDEO_CTL, read_byte(ES, BX)); BX++;
        write_byte(BIOSMEM_SEG,BIOSMEM_SWITCHES, read_byte(ES, BX)); BX++;
        write_byte(BIOSMEM_SEG,BIOSMEM_MODESET_CTL, read_byte(ES, BX)); BX++;
        write_word(BIOSMEM_SEG,BIOSMEM_CURSOR_TYPE, read_word(ES, BX)); BX += 2;
        for(i=0;i<8;i++) {
            write_word(BIOSMEM_SEG, BIOSMEM_CURSOR_POS+2*i, read_word(ES, BX));
            BX += 2;
        }
        write_word(BIOSMEM_SEG,BIOSMEM_CURRENT_START, read_word(ES, BX)); BX += 2;
        write_byte(BIOSMEM_SEG,BIOSMEM_CURRENT_PAGE, read_byte(ES, BX)); BX++;
        /* current font */
        write_word(0, 0x1f * 4, read_word(ES, BX)); BX += 2;
        write_word(0, 0x1f * 4 + 2, read_word(ES, BX)); BX += 2;
        write_word(0, 0x43 * 4, read_word(ES, BX)); BX += 2;
        write_word(0, 0x43 * 4 + 2, read_word(ES, BX)); BX += 2;
    }
    if (CX & 4) {
        BX++;
        v = read_byte(ES, BX); BX++;
        outb(VGAREG_PEL_MASK, read_byte(ES, BX)); BX++;
        // Set the whole dac always, from 0
        outb(VGAREG_DAC_WRITE_ADDRESS,0x00);
        for(i=0;i<256*3;i++) {
            outb(VGAREG_DAC_DATA, read_byte(ES, BX)); BX++;
        }
        BX++;
        outb(VGAREG_DAC_WRITE_ADDRESS, v);
    }
    return BX;
}

// ============================================================================================
//
// Video Utils
//
// ============================================================================================

// --------------------------------------------------------------------------------------------
static uint8_t find_vga_entry(uint8_t mode)
{
 uint8_t i,line=0xFF;
 for(i=0;i<=MODE_MAX;i++)
  if(vga_modes[i].svgamode==mode)
   {line=i;
    break;
   }
 return line;
}

/* =========================================================== */
/*
 * Misc Utils
*/
/* =========================================================== */

uint8_t read_byte(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint8_t *)offset) );
}

void write_byte(uint16_t seg, uint16_t offset, uint8_t data)
{
    *(seg:>(uint8_t *)offset) = data;
}

uint16_t read_word(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint16_t *)offset) );
}

void write_word(uint16_t seg, uint16_t offset, uint16_t data)
{
    *(seg:>(uint16_t *)offset) = data;
}

uint32_t read_dword(uint16_t seg, uint16_t offset)
{
    return( *(seg:>(uint32_t *)offset) );
}

void write_dword(uint16_t seg, uint16_t offset, uint32_t data)
{
    *(seg:>(uint32_t *)offset) = data;
}

#ifdef VGA_DEBUG
void __cdecl unimplemented()
{
 printf("--> Unimplemented\n");
}

void __cdecl unknown()
{
 printf("--> Unknown int10\n");
}

#undef VBE_PRINTF_PORT
#define VBE_PRINTF_PORT 0x504

// --------------------------------------------------------------------------------------------
void __cdecl printf(char *s, ...)
{
    char        c;
    Boolean     in_format;
    unsigned    format_width, i;
    uint16_t    arg, digit, nibble;
    uint16_t STACK_BASED *arg_ptr;

    arg_ptr = (uint16_t STACK_BASED *)&s;

    in_format    = 0;
    format_width = 0;

    while (c = *s) {
        if (c == '%') {
            in_format    = 1;
            format_width = 0;
        } else if (in_format) {
            if ((c >= '0') && (c <= '9')) {
                format_width = (format_width * 10) + (c - '0');
            } else if (c == 'x') {
                arg_ptr++; // increment to next arg
                arg = *arg_ptr;
                if (format_width == 0)
                    format_width = 4;
                i = 0;
                digit = format_width - 1;
                for (i = 0; i < format_width; i++) {
                    nibble = (arg >> (4 * digit)) & 0x000f;
                    if (nibble <= 9)
                        outb(VBE_PRINTF_PORT, nibble + '0');
                    else
                        outb(VBE_PRINTF_PORT, (nibble - 10) + 'A');
                    digit--;
                }
                in_format = 0;
            }
            //else if (c == 'd') {
            //  in_format = 0;
            //  }
        } else {
            outb(VBE_PRINTF_PORT, c);
        }
        ++s;
    }
}
#endif

/// @todo rearrange, call only from VBE module?
extern void vbe_biosfn_return_controller_information(uint16_t STACK_BASED *AX, uint16_t ES, uint16_t DI);
extern void vbe_biosfn_return_mode_information(uint16_t STACK_BASED *AX, uint16_t CX, uint16_t ES, uint16_t DI);
extern void vbe_biosfn_set_mode(uint16_t STACK_BASED *AX, uint16_t BX, uint16_t ES, uint16_t DI);
extern void vbe_biosfn_save_restore_state(uint16_t STACK_BASED *AX, uint16_t CX, uint16_t DX, uint16_t ES, uint16_t STACK_BASED *BX);
extern void vbe_biosfn_get_set_scanline_length(uint16_t STACK_BASED *AX, uint16_t STACK_BASED *BX, uint16_t STACK_BASED *CX, uint16_t STACK_BASED *DX);

// --------------------------------------------------------------------------------------------
/*
 * int10 main dispatcher
 */
void __cdecl int10_func(uint16_t DI, uint16_t SI, uint16_t BP, uint16_t SP, uint16_t BX,
                        uint16_t DX, uint16_t CX, uint16_t AX, uint16_t DS, uint16_t ES, uint16_t FLAGS)
{

 // BIOS functions
 switch(GET_AH())
  {
   case 0x00:
     biosfn_set_video_mode(GET_AL());
     switch(GET_AL()&0x7F)
      {case 6:
        SET_AL(0x3F);
        break;
       case 0:
       case 1:
       case 2:
       case 3:
       case 4:
       case 5:
       case 7:
        SET_AL(0x30);
        break;
      default:
        SET_AL(0x20);
      }
     break;
   case 0x01:
     biosfn_set_cursor_shape(GET_CH(),GET_CL());
     break;
   case 0x02:
     biosfn_set_cursor_pos(GET_BH(),DX);
     break;
   case 0x03:
     vga_get_cursor_pos(GET_BH(), &CX, &DX);
     break;
   case 0x04:
     // Read light pen pos (unimplemented)
#ifdef VGA_DEBUG
     unimplemented();
#endif
     AX=0x00;
     BX=0x00;
     CX=0x00;
     DX=0x00;
     break;
   case 0x05:
     biosfn_set_active_page(GET_AL());
     break;
   case 0x06:
     biosfn_scroll(GET_AL(),GET_BH(),GET_CH(),GET_CL(),GET_DH(),GET_DL(),0xFF,SCROLL_UP);
     break;
   case 0x07:
     biosfn_scroll(GET_AL(),GET_BH(),GET_CH(),GET_CL(),GET_DH(),GET_DL(),0xFF,SCROLL_DOWN);
     break;
   case 0x08:
     vga_read_char_attr(GET_BH(), &AX);
     break;
   case 0x09:
     biosfn_write_char_attr(GET_AL(),GET_BH(),GET_BL(),CX);
     break;
   case 0x0A:
     biosfn_write_char_only(GET_AL(),GET_BH(),GET_BL(),CX);
     break;
   case 0x0C:
     biosfn_write_pixel(GET_BH(),GET_AL(),CX,DX);
     break;
   case 0x0D:
     vga_read_pixel(GET_BH(), CX, DX, &AX);
     break;
   case 0x0E:
     // Ralf Brown Interrupt list is WRONG on bh(page)
     // We do output only on the current page !
#ifdef VGA_DEBUG
     printf("write_teletype %02x\n", GET_AL());
#endif

     biosfn_write_teletype(GET_AL(),0xff,GET_BL(),NO_ATTR);
     break;
   case 0x10:
     // All other functions of group AH=0x10 rewritten in assembler
     biosfn_perform_gray_scale_summing(BX,CX);
     break;
   case 0x11:
     switch(GET_AL())
      {
       case 0x00:
       case 0x10:
        biosfn_load_text_user_pat(GET_AL(),ES,BP,CX,DX,GET_BL(),GET_BH());
        break;
       case 0x01:
       case 0x11:
        biosfn_load_text_8_14_pat(GET_AL(),GET_BL());
        break;
       case 0x02:
       case 0x12:
        biosfn_load_text_8_8_pat(GET_AL(),GET_BL());
        break;
       case 0x04:
       case 0x14:
        biosfn_load_text_8_16_pat(GET_AL(),GET_BL());
        break;
       case 0x20:
        biosfn_load_gfx_8_8_chars(ES,BP);
        break;
       case 0x21:
        biosfn_load_gfx_user_chars(ES,BP,CX,GET_BL(),GET_DL());
        break;
       case 0x22:
        biosfn_load_gfx_8_14_chars(GET_BL());
        break;
       case 0x23:
        biosfn_load_gfx_8_8_dd_chars(GET_BL());
        break;
       case 0x24:
        biosfn_load_gfx_8_16_chars(GET_BL());
        break;
       case 0x30:
        vga_get_font_info(GET_BH(), &ES, &BP, &CX, &DX);
        break;
#ifdef VGA_DEBUG
       default:
        unknown();
#endif
      }

     break;
   case 0x12:
     switch(GET_BL())
      {
       case 0x20:
        biosfn_alternate_prtsc();
        break;
       case 0x35:
        biosfn_switch_video_interface(GET_AL(),ES,DX);
        SET_AL(0x12);
        break;
       case 0x36:
        biosfn_enable_video_refresh_control(GET_AL());
        SET_AL(0x12);
        break;
#ifdef VGA_DEBUG
       default:
        unknown();
#endif
      }
     break;
   case 0x13:
     biosfn_write_string(GET_AL(),GET_BH(),GET_BL(),CX,GET_DH(),GET_DL(),ES,BP);
     break;
   case 0x1B:
     biosfn_read_state_info(BX,ES,DI);
     SET_AL(0x1B);
     break;
   case 0x1C:
     switch(GET_AL())
      {
       case 0x00:
        vga_get_video_state_size(CX,&BX);
        break;
       case 0x01:
        biosfn_save_video_state(CX,ES,BX);
        break;
       case 0x02:
        biosfn_restore_video_state(CX,ES,BX);
        break;
#ifdef VGA_DEBUG
       default:
        unknown();
#endif
      }
     SET_AL(0x1C);
     break;

#ifdef VBE
   case 0x4f:
     if (vbe_has_vbe_display()) {
       switch(GET_AL())
       {
         case 0x00:
          vbe_biosfn_return_controller_information(&AX,ES,DI);
          break;
         case 0x01:
          vbe_biosfn_return_mode_information(&AX,CX,ES,DI);
          break;
         case 0x02:
          vbe_biosfn_set_mode(&AX,BX,ES,DI);
          break;
         case 0x04:
          vbe_biosfn_save_restore_state(&AX, CX, DX, ES, &BX);
          break;
         case 0x06:
          vbe_biosfn_get_set_scanline_length(&AX, &BX, &CX, &DX);
          break;
         case 0x09:
          //FIXME
#ifdef VGA_DEBUG
          unimplemented();
#endif
          // function failed
          AX=0x100;
          break;
         case 0x0A:
          //FIXME
#ifdef VGA_DEBUG
          unimplemented();
#endif
          // function failed
          AX=0x100;
          break;
         default:
#ifdef VGA_DEBUG
          unknown();
#endif
          // function failed
          AX=0x100;
          }
        }
        else {
          // No VBE display
          AX=0x0100;
          }
        break;
#endif

#ifdef VGA_DEBUG
   default:
     unknown();
#endif
  }
}

#ifdef VBE
//#include "vbe.c"
#endif

#ifdef CIRRUS
#include "clext.c"
#endif

// --------------------------------------------------------------------------------------------

