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

typedef struct
{
  /* 0x00 */ volatile uint8_t month; // times out (BERR)
  /* 0x01 */ volatile uint8_t day;
  /* 0x02 */ uint8_t _blank0[5];

  /* 0x07 */ volatile uint8_t tsettle;
  /* 0x08 */ uint8_t _blank1;
  /* 0x09 */ volatile uint8_t tstable;
  /* 0x0a */ uint8_t _blank2;
  /* 0x0b */ volatile uint8_t delay;

  /* 0x0c */ uint8_t _blank3;
  /* 0x0d */ volatile uint8_t pattern;
  /* 0x0e */ uint8_t _blank4;
  /* 0x0f */ volatile uint8_t clock;
} heliRegs;

/* timing values */
double fTSettleVals[32] =
  {
   10, 20, 30, 40, 50, 60, 70, 80,
   90, 100, 110, 120, 130, 140, 150, 160,
   170, 180, 190, 200, 250, 300, 350, 400,
   450, 500, 550, 600, 700, 800, 900, 1000
  };

double fTStableVals[32] =
  {
   400, 500, 600, 700, 800, 900, 971.65, 1000,
   1001.65, 1318.90, 1348.90, 1500, 2000, 2500, 3000, 3500,
   4066.65, 4076.65, 5000, 5500, 6000, 6500, 7000, 8233.35,
   8243.35, 16567, 16667, 33230, 33330, 50000, 100000, 1000000
  };

double fClockVals[4] =
  {
   30, 120, 240, -1
  };

/* Macro to check for library / pointer initialization */
#define CHECKHELI  {							\
    if(hl.initialized == 0)						\
      {                                                                 \
        printf("%s: ERROR: Helcity Generator Library is not initialized \n", \
               __func__);						\
        return -1;							\
      }                                                                 \
  }

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
  volatile heliRegs local;    /* locally stored register values */
  devaddr_t a24_offset;       /* Offset between VME A24 and Local address space */
  pthread_mutex_t rw_mutex;   /* Local library structure Mutex */
  uint8_t  debug;             /* Whether or not to print debug messages to stdout */
} heliLibVars;

/* Initialize the local structure */
static heliLibVars hl = {0, NULL, {0,0,{0,0,0,0,0},0,0,0,0,0,0,0,0}, 0, PTHREAD_MUTEX_INITIALIZER, 0};

#define HLOCK   if(pthread_mutex_lock(&hl.rw_mutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hl.rw_mutex)<0) perror("pthread_mutex_unlock");

#define READHELI(_reg) {hl.local._reg = vmeRead8(&hl.dev->_reg);}
#define WRITEHELI(_reg,_val) {hl.local._reg = _val; vmeWrite8(&hl.dev->_reg, _val);}

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
  int _off = 0;
#define PREG(_reg) {							\
    printf("  %10.18s (0x%02lx) = 0x%02x%s",				\
	   #_reg, (unsigned long)&hl.local._reg - (unsigned long)&hl.local, hl.local._reg, \
	   ((_off++ % 2) == 0) ? "\t" : "\n");				\
      }
  HELI_EPICS ep;

  CHECKHELI;

  HLOCK;

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

  printf("0x%02x      ", hl.local.tsettle & HELI_TSETTLE_MASK);

  printf("0x%02x      ", hl.local.tstable & HELI_TSTABLE_MASK);

  printf("0x%02x      ", hl.local.delay & HELI_DELAY_MASK);

  printf("0x%02x      ", hl.local.pattern & HELI_PATTERN_MASK);

  printf("0x%02x      ", hl.local.clock & HELI_CLOCK_MASK);

  printf("\n");

  ep.iClockReadback = hl.local.clock & HELI_CLOCK_MASK;
  ep.fTSettleReadbackVal = fTSettleVals[hl.local.tstable & HELI_TSTABLE_MASK];
  switch(ep.iClockReadback)
    {
    case 0:
    case 1:
    case 2:
      ep.fFreqReadback = fClockVals[ep.iClockReadback];
      ep.fTStableReadbackVal = ((1.0 / ep.fFreqReadback) * 1000000.0) - ep.fTSettleReadbackVal;
      break;

    default:
      ep.fTStableReadbackVal = fTStableVals[hl.local.tstable & HELI_TSTABLE_MASK];
      ep.fFreqReadback = (1.0 / (ep.fTSettleReadbackVal + ep.fTStableReadbackVal)) * 1000000.0;

    }

  printf("           %7.1f      ", ep.fTSettleReadbackVal);
  printf("%7.1f     ", ep.fTStableReadbackVal);
  printf("               %7.1f ", ep.fFreqReadback);

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
heliConfigure(uint8_t tsettle_set, uint8_t tstable_set, uint8_t delay_set,
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
  WRITEHELI(tsettle, tsettle_set);
  WRITEHELI(tstable, tstable_set);
  WRITEHELI(delay, delay_set);
  WRITEHELI(pattern, pattern_set);
  WRITEHELI(clock, clock_set);
  HUNLOCK;

  return 0;
}

int32_t
heliGetSettings(uint8_t *tsettle_set, uint8_t *tstable_set, uint8_t *delay_set,
		uint8_t *pattern_set, uint8_t *clock_set)
{
  CHECKHELI;

  HLOCK;
  READHELI(tsettle);
  READHELI(tstable);
  READHELI(delay);
  READHELI(pattern);
  READHELI(clock);

  *tsettle_set = hl.local.tsettle;
  *tstable_set = hl.local.tstable;
  *delay_set = hl.local.delay;
  *pattern_set = hl.local.pattern;
  *clock_set = hl.local.clock;
  HUNLOCK;


  return 0;
}

int32_t
heliGetEpicsVars(HELI_EPICS *readback)
{
  CHECKHELI;

  HLOCK;
  READHELI(day);
  READHELI(tsettle);
  READHELI(tstable);
  READHELI(delay);
  READHELI(pattern);
  READHELI(clock);

  readback->iTsettleReadback = hl.local.tsettle & HELI_TSETTLE_MASK;
  readback->fTSettleReadbackVal = fTSettleVals[hl.local.tstable & HELI_TSTABLE_MASK];

  readback->iClockReadback = hl.local.clock & HELI_CLOCK_MASK;
  switch(readback->iClockReadback)
    {
    case 0:
    case 1:
    case 2:
      readback->fFreqReadback = fClockVals[readback->iClockReadback];
      readback->fTStableReadbackVal =
	((1.0 / readback->fFreqReadback) * 1000000.0) - readback->fTSettleReadbackVal;
      break;

    default:
      readback->fTStableReadbackVal = fTStableVals[hl.local.tstable & HELI_TSTABLE_MASK];
      readback->fFreqReadback =
	(1.0 / (readback->fTSettleReadbackVal + readback->fTStableReadbackVal)) * 1000000.0;
    }

  HUNLOCK;

  return 0;
}
