/* fb.c -- Simple frame buffer
   Copyright (C) 2001 Marko Mlinar, markom@opencores.org

This file is part of OpenRISC 1000 Architectural Simulator.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. */

#include <stdio.h>
#include <string.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "fb.h"
#include "sched.h"

#define FB_WRAP (512*1024)

struct fb_state {
  int enabled;
  unsigned long pal[256];
  int ctrl;
  int pic;
  int in_refresh;
  int refresh_count;
  oraddr_t addr;
  oraddr_t cam_addr;
  int camerax;
  int cameray;
  int camera_pos;
  oraddr_t baseaddr;
  int refresh;
  int refresh_rate;
  char *filename;
};

static void change_buf_addr (struct fb_state *fb, oraddr_t addr)
{
  fb->addr = addr;
}

/* Write a register */
void fb_write32 (oraddr_t addr, uint32_t value, void *dat)
{
  struct fb_state *fb = dat;

  switch (addr) {
    case FB_CTRL:    fb->ctrl = value; break;
    case FB_BUFADDR: change_buf_addr (fb, value); break;
    case FB_CAMBUFADDR: fb->cam_addr = value; break;
    case FB_CAMPOSADDR: fb->camera_pos = value;
		     fb->camerax = value % FB_SIZEX;
		     fb->cameray = value / FB_SIZEX;
		     break;
    default:
      addr -= FB_PAL;
      addr /= 4;
      if (addr < 0 || addr >= 256) {
        fprintf (stderr, "Write out of palette buffer (0x%"PRIxADDR")!\n", addr);
      } else fb->pal[addr] = value;
      break;
  } 
}

/* Read a register */
oraddr_t fb_read32 (oraddr_t addr, void *dat)
{
  struct fb_state *fb = dat;

  switch (addr) {
    case FB_CTRL:
      return (fb->ctrl & ~0xff000000) | (fb->in_refresh ? 0x80000000 : 0) | ((unsigned long)(fb->refresh_count & 0x7f) << 24);
      break;
    case FB_BUFADDR: return fb->addr; break;
    case FB_CAMBUFADDR: return fb->cam_addr; break;
    case FB_CAMPOSADDR: return fb->camera_pos; break;
    default:
      addr -= FB_PAL;
      addr /= 4;
      if (addr < 0 || addr >= 256) {
        fprintf (stderr, "Read out of palette buffer (0x%"PRIxADDR")!\n", addr);
        return 0;
      } else return fb->pal[addr];
  }
}

/* define these also for big endian */
#if __BIG_ENDIAN__
#define CNV32(x) (\
     ((((x) >> 24) & 0xff) << 0)\
   | ((((x) >> 16) & 0xff) << 8)\
   | ((((x) >> 8) & 0xff) << 16)\
   | ((((x) >> 0) & 0xff) << 24))
#define CNV16(x) (\
     ((((x) >> 8) & 0xff) << 0)\
   | ((((x) >> 0) & 0xff) << 8))
#else
#define CNV16(x) (x)
#define CNV32(x) (x)
#endif

