/*
 * Copyright 2022, Jefferson Science Associates, LLC.
 * Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *     Authors: Bryan Moffit
 *              moffit@jlab.org                   Jefferson Lab, MS-12B3
 *              Phone: (757) 269-5660             12000 Jefferson Ave.
 *              Fax:   (757) 269-5800             Newport News, VA 23606
 *
 * Description: Library for Helicity Generator module
 *
 */

#include <stdio.h>
#include <pthread.h>
#include "jvme.h"
#include "heliLib.h"

/* Macro to check for library / pointer initialization */
#define CHECKHELI  { if(hl.initialized == 0) {HELI_ERR("Helicity Generator Library is not initialized\n"); return -1;}}

#define HELI_DBG(format, ...) {if (hl.debug==1) {fprintf(stdout,"%s: DEBUG: ",__func__); fprintf(stdout,format, ## __VA_ARGS__);}}
#define HELI_ERR(format, ...) {fprintf(stderr,"%s: ERROR: ",__func__); fprintf(stderr,format, ## __VA_ARGS__);}

#ifndef __JVME_DEVADDR_T
#define __JVME_DEVADDR_T
typedef unsigned long devaddr_t;
#endif

/* Structure to keep track of library variables */
typedef struct
{
  int8_t   initialized;       /* Whether (1) or not (0) the library has been initialized */
  volatile heliRegs *dev;     /* Pointer to device registers */
  devaddr_t a24_offset;       /* Offset between VME A24 and Local address space */
  pthread_mutex_t rw_mutex;   /* Local library structure Mutex */
  uint8_t  debug;             /* Whether or not to print debug messages to stdout */
} heliLibVars;

/* Initialize the local structure */
static heliLibVars hl = {0, NULL, 0, PTHREAD_MUTEX_INITIALIZER, 0};

#define HLOCK   if(pthread_mutex_lock(&hl.rw_mutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hl.rw_mutex)<0) perror("pthread_mutex_unlock");

#define WRITEHELI(_reg,_val) {hl.local._reg = _val; vmeWrite8(&hl.dev->_reg, _val);}

/* Settle Time (usec) */
double fTSettleVals[32] = {
  5,  10,  15,  20,  25,  30,  35,  40,
  45,  50,  60,  70,  80,  90, 100, 110,
  120, 130, 140, 150, 160, 170, 180, 190,
  200, 250, 300, 350, 400, 450, 500, 1000
};

/* Stable Time (usec) */
double fTStableVals[32] = {
  240.40, 245.40,  250.40,  255.40,   470.85,  475.85,  480.85,  485.85,
  490.85, 495.85,  500.85,  505.85,   510.85,  515.85,  900,     971.65,
  1000,  1001.65, 1318.90, 1348.90,  2000,    3000,    4066.65, 5000,
  6000,  7000,    8233.35, 8243.35, 16567,   16667,   33230,   33330
};

/* Mode : LineSync / Free Clock */
double fClockVals[4] =
  {
   30, 120, 240, -1
  };

/* Reporting Delay */
double fDelayVals[16] =
  {
     0,  1,  2,  4,  8,  16,  24,  32,
    40, 48, 64, 72, 96, 112, 128, 256
  };

/* Helicity Pattern */
char sPatternVals[12][256] =
  {
    "Pair",
    "Quartet",
    "Octet",
    "Toggle",
    "Hexo-Quad",
    "Octo-Quad",
    "SPARE [Toggle]",
    "SPARE [Toggle]",
    "Thue-Morse-64",
    "16-Quad",
    "32-Pair"
  };

int32_t
heliInit(uint32_t a24_addr, uint16_t init_flag)
{
  devaddr_t laddr = 0;
  uint8_t rdata = 0;
  int32_t res = 0;

  HLOCK;
  if(hl.initialized)
    {
      printf("%s: WARNING: Re-initializing Helicity Generator library\n",
	     __func__);
    }

  /* translate a24 address to local */
  res = vmeBusToLocalAdrs(0x39, (char *) (unsigned long)a24_addr, (char **) &laddr);

  if(res != 0)
    {
      printf("%s: ERROR in vmeBusToLocalAdrs(0x39,0x%x,&laddr) \n", __func__, a24_addr);
      return (ERROR);
    }

  res = vmeMemProbe((char *) (laddr+0x1), 1, (char *) &rdata);
  if(res < 0)
    {
      printf("%s: ERROR: No addressable module found at VME (local) address 0x%08x (0x%lx)\n",
	     __func__, a24_addr, laddr);
      HUNLOCK;
      return ERROR;
    }

  /* Parse the init_flag */
  hl.debug = (init_flag & HELI_INIT_DEBUG) ? 1 : 0;

  HELI_DBG("helicity generator module found at 0x%08x (0x%lx).  rdata = 0x%x\n",
	   a24_addr, laddr, rdata);

  /* remember the local to a24 address offset */
  hl.a24_offset = laddr - a24_addr;

  /* Map the device pointer to the module registers */
  hl.dev = (volatile heliRegs *) laddr;

  hl.initialized = 1;

  HUNLOCK;
  return 0;
}

