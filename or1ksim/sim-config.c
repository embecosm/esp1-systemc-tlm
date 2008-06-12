/* sim-config.c -- Simulator configuration
   Copyright (C) 1999 Damjan Lampret, lampret@opencores.org

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

/* Simulator configuration. Eventually this one will be a lot bigger. */

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "port.h"
#include "arch.h"
#include "sim-config.h"
#include "abstract.h"
#include "opcode/or32.h"
#include "spr_defs.h"
#include "execute.h"
#include "sprs.h"
#include "pic.h"
#include "stats.h"
#include "icache_model.h"
#include "dcache_model.h"

#include "profiler.h"
#include "mprofiler.h"
#include "cuc.h"

#include "debug.h"

DEFAULT_DEBUG_CHANNEL(config);

#define WARNING(s) fprintf (stderr, "WARNING: config.%s: %s\n", cur_section->name, (s))
#define MERROR(s) {fprintf (stderr, "ERROR: %s\n", s); if (runtime.sim.init) exit (1);}

struct config config;
struct runtime runtime;

struct config_section *cur_section;

struct config_section *sections = NULL;

extern char *or1ksim_ver;

void init_defconfig(void)
{
  memset(&config, 0, sizeof(config));
  /* Sim */
  config.sim.exe_log = 0;
  config.sim.exe_log_type = EXE_LOG_HARDWARE;
  config.sim.exe_log_start = 0;
  config.sim.exe_log_end = 0;
  config.sim.exe_log_marker = 0;
  strcpy (config.sim.exe_log_fn, "executed.log");
  config.sim.profile = 0;
  config.sim.mprofile = 0;

  config.sim.debug = 0;
  config.sim.verbose = 1;
  
  strcpy (config.sim.prof_fn, "sim.profile");
  strcpy (config.sim.mprof_fn, "sim.mprofile");
  strcpy (config.sim.fstdout, "stdout.txt");
  strcpy (runtime.sim.script_fn, "(default)");
  config.sim.clkcycle_ps = 4000; /* 4000 for 4ns (250MHz) */
  if (config.sim.clkcycle_ps) config.sim.system_kfreq = (long)((1000000000.0 / (double)config.sim.clkcycle_ps));
  else config.sim.system_kfreq = INT_MAX;
  if (config.sim.system_kfreq <= 0) config.sim.system_kfreq = 1;
  
  /* IMMU & DMMU*/
  config.immu.enabled = 0;
  config.immu.hitdelay = 1;
  config.immu.missdelay = 1;
  config.immu.pagesize = 8192;
  config.dmmu.enabled = 0;
  config.dmmu.hitdelay = 1;
  config.dmmu.missdelay = 1;
  config.dmmu.pagesize = 8192;
  
  /* IC & DC */
  config.ic.enabled = 0;
  config.ic.hitdelay = 1;
  config.ic.missdelay = 1;
  config.ic.nways = 0;
  config.ic.nsets = 0;
  config.ic.ustates = 0;
  config.dc.enabled = 0;
  config.dc.load_hitdelay = 2;
  config.dc.load_missdelay = 2;
  config.dc.nways = 0;
  config.dc.nsets = 0;
  config.dc.ustates = 0;
  config.dc.store_hitdelay = 0;
  config.dc.store_missdelay = 0;

  /* CPU */
  config.cpu.superscalar = 0;
  config.sim.history = 0;
  config.cpu.hazards = 0;
  config.cpu.dependstats = 0;
  config.cpu.sbuf_len = 0;
  config.cpu.upr = SPR_UPR_UP | SPR_UPR_DCP | SPR_UPR_ICP | SPR_UPR_DMP
                 | SPR_UPR_IMP | SPR_UPR_OB32P | SPR_UPR_DUP | SPR_UPR_PICP
                 | SPR_UPR_PMP | SPR_UPR_TTP;
  config.cpu.sr = 0x00008001;

  /* Debug */
  config.debug.enabled = 0;
  config.debug.gdb_enabled = 0;
  config.debug.server_port = 0;
  
  /* VAPI */
  config.vapi.enabled = 0;
  strcpy (config.vapi.vapi_fn, "vapi.log");
  
  /* PM */
  config.pm.enabled = 0;

  /* CUC */
  strcpy (config.cuc.timings_fn, "virtex.tim");
  config.cuc.memory_order = MO_STRONG;
  config.cuc.calling_convention = 1;
  config.cuc.enable_bursts = 1;
  config.cuc.no_multicycle = 1;

  /* Configure runtime */
  memset(&runtime, 0, sizeof(runtime));
  
  /* Sim */
  runtime.sim.fexe_log = NULL;
  runtime.sim.iprompt = 0;
  runtime.sim.fprof = NULL;
  runtime.sim.fmprof = NULL;
  runtime.sim.init = 1;
  runtime.sim.fout = stdout;
  runtime.sim.script_file_specified = 0;
  runtime.simcmd.profile = 0;
  runtime.simcmd.mprofile = 0;

  /* VAPI */
  runtime.vapi.vapi_file = NULL;
  runtime.vapi.enabled = 0;
}