/* Dumps a bmp file, based on current image */
static int fb_dump_image8 (struct fb_state *fb, char *filename)
{
  int sx = FB_SIZEX;
  int sy = FB_SIZEY;
  int i, x = 0, y = 0;
  FILE *fo;

  unsigned short int u16;
  unsigned long int u32;
  
  if (config.sim.verbose) PRINTF ("Creating %s...", filename);
  fo = fopen (filename, "wb+");
  u16 = CNV16(19778); /* BM */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u32 = CNV32(14 + 40 + sx * sy + 1024); /* size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0); /* reserved */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = 14 + 40 + 1024; /* offset */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  
  u32 = CNV32(40); /* header size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(sx); /* width */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(sy); /* height */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u16 = CNV16(1);  /* planes */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u16 = CNV16(8);  /* bits */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u32 = CNV32(0);  /* compression */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(x * y); /* image size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(2835);  /* x resolution */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(2835);  /* y resolution */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0);  /* ncolours = 0; should be generated */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0);  /* important colours; all are important */
  if (!fwrite (&u32, 4, 1, fo)) return 1;

  for (i = 0; i < 256; i++) {
    unsigned long val, d;
    d = fb->pal[i];
#if 1
    val = ((d >> 0) & 0x1f) << 3;   /* Blue */
    val |= ((d >> 5) & 0x3f) << 10;  /* Green */
    val |= ((d >> 11) & 0x1f) << 19; /* Red */
#else 
    val = CNV32(pal[i]);
#endif
    if (!fwrite (&val, 4, 1, fo)) return 1;
  }

  if (config.sim.verbose) PRINTF ("(%i,%i)", sx, sy);
  /* Data is stored upside down */
  for (y = sy - 1; y >= 0; y--) {
    int align = (4 - sx) % 4;
    int zero = CNV32(0);
    int add;
    while (align < 0) align += 4;
    for (x = 0; x < sx; x++) {
      add = (fb->addr & ~(FB_WRAP - 1)) | ((fb->addr + y * sx + x) & (FB_WRAP - 1));
      fputc (eval_direct8 (add, 0, 0), fo);
    }
    if (align && !fwrite (&zero, align, 1, fo)) return 1;
  }

  if (config.sim.verbose) PRINTF ("DONE\n");  
  fclose (fo);
  return 0;
}

/* Dumps a bmp file, based on current image */
static int fb_dump_image24 (struct fb_state *fb, char *filename)
{
  int sx = FB_SIZEX;
  int sy = FB_SIZEY;
  int x = 0, y = 0;
  FILE *fo;

  unsigned short int u16;
  unsigned long int u32;

  if (config.sim.verbose) PRINTF ("Creating %s...", filename);
  fo = fopen (filename, "wb+");
  u16 = CNV16(19778); /* BM */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u32 = CNV32(14 + 40 + sx * sy * 3); /* size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0); /* reserved */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = 14 + 40; /* offset */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  
  u32 = CNV32(40); /* header size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(sx); /* width */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(sy); /* height */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u16 = CNV16(1);  /* planes */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u16 = CNV16(24);  /* bits */
  if (!fwrite (&u16, 2, 1, fo)) return 1;
  u32 = CNV32(0);  /* compression */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(x * y * 3); /* image size */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(2835);  /* x resolution */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(2835);  /* y resolution */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0);  /* ncolours = 0; should be generated */
  if (!fwrite (&u32, 4, 1, fo)) return 1;
  u32 = CNV32(0);  /* important colours; all are important */
  if (!fwrite (&u32, 4, 1, fo)) return 1;

  if (config.sim.verbose) PRINTF ("(%i,%i)", sx, sy);
  /* Data is stored upside down */
  for (y = sy - 1; y >= 0; y--) {
    unsigned char line[FB_SIZEX][3];
    for (x = 0; x < sx; x++) 
      if (y >= fb->cameray && x >= fb->camerax && y < fb->cameray + CAM_SIZEY && x < fb->camerax + CAM_SIZEX) {
        int add = (fb->cam_addr + (x - fb->camerax + (y - fb->cameray) * CAM_SIZEX) * 2) ^ 2;
	unsigned short d = eval_direct16 (add, 0, 0); 
        line[x][0] = ((d >> 0) & 0x1f) << 3;  /* Blue */
        line[x][1] = ((d >> 5) & 0x3f) << 2;  /* Green */
        line[x][2] = ((d >> 11) & 0x1f) << 3; /* Red */
      } else {
        int add = (fb->addr & ~(FB_WRAP - 1)) | ((fb->addr + y * sx + x) & (FB_WRAP - 1));
        unsigned short d = fb->pal[eval_direct8 (add, 0, 0)];
        line[x][0] = ((d >> 0) & 0x1f) << 3;  /* Blue */
        line[x][1] = ((d >> 5) & 0x3f) << 2;  /* Green */
        line[x][2] = ((d >> 11) & 0x1f) << 3; /* Red */
      }
    if(!fwrite (line, sizeof(line), 1, fo)) return 1;
  }

  if (config.sim.verbose) PRINTF ("DONE\n");  
  fclose (fo);
  return 0;
}

