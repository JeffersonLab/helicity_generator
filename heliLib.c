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
char sPatternVals[11][256] =
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


/**
 * @brief Initialize Helicity Generator Library
 * @details Initialize Helicity Generator Library
 * @param[in] a24_addr VME A24 of the Helicity Generator (0xa00000)
 * @param[in] init_flag Initialization bit mask
 *             value  what
 *                 1  DEBUG enabled
 *
 * @return 0 if successful, otherwise -1
 */
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

/**
 * @brief Show Helicity Generator Status
 * @details Print the status of the helicity generator to standard out.
 * @param[in] print_regs Flag to print raw register values (1=enable)
 * @return 0 if successful, otherwise -1
 */
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

/**
 * @brief Enable / Disable debug messages
 * @details Enable / Disable debug messages
 * @param[in] debug_set 1=enable, 0=disable
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetDebug(uint8_t debug_set)
{
  CHECKHELI;

  HLOCK;
  hl.debug = (debug_set) ? 1 : 0;
  HUNLOCK;

  return 0;
}

/**
 * @brief Return the value of the debug flag
 * @details Return the value of the debug flag
 * @return 1 if enabled, 0 if disabled, otherwise -1
 */
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

/**
 * @brief Set helicity generator registers
 * @details Set the values directly to helicity generator registers
 * @param[in] TSETTLEin TSETTLE register value
 * @param[in] TSTABLEin TSTABLE register value
 * @param[in] DELAYin DELAY register value
 * @param[in] PATTERNin PATTERN register value
 * @param[in] CLOCKin CLOCK register value
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetRegisters(uint8_t TSETTLEin, uint8_t TSTABLEin, uint8_t DELAYin,
		 uint8_t PATTERNin, uint8_t CLOCKin)
{
  CHECKHELI;

  if(TSETTLEin > HELI_TSETTLE_MASK)
    {
      HELI_ERR("Invalid TSETTLEin (0x%x)\n", TSETTLEin);
      return ERROR;
    }

  if(TSTABLEin > HELI_TSTABLE_MASK)
    {
      HELI_ERR("Invalid TSTABLEin (0x%x)\n", TSTABLEin);
      return ERROR;
    }

  if(DELAYin > HELI_DELAY_MASK)
    {
      HELI_ERR("Invalid DELAYin (0x%x)\n", DELAYin);
      return ERROR;
    }

  if(PATTERNin > HELI_PATTERN_MASK)
    {
      HELI_ERR("Invalid PATTERNin (0x%x)\n", PATTERNin);
      return ERROR;
    }

  if(CLOCKin > HELI_CLOCK_MASK)
    {
      HELI_ERR("Invalid CLOCKin (0x%x)\n", CLOCKin);
      return ERROR;
    }


  HLOCK;
  vmeWrite8(&hl.dev->tsettle, TSETTLEin);
  vmeWrite8(&hl.dev->tstable, TSTABLEin);
  vmeWrite8(&hl.dev->delay, DELAYin);
  vmeWrite8(&hl.dev->pattern, PATTERNin);
  vmeWrite8(&hl.dev->clock, CLOCKin);
  HUNLOCK;

  return 0;
}

/**
 * @brief Return the helicity generator register values
 * @details Return the helicity generator register values
 * @param[inout] TSETTLEout TSETTLE register value
 * @param[out] TSTABLEout TSTABLE register value
 * @param[out] DELAYout DELAY register value
 * @param[out] PATTERNout PATTERN register value
 * @param[out] CLOCKout CLOCK register value
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetRegisters(uint8_t *TSETTLEout, uint8_t *TSTABLEout, uint8_t *DELAYout,
		 uint8_t *PATTERNout, uint8_t *CLOCKout)
{
  CHECKHELI;

  HLOCK;
  *TSETTLEout = vmeRead8(&hl.dev->tsettle) & HELI_TSETTLE_MASK;
  *TSTABLEout = vmeRead8(&hl.dev->tstable) & HELI_TSTABLE_MASK;
  *DELAYout = vmeRead8(&hl.dev->delay) & HELI_DELAY_MASK;
  *PATTERNout = vmeRead8(&hl.dev->pattern) & HELI_PATTERN_MASK;
  *CLOCKout = vmeRead8(&hl.dev->clock) & HELI_CLOCK_MASK;
  HUNLOCK;

  return 0;
}

/**
 * @brief Set the line sync mode
 * @details Set the line sync mode
 * @param[in] CLOCKs Line Sync Option
 *            0 : 30 Hz Line Sync
 *            1 : 120 Hz Line Sync
 *            2 : 240 Hz Line Sync
 *            3 : Free Clock
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetMode(uint32_t CLOCKs)
{
  CHECKHELI;

  if(CLOCKs > 4)
    {
      HELI_ERR("Invalid CLOCKs (%d)\n", CLOCKs);
      return -1;
    }

  HLOCK;
  uint8_t masked = vmeRead8(&hl.dev->clock) & ~0x3; /* Keep the any other settings (BOARDCLOCKd) */
  vmeWrite8(&hl.dev->clock, CLOCKs | masked);
  HUNLOCK;

  return 0;
}