static void version(void)
{
  PRINTF ("\n");
  PRINTF ("OpenRISC 1000 (OR32) Architectural Simulator, version %s\n",
          or1ksim_ver);
  PRINTF ("Copyright (C) 1999 Damjan Lampret, lampret@opencores.org\n");
  PRINTF ("Copyright (C) 2000 Damjan Lampret, lampret@opencores.org\n");
  PRINTF ("                   Jimmy Chen-Min Chen, jimmy@ee.nctu.edu.tw\n");
  PRINTF ("                   Johan Rydberg, johan.rydberg@insight.se\n");
  PRINTF ("                   Marko Mlinar, markom@opencores.org\n");
  PRINTF ("Copyright (C) 2001 Simon Srot, simons@opencores.org\n");
  PRINTF ("                   Marko Mlinar, markom@opencores.org\n");
  PRINTF ("Copyright (C) 2002 Marko Mlinar, markom@opencores.org\n");
  PRINTF ("                   Simon Srot, simons@opencores.org\n");
  PRINTF ("Visit http://www.opencores.org for more information about ");
  PRINTF ("OpenRISC 1000 and\nother open source cores.\n\n");
  PRINTF ("This software comes with ABSOLUTELY NO WARRANTY; for ");
  PRINTF ("details see COPYING.\nThis is free software, and you ");
  PRINTF ("are welcome to redistribute it under certain\nconditions; ");
  PRINTF ("for details see COPYING.\n");
}

int parse_args(int argc, char *argv[])
{
  argv++; argc--;
  while (argc) {
    if (strcmp(*argv, "profiler") == 0) {
      exit (main_profiler (argc, argv));
    } else
    if (strcmp(*argv, "mprofiler") == 0) {
      exit (main_mprofiler (argc, argv));
    } else
    if (*argv[0] != '-') {
      runtime.sim.filename = argv[0];
      argc--;
      argv++;
    } else
    if (strcmp(*argv, "-f") == 0 || strcmp(*argv, "--file") == 0) {      
      argv++; argc--;
      if (argv[0]) {
        read_script_file(argv[0]);
        argv++; argc--;
      } else {
        fprintf(stderr, "Configure filename not specified!\n");
        return 1;
      }
    } else
    if (strcmp(*argv, "--nosrv") == 0) {  /* (CZ) */
      config.debug.gdb_enabled = 0;
      argv++; argc--;
    } else
    if (strcmp(*argv, "--srv") == 0) {  /* (CZ) */
      char *s;
      if(!--argc)
        return 1;
      config.debug.enabled = 1;
      config.debug.gdb_enabled = 1;
      config.debug.server_port = strtol(*(++argv),&s,10);
      if(*s)
        return 1;
      argv++; argc--;
    } else
    if (strcmp(*argv, "-i") == 0) {
      runtime.sim.iprompt = 1;
      argv++; argc--;
    } else
    if (strcmp(*argv, "-v") == 0) {
      version();
      exit(0);
    } else
    if (strcmp(*argv, "--enable-profile") == 0) {
      runtime.simcmd.profile = 1;
      argv++; argc--;
    } else
    if (strcmp(*argv, "--enable-mprofile") == 0) {
      runtime.simcmd.mprofile = 1;
      argv++; argc--;
    } else
    if (strcmp(*argv, "-d") == 0) {
      parse_dbchs(*(++argv));
      argv++; argc -= 2;
    } else {
      fprintf(stderr, "Unknown option: %s\n", *argv);
      return 1;
    }
  }
  
  if (!argc)
    return 0;
          
  return 0;
}