void fb_job (void *dat)
{
  struct fb_state *fb = dat;

  if (fb->refresh) {
    /* dump the image? */
    if (fb->ctrl & 1) {
      char temp[STR_SIZE];
      sprintf (temp, "%s%04i.bmp", fb->filename, fb->pic);
      if (fb->ctrl & 2) fb_dump_image24 (fb, temp);
      else fb_dump_image8 (fb, temp);
      fb->pic++;
    }
    SCHED_ADD(fb_job, dat, fb->refresh_rate / REFRESH_DIVIDER);
    fb->in_refresh = 0;
    fb->refresh = 0;
  } else {
    fb->refresh_count++;
    fb->refresh = 1;
    SCHED_ADD(fb_job, dat, fb->refresh_rate / REFRESH_DIVIDER); 
  }
}

/* Reset all FBs */
void fb_reset (void *dat)
{
  struct fb_state *fb = dat;
  int i;

  fb->pic = 0;
  fb->addr = 0;
  fb->ctrl = 0;

  for (i = 0; i < 256; i++)
    fb->pal[i] = (i << 16) | (i << 8) | (i << 0);

  SCHED_ADD(fb_job, dat, fb->refresh_rate);
  fb->refresh = 0;
}

/*-----------------------------------------------------[ FB configuration ]---*/
void fb_baseaddr(union param_val val, void *dat)
{
  struct fb_state *fb = dat;
  fb->baseaddr = val.addr_val;
}

void fb_refresh_rate(union param_val val, void *dat)
{
  struct fb_state *fb = dat;
  fb->refresh_rate = val.int_val;
}

void fb_filename(union param_val val, void *dat)
{
  struct fb_state *fb = dat;
  if(!(fb->filename = strdup(val.str_val))) {
    fprintf(stderr, "Peripheral FB: Run out of memory\n");
    exit(-1);
  }
}

void fb_enabled(union param_val val, void *dat)
{
  struct fb_state *fb = dat;
  fb->enabled = val.int_val;
}

void *fb_sec_start(void)
{
  struct fb_state *new = malloc(sizeof(struct fb_state));

  if(!new) {
    fprintf(stderr, "Peripheral FB: Run out of memory\n");
    exit(-1);
  }

  new->baseaddr = 0;
  new->ctrl = 0;
  new->pic = 0;
  new->in_refresh = 1;
  new->refresh_count = 0;
  new->addr = 0;
  new->cam_addr = 0;
  new->camerax = 0;
  new->cameray = 0;
  new->camera_pos = 0;
  new->enabled = 1;

  return new;
}

void fb_sec_end(void *dat)
{
  struct fb_state *fb = dat;
  struct mem_ops ops;

  if(!fb->enabled) {
    free(dat);
    return;
  }

  memset(&ops, 0, sizeof(struct mem_ops));

  ops.readfunc32 = fb_read32;
  ops.writefunc32 = fb_write32;
  ops.write_dat32 = dat;
  ops.read_dat32 = dat;

  /* FIXME: Correct delay? */
  ops.delayr = 2;
  ops.delayw = 2;

  reg_mem_area(fb->baseaddr, FB_PAL + 256*4, 0, &ops);

  reg_sim_reset(fb_reset, dat);
}

void reg_fb_sec(void)
{
  struct config_section *sec = reg_config_sec("fb", fb_sec_start, fb_sec_end);

  reg_config_param(sec, "baseaddr", paramt_addr, fb_baseaddr);
  reg_config_param(sec, "enabled", paramt_int, fb_enabled);
  reg_config_param(sec, "refresh_rate", paramt_int, fb_refresh_rate);
  reg_config_param(sec, "filename", paramt_str, fb_filename);
}
