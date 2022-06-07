#pragma once
/*
 * Copyright 2022, Jefferson Science Associates, LLC.
 * Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *     Authors: Bryan Moffit
 *              moffit@jlab.org                   Jefferson Lab, MS-12B3
 *              Phone: (757) 269-5660             12000 Jefferson Ave.
 *              Fax:   (757) 269-5800             Newport News, VA 23606
 *
 * Description: Header for Helicity Generator module Library
 *
 */

#include <stdint.h>

/* register masks - from drvHel.c */
#define HELI_TSETTLE_MASK  0x1f	/* 5 bit register */
#define HELI_TSTABLE_MASK  0x1f	/* 5 bit register */
#define HELI_DELAY_MASK    0x0f	/* 4 bit register */
#define HELI_MONTH_MASK	   0xff
#define HELI_DAY_MASK	   0xff
#define HELI_YEAR_MASK     0xff
#define HELI_PATTERN_MASK  0x07	/* 3 bit register */
#define HELI_CLOCK_MASK    0xff


#define HELI_INIT_DEBUG (0 << 1)

typedef struct
{
  int32_t iTsettleReadback;
  double  fTSettleReadbackVal;

  int32_t iTStableReadbackVal;
  double  fTStableReadbackVal;

  double  fFreqReadback;
  int32_t iClockReadback;
} HELI_EPICS;

int32_t heliInit(uint32_t a24_addr, uint16_t init_flag);
int32_t heliStatus(int32_t print_regs);

int32_t heliConfigure(uint8_t tsettle_set, uint8_t tstable_set, uint8_t delay_set,
		      uint8_t pattern_set, uint8_t clock_set);
int32_t heliGetSettings(uint8_t *tsettle_set, uint8_t *tstable_set, uint8_t *delay_set,
			uint8_t *pattern_set, uint8_t *clock_set);

int32_t heliGetEpicsVars(HELI_EPICS *readback);