void print_config(void)
{
  if (config.sim.verbose) {
    char temp[20];
    PRINTF("Verbose on, ");
    if (config.sim.debug)
      PRINTF("simdebug on, ");
    else
      PRINTF("simdebug off, ");
    if (runtime.sim.iprompt)
      PRINTF("interactive prompt on\n");
    else
      PRINTF("interactive prompt off\n");
      
    PRINTF("Machine initialization...\n");
    generate_time_pretty (temp, config.sim.clkcycle_ps);
    PRINTF("Clock cycle: %s\n", temp);
    if (cpu_state.sprs[SPR_UPR] & SPR_UPR_DCP)
      PRINTF("Data cache present.\n");
    else
      PRINTF("No data cache.\n");
    if (cpu_state.sprs[SPR_UPR] & SPR_UPR_ICP)
      PRINTF("Insn cache tag present.\n");
    else
      PRINTF("No instruction cache.\n");
    if (config.bpb.enabled)
      PRINTF("BPB simulation on.\n");
    else
      PRINTF("BPB simulation off.\n");
    if (config.bpb.btic)
      PRINTF("BTIC simulation on.\n");
    else
      PRINTF("BTIC simulation off.\n");
  }
}

struct config_param {
  char *name;
  enum param_t type;
  void (*func)(union param_val, void *dat);
  struct config_param *next;
};

void base_include (union param_val val, void *dat) {
  read_script_file (val.str_val);
  cur_section = NULL;
}

/*----------------------------------------------[ Simulator configuration ]---*/
void sim_debug (union param_val val, void *dat) {
  config.sim.debug = val.int_val;
}

void sim_verbose (union param_val val, void *dat) {
  config.sim.verbose = val.int_val;
}

void sim_profile (union param_val val, void *dat) {
  config.sim.profile = val.int_val;
}

void sim_prof_fn (union param_val val, void *dat) {
  strcpy(config.sim.prof_fn, val.str_val);
}

void sim_mprofile (union param_val val, void *dat) {
  config.sim.mprofile = val.int_val;
}

void sim_mprof_fn (union param_val val, void *dat) {
  strcpy(config.sim.mprof_fn, val.str_val);
}

void sim_history (union param_val val, void *dat) {
  config.sim.history = val.int_val;
}

void sim_exe_log (union param_val val, void *dat) {
  config.sim.exe_log = val.int_val;
}

void sim_exe_log_type (union param_val val, void *dat) {
  if (strcmp (val.str_val, "default") == 0)
    config.sim.exe_log_type = EXE_LOG_HARDWARE;
  else if (strcmp (val.str_val, "hardware") == 0)
    config.sim.exe_log_type = EXE_LOG_HARDWARE;
  else if (strcmp (val.str_val, "simple") == 0)
    config.sim.exe_log_type = EXE_LOG_SIMPLE;
  else if (strcmp (val.str_val, "software") == 0) {
    config.sim.exe_log_type = EXE_LOG_SOFTWARE;
  } else {
    char tmp[200];
    sprintf (tmp, "invalid execution log type '%s'.\n", val.str_val);
    CONFIG_ERROR(tmp);
  }
}

void sim_exe_log_start (union param_val val, void *dat) {
  config.sim.exe_log_start = val.longlong_val;
}

void sim_exe_log_end (union param_val val, void *dat) {
  config.sim.exe_log_end = val.longlong_val;
}

void sim_exe_log_marker (union param_val val, void *dat) {
  config.sim.exe_log_marker = val.int_val;
}

void sim_exe_log_fn (union param_val val, void *dat) {
  strcpy(config.sim.exe_log_fn, val.str_val);
}

