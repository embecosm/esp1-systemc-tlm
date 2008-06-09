/* sim-config.h -- Simulator configuration header file
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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdio.h>

/* Simulator configuration macros. Eventually this one will be a lot bigger. */

#define MAX_SBUF_LEN     256          /* Max. length of store buffer */

#define EXE_LOG_HARDWARE 0            /* Print out RTL states */
#define EXE_LOG_SIMPLE   1            /* Executed log prints out dissasembly */
#define EXE_LOG_SOFTWARE 2            /* Simple with some register output*/

#define STR_SIZE        (256)

struct config {
  struct {
    int enabled;                      /* Is tick timer enabled?  */
  } tick;
  
  struct {
    int enabled;                      /* Whether IMMU is enabled */
    int nways;                        /* Number of ITLB ways */
    int nsets;                        /* Number of ITLB sets */
    int pagesize;                     /* ITLB page size */
    int pagesize_log2;                /* ITLB page size (log2(pagesize)) */
    oraddr_t page_offset_mask;        /* Address mask to get page offset */
    oraddr_t page_mask;               /* Page number mask (diff. from vpn) */
    oraddr_t vpn_mask;                /* Address mask to get vpn */
    int lru_reload;                   /* What to reload the lru value to */
    oraddr_t set_mask;                /* Mask to get set of an address */
    int entrysize;                    /* ITLB entry size */
    int ustates;                      /* number of ITLB usage states */
    int missdelay;                    /* How much cycles does the miss cost */
    int hitdelay;                     /* How much cycles does the hit cost */
  } immu;
  
  struct {
    int enabled;                      /* Whether DMMU is enabled */
    int nways;                        /* Number of DTLB ways */
    int nsets;                        /* Number of DTLB sets */
    int pagesize;                     /* DTLB page size */
    int pagesize_log2;                /* DTLB page size (log2(pagesize)) */
    oraddr_t page_offset_mask;        /* Address mask to get page offset */
    oraddr_t page_mask;               /* Page number mask (diff. from vpn) */
    oraddr_t vpn_mask;                /* Address mask to get vpn */
    int lru_reload;                   /* What to reload the lru value to */
    oraddr_t set_mask;                /* Mask to get set of an address */
    int entrysize;                    /* DTLB entry size */
    int ustates;                      /* number of DTLB usage states */
    int missdelay;                    /* How much cycles does the miss cost */
    int hitdelay;                     /* How much cycles does the hit cost */
  } dmmu;
  
  struct {
    int enabled;                      /* Whether instruction cache is enabled */
    int nways;                        /* Number of IC ways */
    int nsets;                        /* Number of IC sets */
    int blocksize;                    /* IC entry size */
    int ustates;                      /* number of IC usage states */
    int missdelay;                    /* How much cycles does the miss cost */
    int hitdelay;                     /* How much cycles does the hit cost */
  } ic;

  struct {
    int enabled;                      /* Whether data cache is enabled */
    int nways;                        /* Number of DC ways */
    int nsets;                        /* Number of DC sets */
    int blocksize;                    /* DC entry size */
    int ustates;                      /* number of DC usage states */
    int store_missdelay;              /* How much cycles does the store miss cost */
    int store_hitdelay;               /* How much cycles does the store hit cost */
    int load_missdelay;               /* How much cycles does the load miss cost */
    int load_hitdelay;                /* How much cycles does the load hit cost */
  } dc;
  
  struct {
    int enabled;                      /* branch prediction buffer analysis */
    int sbp_bnf_fwd;                  /* Static branch prediction for l.bnf uses forward prediction */
    int sbp_bf_fwd;                   /* Static branch prediction for l.bf uses forward prediction */    
    int btic;		              /* branch prediction target insn cache analysis */
    int missdelay;                    /* How much cycles does the miss cost */
    int hitdelay;                     /* How much cycles does the hit cost */
#if 0                                 
    int nways;                        /* Number of BP ways */
    int nsets;                        /* Number of BP sets */
    int blocksize;                    /* BP entry size */
    int ustates;                      /* number of BP usage states */
    int pstates;                      /* number of BP predict states */
#endif                                
  } bpb;                              
                                      