/**
 * @brief Return the line sync status
 * @details Return the line sync status
 * @param[inout] CLOCKd Line Sync Status
 *            0 : 30 Hz Line Sync
 *            1 : 120 Hz Line Sync
 *            2 : 240 Hz Line Sync
 *            3 : Free Clock
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetMode(uint32_t *CLOCKd)
{
  CHECKHELI;

  HLOCK;
  *CLOCKd = vmeRead8(&hl.dev->clock) & 0x3;
  HUNLOCK;

  return 0;
}

/**
 * @brief Set the helicity pattern
 * @details Set the helicity pattern
 * @param[in] PATTERNs Helicity Pattern mode
 *               0 : Pair
 *               1 : Quartet
 *               2 : Octet
 *               3 : Toggle
 *               4 : Hexo-Quad
 *               5 : Octo-Quad
 *               6 : SPARE [Toggle]
 *               7 : SPARE [Toggle]
 *               8 : Thue-Morse-64
 *               9 : 16-Quad
 *              10 : 32-Pair
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetHelicityPattern(uint32_t PATTERNs)
{
  CHECKHELI;

  if(PATTERNs > 10)
    {
      HELI_ERR("Invalid PATTERNs (%d)\n", PATTERNs);
      return -1;
    }

  HLOCK;
  vmeWrite8(&hl.dev->pattern, PATTERNs);
  HUNLOCK;

  return 0;
}

/**
 * @brief Get the helicity pattern setting
 * @details Get the helicity pattern setting
 * @param[out] PATTERNd Helicity Pattern setting
 *               0 : Pair
 *               1 : Quartet
 *               2 : Octet
 *               3 : Toggle
 *               4 : Hexo-Quad
 *               5 : Octo-Quad
 *               6 : SPARE [Toggle]
 *               7 : SPARE [Toggle]
 *               8 : Thue-Morse-64
 *               9 : 16-Quad
 *              10 : 32-Pair
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetHelicityPattern(uint32_t *PATTERNd)
{
  CHECKHELI;

  HLOCK;
  *PATTERNd = vmeRead8(&hl.dev->pattern) & HELI_PATTERN_MASK;
  HUNLOCK;

  return 0;
}

/**
 * @brief Set the helicity reporting delay
 * @details Set the helicity reporting delay
 * @param[in] DELAYs Helicity Reporting Delay setting
 *             0 : No delay
 *             1 : 1 window
 *             2 : 2 windows
 *             3 : 4 windows
 *             4 : 8 windows
 *             5 : 16 windows
 *             6 : 24 windows
 *             7 : 32 windows
 *             8 : 40 windows
 *             9 : 48 windows
 *            10 : 64 windows
 *            11 : 72 windows
 *            12 : 96 windows
 *            13 : 112 windows
 *            14 : 128 windows
 *            15 : 256 windows
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetReportingDelay(uint32_t DELAYs)
{
  CHECKHELI;

  if(DELAYs > 10)
    {
      HELI_ERR("Invalid DELAYs (%d)\n", DELAYs);
      return -1;
    }

  HLOCK;
  vmeWrite8(&hl.dev->delay, DELAYs);
  HUNLOCK;

  return 0;
}

/**
 * @brief Get the helicity reporting delay
 * @details Get the helicity reporting delay
 * @param[out] DELAYd Helicity Reporting Delay setting
 *             0 : No delay
 *             1 : 1 window
 *             2 : 2 windows
 *             3 : 4 windows
 *             4 : 8 windows
 *             5 : 16 windows
 *             6 : 24 windows
 *             7 : 32 windows
 *             8 : 40 windows
 *             9 : 48 windows
 *            10 : 64 windows
 *            11 : 72 windows
 *            12 : 96 windows
 *            13 : 112 windows
 *            14 : 128 windows
 *            15 : 256 windows
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetReportingDelay(uint32_t *DELAYd)
{
  CHECKHELI;

  HLOCK;
  *DELAYd = vmeRead8(&hl.dev->delay) & HELI_DELAY_MASK;
  HUNLOCK;

  return 0;
}

/**
 * @brief Get the timing parameters of the helicity window
 * @details Get the timing parameters of the helicity window
 * @param[out] fTSettleReadbackVal TSettle [usec]
 * @param[out] fTStableReadbackVal TStable [usec]
 * @param[out] fFreqReadback Helicity Board Frequency (Hz)
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetHelcityTiming(double *fTSettleReadbackVal, double *fTStableReadbackVal, double *fFreqReadback)
{
  CHECKHELI;

  uint8_t iClockReadback, iTSettleReadback, iTStableReadback;

  HLOCK;
  iClockReadback = vmeRead8(&hl.dev->clock) & 0x3;
  iTSettleReadback = vmeRead8(&hl.dev->tsettle) & HELI_TSETTLE_MASK;
  iTStableReadback = vmeRead8(&hl.dev->tstable) & HELI_TSTABLE_MASK;

  /* get tsettle time */
  *fTSettleReadbackVal = fTSettleVals[iTSettleReadback];

  /* get tstable time and frequency */
  switch (iClockReadback)
    {
    case 0:
      *fFreqReadback = 30.0;
      *fTStableReadbackVal = ((1.0 / *fFreqReadback) * 1000000.0) - *fTSettleReadbackVal;
      break;

    case 1:
      *fFreqReadback = 120.0;
      *fTStableReadbackVal = ((1.0 / *fFreqReadback) * 1000000.0) - *fTSettleReadbackVal;
      break;

    case 2:
      *fFreqReadback = 240.0;
      *fTStableReadbackVal = ((1.0 / *fFreqReadback) * 1000000.0) - *fTSettleReadbackVal;
      break;

    default:	/* free clock */
      *fTStableReadbackVal = fTStableVals[iTStableReadback];
      *fFreqReadback = (1.0 / (*fTSettleReadbackVal + *fTStableReadbackVal)) * 1000000.0;
      break;
    }

  HUNLOCK;

  return 0;
}