void sim_clkcycle (union param_val val, void *dat) {
  int len = strlen (val.str_val);
  int pos = len - 1;
  long time;
  if (len < 2) goto err;
  if (val.str_val[pos--] != 's') goto err;
  switch (val.str_val[pos--]) {
    case 'p': time = 1; break;
    case 'n': time = 1000; break;
    case 'u': time = 1000000; break;
    case 'm': time = 1000000000; break;
  default:
    goto err;
  }
  val.str_val[pos + 1] = 0;
  config.sim.clkcycle_ps = time * atol (val.str_val);
  return;
err:
  CONFIG_ERROR("invalid time format.");
}

void sim_stdout (union param_val val, void *dat) {
  strcpy(config.sim.fstdout, val.str_val);
}

void reg_sim_sec (void) {
  struct config_section *sec = reg_config_sec("sim", NULL, NULL);

  reg_config_param(sec, "debug", paramt_int, sim_debug);
  reg_config_param(sec, "verbose", paramt_int, sim_verbose);
  reg_config_param(sec, "profile", paramt_int, sim_profile);
  reg_config_param(sec, "prof_fn", paramt_str, sim_prof_fn);
  reg_config_param(sec, "mprofile", paramt_int, sim_mprofile);
  reg_config_param(sec, "mprof_fn", paramt_str, sim_mprof_fn);
  reg_config_param(sec, "history", paramt_int, sim_history);
  reg_config_param(sec, "exe_log", paramt_int, sim_exe_log);
  reg_config_param(sec, "exe_log_type", paramt_word, sim_exe_log_type);
  reg_config_param(sec, "exe_log_start", paramt_longlong, sim_exe_log_start);
  reg_config_param(sec, "exe_log_end", paramt_longlong, sim_exe_log_end);
  reg_config_param(sec, "exe_log_marker", paramt_int, sim_exe_log_marker);
  reg_config_param(sec, "exe_log_fn", paramt_str, sim_exe_log_fn);
  reg_config_param(sec, "clkcycle", paramt_word, sim_clkcycle);
  reg_config_param(sec, "stdout", paramt_str, sim_stdout);
}

/*----------------------------------------------------[ CPU configuration ]---*/
void cpu_ver (union param_val val, void *dat) {
  config.cpu.ver = val.int_val;
}

void cpu_rev (union param_val val, void *dat) {
  config.cpu.rev = val.int_val;
}

void cpu_upr (union param_val val, void *dat) {
  config.cpu.upr = val.int_val;
}

void cpu_sr (union param_val val, void *dat) {
  config.cpu.sr = val.int_val;
}

void cpu_hazards (union param_val val, void *dat) {
  config.cpu.hazards = val.int_val;
}

void cpu_superscalar (union param_val val, void *dat) {
  config.cpu.superscalar = val.int_val;
}

void cpu_dependstats (union param_val val, void *dat) {
  config.cpu.dependstats = val.int_val;
}

void cpu_sbuf_len (union param_val val, void *dat) {
  if (val.int_val >= MAX_SBUF_LEN) {
    config.cpu.sbuf_len = MAX_SBUF_LEN - 1;
    WARNING("sbuf_len too large; truncated.");
  } else if (val.int_val < 0) {
    config.cpu.sbuf_len = 0;
    WARNING("sbuf_len negative; disabled.");
  } else
    config.cpu.sbuf_len = val.int_val;
}

void reg_cpu_sec(void)
{
  struct config_section *sec = reg_config_sec("cpu", NULL, NULL);

  reg_config_param(sec, "ver", paramt_int, cpu_ver);
  reg_config_param(sec, "rev", paramt_int, cpu_rev);
  reg_config_param(sec, "upr", paramt_int, cpu_upr);
  reg_config_param(sec, "sr", paramt_int, cpu_sr);
  reg_config_param(sec, "hazards", paramt_int, cpu_hazards);
  reg_config_param(sec, "superscalar", paramt_int, cpu_superscalar);
  reg_config_param(sec, "dependstats", paramt_int, cpu_dependstats);
  reg_config_param(sec, "sbuf_len", paramt_int, cpu_sbuf_len);
}

