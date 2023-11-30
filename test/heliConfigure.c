/*
 * File:
 *    heliConfigure.c
 *
 * Description:
 *    Configure the output of the helicity generator module
 *
 *
 */

#define HELICITY_GENERATOR_ADDRESS 0xA00000

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include "jvme.h"
#include "heliLib.h"

char progName[128];
int Verbose=0;

enum doBits
  {
    DO_CLOCK      = 1 << 0,
    DO_PATTERN    = 1 << 1,
    DO_DELAY      = 1 << 2,
    DO_TSETTLE    = 1 << 3,
    DO_TSTABLE    = 1 << 4,
    DO_BOARDCLOCK = 1 << 5,
    SHOW          = 1 << 6
  };

/* this structure holds the user arguments */
typedef struct
{
  uint8_t CLOCKs;
  uint8_t PATTERNs;
  uint8_t DELAYs;
  uint32_t TSETTLEs;
  uint32_t TSTABLEs;
  uint32_t BOARDCLOCKs;
} argValue_t;

void
usage()
{
  printf("\nUsage: \n");
  printf("\t %s [options]\n", progName);
  printf("Configure the helicity generator module with the provided arguments\n");
  printf("\n");
  printf(" -m, --mode {index}                select the clock mode\n");
  printf(" -p, --pattern {index}             select the helicity pattern\n");
  printf(" -d, --delay {index}               select the helicity delay\n");
  printf(" -t, --tsettle {index}             select the tsettle\n");
  printf(" -s, --tstable {index}             select the tstable\n");
  printf(" -b, --boardclock {index}          select the board clock output\n");
  printf("     --show {selections}           show the available selections for {selections}\n");
  printf("                                   (e.g. --show mode,pattern,tstable)\n");
  printf(" -h, --help                        this help message\n");
  printf("\n");
  printf("Exit status:\n");
  printf("  0  if OK,\n");
  printf("  1  if argument ERROR\n");
  printf("  2  if VME Driver ERROR\n");
  printf("  3  if helicity generator library ERROR\n");
  printf("\n");
}

/* Search through the -show string, searching for helicity generator parameter names */
void
fillShowBits(char argString[], uint8_t *showBits)
{
  *showBits = 0;

  char parameter[6][256] = {
    "mode",
    "pattern",
    "delay",
    "tsettle",
    "tstable",
    "boardclock"
  };

  uint32_t iparam;
  for(iparam = 0; iparam < 6; iparam++)
    {
      if(strstr(argString, parameter[iparam]) != NULL)
	*showBits |= (1 << iparam);
    }

  if(*showBits > 0)
    *showBits |= SHOW;

}

/* parse the command line with getopt_long, return user arguments */
int32_t
parseArgs(int32_t argc, char *argv[], argValue_t *value, uint8_t *doBits)
{
  int32_t rval = 0;

  char showString[256];
  static struct option long_options[] =
  {
    /* {const char *name, int has_arg, int *flag, int val} */
    {"help",       no_argument,       0,        'h'},
    {"verbose",    no_argument,       &Verbose, 1},
    {"mode",       required_argument, 0,        'm'},
    {"pattern",    required_argument, 0,        'p'},
    {"delay",      required_argument, 0,        'd'},
    {"tsettle",    required_argument, 0,        't'},
    {"tstable",    required_argument, 0,        's'},
    {"boardclock", required_argument, 0,        'b'},
    {"show",       required_argument, 0,        'l'},
    {0, 0, 0, 0}
  };

  /* Initialize output */
  memset(value, 0, sizeof(*value));
  *doBits = 0;

  while(1)
    {
      int opt_param, option_index = 0;
      option_index = 0;
      opt_param = getopt_long (argc, argv, "h:",
			       long_options, &option_index);

      if (opt_param == -1) /* No more option parameters left */
	break;

      switch (opt_param)
	{
	case 0:
	  break;

	case 'm': /* Mode */
	  *doBits |= DO_CLOCK;
	  value->CLOCKs = strtol(optarg, NULL, 10);
	  break;

	case 'p': /* PATTERN */
	  *doBits |= DO_PATTERN;
	  value->PATTERNs = strtol(optarg, NULL, 10);
	  break;

	case 'd': /* DELAY */
	  *doBits |= DO_DELAY;
	  value->DELAYs = strtol(optarg, NULL, 10);
	  break;

	case 't': /* TSETTLE */
	  *doBits |= DO_TSETTLE;
	  value->TSETTLEs = strtol(optarg, NULL, 10);
	  break;

	case 's': /* TSTABLE */
	  *doBits |= DO_TSTABLE;
	  value->TSTABLEs = strtol(optarg, NULL, 10);
	  break;

	case 'b': /* BOARDCLOCK */
	  *doBits |= DO_BOARDCLOCK;
	  value->BOARDCLOCKs = strtol(optarg, NULL, 10);
	  break;

	case 'l': /* show */
	  fillShowBits(optarg, doBits);
	  break;

	case 'h': /* help */
	case '?': /* Invalid Option */
	default:
	  usage();
	  rval = 1;
	}
    }
  if(*doBits == 0)
    {
      usage();
      rval = -1;
    }


  return rval;
}