/**
 * @brief Get the frequency of the helicity signal
 * @details Get the frequency of the helicity signal
 * @param[out] FREQ Frequency of Helicity signal (Hz)
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetHelicityBoardFrequency(double *FREQ)
{
  double ftsettle, ftstable, ffreq;
  int32_t rval = heliGetHelcityTiming(&ftsettle, &ftstable, &ffreq);

  if(rval < 0)
    return -1;

  *FREQ = ffreq;

  return 0;
}

/**
 * @brief Print Allowed values for TSettle to standard out
 * @details Print Allowed values for TSettle to standard out
 */
void
heliPrintTSettle()
{
  printf("  Index    TSettle [usec]     Index    TSettle [usec]\n");
  int32_t i;
  for(i = 0; i < 16; i++)
    printf("     %2d   %8.f               %2d   %8.f\n", i, fTSettleVals[i],
	   i+16, fTSettleVals[i+16]);
}

/**
 * @brief Set TSettle for the helicity signal
 * @details Set TSettle for the helicity signal
 * @param[in] TSETTLEs TSettle index from these values
 * Index    TSettle [usec]     Index    TSettle [usec]
 *     0          5               16        120
 *     1         10               17        130
 *     2         15               18        140
 *     3         20               19        150
 *     4         25               20        160
 *     5         30               21        170
 *     6         35               22        180
 *     7         40               23        190
 *     8         45               24        200
 *     9         50               25        250
 *    10         60               26        300
 *    11         70               27        350
 *    12         80               28        400
 *    13         90               29        450
 *    14        100               30        500
 *    15        110               31       1000
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetTSettle(uint8_t TSETTLEs)
{
  CHECKHELI;

  if(TSETTLEs > 32)
    {
      HELI_ERR("Invalid TSETTLEs (%d)\n", TSETTLEs);
      return -1;
    }

  HLOCK;
  vmeWrite8(&hl.dev->tsettle, TSETTLEs);
  HUNLOCK;

  return 0;
}

/**
 * @brief Get the TSettle for the helicity signal
 * @details Get the TSettle for the helicity signal
 * @param[out] TSETTLEd Value of TSettle [usec]
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetTSettle(double *TSETTLEd)
{
  double ftsettle, ftstable, ffreq;
  int32_t rval = heliGetHelcityTiming(&ftsettle, &ftstable, &ffreq);

  if(rval < 0)
    return -1;

  *TSETTLEd = ftsettle;

  return 0;
}

/**
 * @brief Print Allowed values for TStable to standard out
 * @details Print Allowed values for TStable to standard out
 */
