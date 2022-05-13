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
  /* 0x00 */ volatile uint8_t month;
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

/* register masks - from drvHel.c */
#define TSETTLE_MASK	0x1f	/* 5 bit register */
#define TSTABLE_MASK	0x1f	/* 5 bit register */
#define DELAY_MASK	0x0f	/* 4 bit register */
#define MONTH_MASK	0xff
#define DAY_MASK	0xff
#define YEAR_MASK	0xff
#define PATTERN_MASK	0x07	/* 3 bit register */
#define CLOCK_MASK	0xff

/* Macro to check for library / pointer initialization */
#define CHECKHELI

#ifndef __JVME_DEVADDR_T
#define __JVME_DEVADDR_T
typedef unsigned long devaddr_t;
#endif

/* Structure to keep track of library variables */
typedef struct
{
  volatile heliRegs *dev;     /* Pointer to device registers */
  volatile heliRegs local;    /* locally stored register values */
  devaddr_t a24_offset;       /* Offset between VME A24 and Local address space */
  pthread_mutex_t rw_mutex;   /* Local library structure Mutex */
  uint8_t  rw_mode;           /* Whether to Set/Get routines access dev (0) or local (1) reg values */
} heliLibVars;

static heliLibVars hl = {NULL};

#define HLOCK   if(pthread_mutex_lock(&hl.rw_mutex)<0) perror("pthread_mutex_lock");
#define HUNLOCK if(pthread_mutex_unlock(&hl.rw_mutex)<0) perror("pthread_mutex_unlock");


int32_t
heliInit(uint32_t a24_addr, uint16_t init_flag)
{

  return 0;
}

int32_t
heliStatus(int32_t print_regs)
{
  CHECKHELI;

  HLOCK;

  HUNLOCK;

  return 0;
}

int32_t
heliSetLibraryMode(uint8_t rw_mode_set)
{


  return 0;
}

int32_t
heliConfigure(uint8_t tsettle_set, uint8_t tstable_set, uint8_t delay_set,
	      uint8_t pattern_set, uint8_t clock_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetSettings(uint8_t *tsettle_set, uint8_t *tstable_set, uint8_t *delay_set,
		uint8_t *pattern_set, uint8_t *clock_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliSetTSettle(uint8_t tsettle_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetTSettle(uint8_t tsettle_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliSetTStable(uint8_t tstable_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetTStable(uint8_t tstable_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliSetDelay(uint8_t delay_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetDelay(uint8_t delay_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliSetPattern(uint8_t pattern_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetPattern(uint8_t pattern_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliSetClock(uint8_t clock_set)
{
  CHECKHELI;

  return 0;
}

int32_t
heliGetClock(uint8_t clock_set)
{
  CHECKHELI;

  return 0;
}