/* function to show available selections of the helicity generator */

void
helicity_generator_show_selections(uint8_t showMask)
{

  if(showMask & DO_CLOCK)
    {
      printf(" Mode Selections:\n");
      heliPrintModeSelections();
    }

  if(showMask & DO_PATTERN)
    {
      printf(" Helicity Pattern Selections:\n");
      heliPrintHelicityPatternSelections();
    }

  if(showMask & DO_DELAY)
    {
      printf(" Delay Selections:\n");
      heliPrintReportingDelaySelections();
    }

  if(showMask & DO_TSETTLE)
    {
      printf(" TSettle Selections:\n");
      heliPrintTSettleSelections();
    }

  if(showMask & DO_TSTABLE)
    {
      printf(" TStable Selections:\n");
      heliPrintTStableSelections();
    }

  if(showMask & DO_BOARDCLOCK)
    {
      printf(" Board Clock Selections:\n");
      heliPrintBoardClockSelections();
    }

}

/* function to apply the user selections to the helicity generator */

int32_t
helicity_generator_set(uint8_t setMask, argValue_t args)
{
  int32_t rval = 0;

  if(setMask & DO_CLOCK)
    {
      printf("Select Mode %d\n", args.CLOCKs);
      rval = heliSelectMode(args.CLOCKs);
    }

  if(setMask & DO_PATTERN)
    {
      printf("Select Helicity Pattern %d\n", args.PATTERNs);
      rval |= heliSelectHelicityPattern(args.PATTERNs);
    }

  if(setMask & DO_DELAY)
    {
      printf("Select Reporting Delay %d\n", args.DELAYs);
      rval |= heliSelectReportingDelay(args.DELAYs);
    }

  if(setMask & DO_TSETTLE)
    {
      printf("Select TSettle %d\n", args.TSETTLEs);
      rval |= heliSelectTSettle(args.TSETTLEs);
    }

  if(setMask & DO_TSTABLE)
    {
      printf("Select TStable %d\n", args.TSTABLEs);
      rval |= heliSelectTStable(args.TSTABLEs);
    }

  if(setMask & DO_BOARDCLOCK)
    {
      printf("Select Board Clock %d\n", args.BOARDCLOCKs);
      rval |= heliSelectBoardClock(args.BOARDCLOCKs);
    }

  return rval;
}

/************************************************************
 *  MAIN
 */

int32_t
main(int32_t argc, char *argv[])
{
  int32_t iarg = 0, stat = 0, rval = 0;

  strncpy(progName, argv[0], 128);

  argValue_t setting = {0,0,0,0,0,0}; uint8_t doBits = 0;

  if(parseArgs(argc, argv, &setting, &doBits) < 0)
    exit(1);

  stat = vmeOpenDefaultWindows();
  if (stat != OK)
    {
      printf("vmeOpenDefaultwindows failed: code 0x%08x\n",stat);
      rval = 2;
      exit(rval);
    }

  vmeCheckMutexHealth(1);
  vmeBusLock();
  printf("\n");

  stat = heliInit(HELICITY_GENERATOR_ADDRESS, HELI_INIT_DEBUG);
  if (stat != OK)
    {
      printf("heliInit failed: code 0x%08x\n",stat);
      rval = 3;
      goto CLOSE;
    }

  if(doBits & SHOW)
    {
      helicity_generator_show_selections(doBits);
    }
  else
    {
      stat = helicity_generator_set(doBits, setting);

      if (stat != OK)
	{
	  printf("ERROR: code 0x%08x\n",stat);
	  rval = 3;
	}
      heliStatus(1);
    }


 CLOSE:
  vmeBusUnlock();

  stat = vmeCloseDefaultWindows();
  if (stat != OK)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",stat);
      if(rval == 0) rval = 1;
    }

  exit(rval);
}

/*
  Local Variables:
  compile-command: "make -k heliConfigure "
  End:
*/