void
heliPrintTStable()
{
  printf("  Index    TStable [usec]\n");
  int32_t i;
  for(i = 0; i < 32; i++)
    printf("     %2d   %8.2f\n", i, fTStableVals[i]);
}

/**
 * @brief Set TStable of the helicity signal
 * @details Set TStable of the helicity signal
 * @param[in] TSTABLEs Index of TStable
 * Index    TStable [usec]     Index    TStable [usec]
 *     0     240.40               16    1000.00
 *     1     245.40               17    1001.65
 *     2     250.40               18    1318.90
 *     3     255.40               19    1348.90
 *     4     470.85               20    2000.00
 *     5     475.85               21    3000.00
 *     6     480.85               22    4066.65
 *     7     485.85               23    5000.00
 *     8     490.85               24    6000.00
 *     9     495.85               25    7000.00
 *    10     500.85               26    8233.35
 *    11     505.85               27    8243.35
 *    12     510.85               28   16567.00
 *    13     515.85               29   16667.00
 *    14     900.00               30   33230.00
 *    15     971.65               31   33330.00
 * @return 0 if successful, otherwise -1
 */
int32_t
heliSetTStable(uint8_t TSTABLEs)
{
  CHECKHELI;

  if(TSTABLEs > 32)
    {
      HELI_ERR("Invalid TSTABLEs (%d)\n", TSTABLEs);
      return -1;
    }

  HLOCK;
  vmeWrite8(&hl.dev->tstable, TSTABLEs);
  HUNLOCK;

  return 0;
}

/**
 * @brief Get the TStable for the helicity signal
 * @details Get the TStable for the helicity signal
 * @param[out] TSTABLEd Value of TStable [usec]
 * @return 0 if successful, otherwise -1
 */
int32_t
heliGetTStable(double *TSTABLEd)
{
  double ftsettle, ftstable, ffreq;
  int32_t rval = heliGetHelcityTiming(&ftsettle, &ftstable, &ffreq);

  if(rval < 0)
    return -1;

  *TSTABLEd = ftstable;

  return 0;
}

int32_t
heliGetBoardClock(uint32_t *BOARDCLOCKd)
{
  CHECKHELI;

  HLOCK;
  HUNLOCK;

  return 0;
}