void reg_config_secs(void)
{
  reg_config_param(reg_config_sec("base", NULL, NULL), "include", paramt_str,
                   base_include);

  reg_generic_sec();			/* JPB */
  reg_sim_sec();
  reg_cpu_sec();
  reg_memory_sec();
  reg_mc_sec();
  reg_uart_sec();
  reg_dma_sec();
  reg_debug_sec();
  reg_vapi_sec();
  reg_ethernet_sec();
  reg_immu_sec();
  reg_dmmu_sec();
  reg_ic_sec();
  reg_dc_sec();
  reg_gpio_sec();
  reg_bpb_sec();
  reg_pm_sec();
  reg_vga_sec();
  reg_fb_sec();
  reg_kbd_sec();
  reg_ata_sec();
  reg_cuc_sec();
}

/* Returns a user friendly string of type */
static char *get_paramt_str(enum param_t type)
{
  switch(type) {
  case paramt_int:
    return "integer";
  case paramt_longlong:
    return "longlong";
  case paramt_addr:
    return "address";
  case paramt_str:
    return "string";
  case paramt_word:
    return "word";
  case paramt_none:
    return "none";
  }
  return "";
}

void reg_config_param(struct config_section *sec, const char *param,
                      enum param_t type,
                      void (*param_cb)(union param_val, void *))
{
  struct config_param *new = malloc(sizeof(struct config_param));

  TRACE("Registering config param `%s' to section `%s', type %s\n", param,
        sec->name, get_paramt_str(type));

  if(!new) {
    fprintf(stderr, "Out-of-memory\n");
    exit(1);
  }

  if(!(new->name = strdup(param))) {
    fprintf(stderr, "Out-of-memory\n");
    exit(1);
  }

  new->func = param_cb;
  new->type = type;

  new->next = sec->params;
  sec->params = new;
}

struct config_section *reg_config_sec(const char *section,
                                      void *(*sec_start)(void),
                                      void (*sec_end)(void *))
{
  struct config_section *new = malloc(sizeof(struct config_section));

  TRACE("Registering config section `%s'\n", section);

  if(!new) {
    fprintf(stderr, "Out-of-memory\n");
    exit(1);
  }

  if(!(new->name = strdup(section))) {
    fprintf(stderr, "Out-of-memory\n");
    exit(1);
  }

  new->next = sections;
  new->sec_start = sec_start;
  new->sec_end = sec_end;
  new->params = NULL;

  sections = new;

  return new;
}

static void switch_param(char *param, struct config_param *cur_param)
{
  char *end_p;
  union param_val val;

    /* Skip over an = sign if it exists */
  if(*param == '=') {
    param++;
    while(*param && isspace(*param)) param++;
  }

  switch (cur_param->type) {
  case paramt_int:
    val.int_val = strtol(param, NULL, 0);
    break;
  case paramt_longlong:
    val.longlong_val = strtoll(param, NULL, 0);
    break;
  case paramt_addr:
    val.addr_val = strtoul(param, NULL, 0);
    break;
  case paramt_str:
    if(*param != '"') {
      CONFIG_ERROR("String value expected\n");
      return;
    }

    param++;
    end_p = param;
    while(*end_p && (*end_p != '"')) end_p++; 
    *end_p = '\0';
    val.str_val = param;
    break;
  case paramt_word:
    end_p = param;
    while(*end_p && !isspace(*end_p)) end_p++; 
    *end_p = '\0';
    val.str_val = param;
    break;
  case paramt_none:
    break;
  }

  cur_param->func(val, cur_section->dat);
}

/* Read environment from a script file. Does not fail - assumes default configuration instead.
   The syntax of script file is:
   param = value
   section x
     data
     param = value
   end
   
   Example:
   section mc
     memory_table_file = sim.mem
     enable = 1
     POC = 0x47892344
   end
   
 */
 