int32_t
heliStatus(int32_t print_regs)
{
  volatile heliRegs hr;
  CHECKHELI;

  HLOCK;

#define READHELI(_reg) {hr._reg = vmeRead8(&hl.dev->_reg);}

  READHELI(day);
  READHELI(tsettle);
  READHELI(tstable);
  READHELI(delay);
  READHELI(pattern);
  READHELI(clock);

  HUNLOCK;

  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("STATUS for JLab Helicity Generator\n");

  int _off = 0;
#define PREG(_reg) {							\
    printf("  %10.18s (0x%02lx) = 0x%02x%s",				\
	   #_reg, (unsigned long)&hr._reg - (unsigned long)&hr, hr._reg, \
	   ((_off++ % 2) == 0) ? "\t" : "\n");				\
  }
  if(print_regs)
    {
      printf("\n");
      PREG(day);
      PREG(tsettle);
      PREG(tstable);
      PREG(delay);
      PREG(pattern);
      PREG(clock);
      printf("\n");
    }

  printf("\n");
  printf(" A24 addr     tsettle   tstable   delay     pattern   clock\n");
  printf("              [us]      [us]                          [Hz]\n");
  printf("--------------------------------------------------------------------------------\n");

  printf(" 0x%06x     ", (uint32_t)((devaddr_t)&hl.dev->month - hl.a24_offset));

  printf("0x%02x      ", hr.tsettle & HELI_TSETTLE_MASK);

  printf("0x%02x      ", hr.tstable & HELI_TSTABLE_MASK);

  printf("0x%02x      ", hr.delay & HELI_DELAY_MASK);

  printf("0x%02x      ", hr.pattern & HELI_PATTERN_MASK);

  printf("0x%02x      ", hr.clock & HELI_CLOCK_MASK);

  printf("\n");


  printf("\n");
  printf("\n");
  printf("--------------------------------------------------------------------------------\n");
  printf("\n");

  return 0;
}

int32_t
heliSetDebug(uint8_t debug_set)
{
  CHECKHELI;

  HLOCK;
  hl.debug = (debug_set) ? 1 : 0;
  HUNLOCK;

  return 0;
}

int32_t
heliGetDebug()
{
  int32_t rval = 0;
  CHECKHELI;

  HLOCK;
  rval = hl.debug;
  HUNLOCK;

  return rval;
}

int32_t
heliSetRegisters(uint8_t tsettle_set, uint8_t tstable_set, uint8_t delay_set,
		 uint8_t pattern_set, uint8_t clock_set)
{
  CHECKHELI;

  if(tsettle_set > HELI_TSETTLE_MASK)
    {
      HELI_ERR("Invalid tsettle_set (0x%x)\n", tsettle_set);
      return ERROR;
    }

  if(tstable_set > HELI_TSTABLE_MASK)
    {
      HELI_ERR("Invalid tstable_set (0x%x)\n", tstable_set);
      return ERROR;
    }

  if(delay_set > HELI_DELAY_MASK)
    {
      HELI_ERR("Invalid delay_set (0x%x)\n", delay_set);
      return ERROR;
    }

  if(pattern_set > HELI_PATTERN_MASK)
    {
      HELI_ERR("Invalid pattern_set (0x%x)\n", pattern_set);
      return ERROR;
    }

  if(clock_set > HELI_CLOCK_MASK)
    {
      HELI_ERR("Invalid clock_set (0x%x)\n", clock_set);
      return ERROR;
    }


  HLOCK;
  vmeWrite8(&hl.dev->tsettle, tsettle_set);
  vmeWrite8(&hl.dev->tstable, tstable_set);
  vmeWrite8(&hl.dev->delay, delay_set);
  vmeWrite8(&hl.dev->pattern, pattern_set);
  vmeWrite8(&hl.dev->clock, clock_set);
  HUNLOCK;

  return 0;
}