  struct {                            
    unsigned long upr;                /* Unit present register */
    unsigned long ver, rev;           /* Version register */
    int sr;                           /* Supervision register */
    int superscalar;                  /* superscalara analysis */
    int hazards;                      /* dependency hazards analysis */
    int dependstats;                  /* dependency statistics */
    int sbuf_len;                     /* length of store buffer, zero if disabled */
  } cpu;                              
                                      
  struct {                            
    int debug;                        /* Simulator debugging */
    int verbose;                      /* Force verbose output */
                                      
    int profile;                      /* Is profiler running */
    char prof_fn[STR_SIZE];           /* Profiler filename */
                                      
    int mprofile;                     /* Is memory profiler running */
    char mprof_fn[STR_SIZE];          /* Memory profiler filename */
                                      
    int history;                      /* instruction stream history analysis */
    int exe_log;                      /* Print out RTL states? */
    int exe_log_type;                 /* Type of log */
    long long int exe_log_start;      /* First instruction to log */
    long long int exe_log_end;        /* Last instruction to log, -1 if continuous */
    int exe_log_marker;               /* If nonzero, place markers before each exe_log_marker instructions */
    char exe_log_fn[STR_SIZE];        /* RTL state comparison filename */
    char fstdout[STR_SIZE];           /* stdout filename */
    long clkcycle_ps;                 /* Clock duration in ps */
    long system_kfreq;                /* System frequency in kHz*/
  } sim;                              
                                      
  struct {                            
    int enabled;                      /* Whether is debug module enabled */
    int gdb_enabled;                  /* Whether is debugging with gdb possible */
    int server_port;                  /* A user specified port number for services */
    unsigned long vapi_id;            /* "Fake" vapi device id for JTAG proxy */
  } debug;                            
                                      
  struct {                            /* Verification API, part of Advanced Core Verification */
    int enabled;                      /* Whether is VAPI module enabled */
    int server_port;                  /* A user specified port number for services */
    int log_enabled;                  /* Whether to log the vapi requests */
    int hide_device_id;               /* Whether to log device ID for each request */
    char vapi_fn[STR_SIZE];           /* vapi log filename */
  } vapi;                             
                                      
  struct {                            
    int enabled;                      /* Whether power menagement is operational */
  } pm;                               

  struct {
    char timings_fn[STR_SIZE];        /* Filename of the timing table */
    int memory_order;                 /* Memory access stricness */
    int calling_convention;           /* Whether functions follow standard calling convention */
    int enable_bursts;                /* Whether burst are enabled */
    int no_multicycle;                /* When enabled no multicycle paths are generated */
  } cuc;
};                                    
                                      
struct runtime {                      
  struct {                            
    FILE *fprof;                      /* Profiler file */
    FILE *fmprof;                     /* Memory profiler file */
    FILE *fexe_log;                   /* RTL state comparison file */
    FILE *fout;                       /* file for standard output */
    int init;                         /* Whether we are still initilizing sim */
    int script_file_specified;        /* Whether script file was already loaded */
    char *filename;                   /* Original Command Simulator file (CZ) */
    char script_fn[STR_SIZE];         /* Script file read */
    int iprompt;                      /* Interactive prompt */
    int iprompt_run;                  /* Interactive prompt is running */
    long long cycles;                 /* Cycles counts fetch stages */

    int mem_cycles;                   /* Each cycle has counter of mem_cycles;
                                         this value is joined with cycles
                                         at the end of the cycle; no sim
                                         originated memory accesses should be
                                         performed inbetween. */
    int loadcycles;                   /* Load and store stalls */
    int storecycles;

    long long reset_cycles;