void read_script_file (char *filename)
{  
  FILE *f;
  char *home = getenv("HOME");
  char ctmp[STR_SIZE];
  int local = 1;
  cur_section = NULL;
  
  sprintf(ctmp, "%s/.or1k/%s", home, filename);
  if ((f = fopen (filename, "rt")) || (home && (f = fopen (ctmp, "rt")))) {
    if (config.sim.verbose)
      PRINTF ("Reading script file from '%s'...\n", local ? filename : ctmp);
    strcpy (runtime.sim.script_fn, local ? filename : ctmp);

    while (!feof(f)) {
      char param[STR_SIZE];
      if (fscanf(f, "%s ", param) != 1) break;
      /* Is this a section? */
      if (strcmp (param, "section") == 0) {
        struct config_section *cur;
        cur_section = NULL;
        if (fscanf (f, "%s\n", param) != 1) {
          fprintf (stderr, "%s: ERROR: Section name required.\n", local ? filename : ctmp);
          exit (1);
        }
        TRACE("Came across section `%s'\n", param);
        for (cur = sections; cur; cur = cur->next)
          if (strcmp (cur->name, param) == 0) {
            cur_section = cur;
            break;
          }
        if (!cur) {
          fprintf (stderr, "WARNING: config: Unknown section: %s; ignoring.\n",
                   param);
          /* just skip section */
          while (fscanf (f, "%s\n", param) == 1 && strcmp (param, "end"));
        } else {
          cur->dat = NULL;
          if (cur->sec_start)
            cur->dat = cur->sec_start();
        }
      } else if (strcmp (param, "end") == 0) {
        if(cur_section->sec_end)
          cur_section->sec_end(cur_section->dat);
        cur_section = NULL;
      } else if (strncmp (param, "/*", 2) == 0) {
        char c0 = 0, c1 = 0;
        while (c0 != '*' || c1 != '/') {
          c0 = c1;
          c1 = fgetc(f);
          if (feof(f)) {
            fprintf (stderr, "%s: ERROR: Comment reached EOF.\n", local ? filename : ctmp);
            exit (1);
          }
        }
      } else {
        struct config_param *cur_param;
        char *cur_p;
        TRACE("Came across parameter `%s' in section `%s'\n", param,
              cur_section->name);
        for (cur_param = cur_section->params; cur_param; cur_param = cur_param->next)
          if (strcmp (cur_param->name, param) == 0) {
            break;
          }
        if (!cur_param) {
          char tmp[200];
          sprintf (tmp, "Invalid parameter: %s; ignoring.\n", param);
          WARNING(tmp);
          while (fgetc(f) != '\n' || feof(f));
          continue;
        }

        if(cur_param->type == paramt_none)
          continue;

        /* Parse parameter value */
        cur_p = fgets (param, STR_SIZE, f);

        while(*cur_p && isspace(*cur_p)) cur_p++;

        switch_param(cur_p, cur_param);
      }
    }
    fclose (f);
    runtime.sim.script_file_specified = 1;
  } else
    if (config.sim.verbose)
      fprintf (stderr, "WARNING: Cannot read script file from '%s',\nneither '%s'.\n", filename, ctmp);
}

/* Utility for execution of set sim command.  */
static int set_config (int argc, char **argv)
{
  struct config_section *cur;
  struct config_param *cur_param;

  if (argc < 2) return 1;

  PRINTF ("sec:%s\n", argv[1]);
  cur_section = NULL;
  for (cur = sections; cur; cur = cur->next)
    if (strcmp (cur->name, argv[1]) == 0) {
      cur_section = cur;
      break;
    }

  if (!cur_section) return 1;

  if (argc < 3) return 2;
  
  PRINTF ("item:%s\n", argv[2]);
  {
    for (cur_param = cur->params; cur_param; cur_param = cur_param->next)
      if (strcmp (cur_param->name, argv[2]) == 0) {
        break;
      }
    if (!cur_param) return 2;
    
    /* Parse parameter value */
    if (cur_param->type) {
      if (argc < 4) return 3;
      PRINTF ("params:%s\n", argv[3]);
    }

    switch_param(argv[3], cur_param);
  }
  return 0;
}

/* Executes set sim command, displays error.  */
void set_config_command(int argc, char **argv)
{
  struct config_section *cur;
  struct config_param *cur_param;

  switch (set_config (argc, argv)) {
    case 1:
      PRINTF ("Invalid or missing section name.  One of valid sections must be specified:\n");
      for (cur = sections; cur; cur = cur->next)
        PRINTF ("%s ", cur->name);
      PRINTF ("\n");
      break;
    case 2:
      PRINTF ("Invalid or missing item name.  One of valid items must be specified:\n");
      for (cur_param = cur_section->params; cur_param; cur_param = cur_param->next)
        PRINTF ("%s ", cur_param->name);
      PRINTF ("\n");
      break;
    case 3:
      PRINTF ("Invalid parameters specified.\n");
      break;
  }
}

