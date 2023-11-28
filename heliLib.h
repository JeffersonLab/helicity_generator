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

#define HELI_INIT_DEBUG (0 << 1)

typedef struct
{
  /* 0x00 */ volatile uint8_t month; // times out (BERR)
  /* 0x01 */ volatile uint8_t day;
  /* 0x02          */ uint8_t _blank0[5];

  /* 0x07 */ volatile uint8_t tsettle;
  /* 0x08          */ uint8_t _blank1;
  /* 0x09 */ volatile uint8_t tstable;
  /* 0x0a          */ uint8_t _blank2;
  /* 0x0b */ volatile uint8_t delay;

  /* 0x0c          */ uint8_t _blank3;
  /* 0x0d */ volatile uint8_t pattern;
  /* 0x0e          */ uint8_t _blank4;
  /* 0x0f */ volatile uint8_t clock;
} heliRegs;

/* register masks - from drvHel.c */
#define HELI_TSETTLE_MASK  0x1f	/* 5 bit register */
#define HELI_TSTABLE_MASK  0x1f	/* 5 bit register */
#define HELI_DELAY_MASK    0x0f	/* 4 bit register */
#define HELI_MONTH_MASK	   0xff
#define HELI_DAY_MASK	   0xff
#define HELI_YEAR_MASK     0xff
#define HELI_PATTERN_MASK  0x0f	/* 4 bit register */
#define HELI_CLOCK_MASK    0xff


int32_t heliInit(uint32_t a24_addr, uint16_t init_flag);
int32_t heliStatus(int32_t print_regs);

int32_t heliSetDebug(uint8_t debug_set);
int32_t heliGetDebug();

int32_t heliSetRegisters(uint8_t TSETTLEin, uint8_t TSTABLEin, uint8_t DELAYin,
			 uint8_t PATTERNin, uint8_t CLOCKin);
int32_t heliGetRegisters(uint8_t *TSETTLEout, uint8_t *TSTABLEout, uint8_t *DELAYout,
			 uint8_t *PATTERNout, uint8_t *CLOCKout);

int32_t heliSetMode(uint32_t CLOCKs);
int32_t heliGetMode(uint32_t *CLOCKd);

int32_t heliSetHelicityPattern(uint32_t PATTERNs);
int32_t heliGetHelicityPattern(uint32_t *PATTERNd);

int32_t heliSetReportingDelay(uint32_t DELAYs);
int32_t heliGetReportingDelay(uint32_t *DELAYd);

int32_t heliGetHelcityTiming(double *fTSettleReadbackVal,
			     double *fTStableReadbackVal, double *fFreqReadback);
int32_t heliGetHelicityBoardFrequency(double *FREQ);

void heliPrintTSettle();
int32_t heliSetTSettle(uint8_t TSETTLEs);
int32_t heliGetTSettle(double *TSETTLEd);

void heliPrintTStable();
int32_t heliSetTStable(uint8_t TSTABLEs);
int32_t heliGetTStable(double *TSTABLEd);