    int hush;                         /* Is simulator to do reg dumps */
  } sim;

  /* Command line parameters */
  struct {
    int profile;                      /* Whether profiling was enabled */
    int mprofile;                     /* Whether memory profiling was enabled */
  } simcmd;
  
  struct {
    long long instructions;           /* Instructions executed */
    long long reset_instructions;
    
    int stalled;
    int hazardwait;                   /* how many cycles were wasted because of hazards */
    int supercycles;                  /* Superscalar cycles */
  } cpu;                              
                                      
  struct {                            /* Verification API, part of Advanced Core Verification */
    int enabled;                      /* Whether is VAPI module enabled */
    FILE *vapi_file;                  /* vapi file */
    int server_port;                  /* A user specified port number for services */
  } vapi;
  
/* CUC configuration parameters */
  struct {
    int mdelay[4];                  /* average memory delays in cycles
                                     {read single, read burst, write single, write burst} */
    double cycle_duration;          /* in ns */
  } cuc;
};

extern struct config config;

#define PRINTF(x...) fprintf (runtime.sim.fout, x)

extern struct runtime runtime;

/* Read environment from a script file. Does not fail - assumes defaukt configuration instead. */
void read_script_file (char *filename);

/* Executes set sim command.  Returns nonzero if error.  */
void set_config_command (int argc, char **argv);

void init_defconfig(void);

int parse_args(int argc, char *argv[]);

void print_config(void);

void sim_done(void);

/* Periodically checks runtime.sim.iprompt to see if ctrl_c has been pressed */
void check_int(void *dat);

/* Number of cycles between checks to runtime.sim.iprompt */
#define CHECK_INT_TIME 100000

/* Resets all subunits */
void sim_reset(void);

/* Handle the sim commandline */
void handle_sim_command(void);

/* Registers a new reset hook, called when sim_reset below is called */
void reg_sim_reset(void (*reset_hook)(void *), void *dat);

/* Registers a status printing callback */
void reg_sim_stat(void (*stat_func)(void *dat), void *dat);

union param_val {
  char *str_val;
  int int_val;
  long long int longlong_val;
  oraddr_t addr_val;
};

enum param_t {
  paramt_none = 0, /* No parameter */
  paramt_str, /* String parameter enclosed in double quotes (") */
  paramt_word, /* String parameter NOT enclosed in double quotes */
  paramt_int, /* Integer parameter */
  paramt_longlong, /* Long long int parameter */
  paramt_addr /* Address parameter */
};

struct config_section {
  char *name;
  void *(*sec_start)(void);
  void (*sec_end)(void *);
  void *dat;
  struct config_param *params;
  struct config_section *next;
};

/* Register a parameter in a section of the config file */
void reg_config_param(struct config_section *sec, const char *param,
                      enum param_t type,
                      void (*param_cb)(union param_val, void*));

/* Register a section in the config file */
struct config_section *reg_config_sec(const char *section,
                                      void *(*sec_start)(void),
                                      void (*sec_end)(void *));

extern struct config_section *cur_section;
#define CONFIG_ERROR(s) {fprintf (stderr, "ERROR: config.%s:%s\n", cur_section->name, s); if (runtime.sim.init) exit (1);}

/* FIXME: These will disapeer one day... */
void reg_mc_sec(void);
void reg_uart_sec(void);
void reg_dma_sec(void);
void reg_memory_sec(void);
void reg_debug_sec(void);
void reg_vapi_sec(void);
void reg_ethernet_sec(void);
void reg_immu_sec(void);
void reg_dmmu_sec(void);
void reg_ic_sec(void);
void reg_dc_sec(void);
void reg_gpio_sec(void);
void reg_bpb_sec(void);
void reg_pm_sec(void);
void reg_vga_sec(void);
void reg_fb_sec(void);
void reg_kbd_sec(void);
void reg_ata_sec(void);
void reg_cuc_sec(void);
void reg_config_secs(void);
#endif
