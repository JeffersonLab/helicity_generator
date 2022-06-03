/*
 * File:
 *    heliConfig
 *
 * Description:
 *    Configure the output of the helicity generator module
 *
 *
 */


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "jvme.h"
#include "heliLib.h"

char progName[128] = "heliConfig";

void
usage()
{
  printf("\nUsage: \n");
  printf("\t %s [TSETTLE TSTABLE DELAY PATTERN CLOCK]\n", progName);
  printf("Configure the helicity generator module with the provided arguments\n");
  printf("\n");
  printf("  [TSETTLE                    \n");
  printf("   TSTABLE                    \n");
  printf("   DELAY                    \n");
  printf("   PATTERN                    \n");
  printf("   CLOCK]                    \n");
  printf("\n");
  printf("Exit status:\n");
  printf("  0  if OK,\n");
  printf("  1  if argument ERROR\n");
  printf("  2  if VME Driver ERROR\n");
  printf("  3  if helicity generator library ERROR\n");
}

int32_t
main(int32_t argc, char *argv[])
{

  int32_t iarg = 0, stat = 0, rval = 0;
  uint32_t address = 0x00a00000; /* Current Firmware default address */
  uint8_t tsettle_set = 0, tstable_set = 0, delay_set = 0, pattern_set = 0, clock_set = 0;


  strncpy(progName, argv[0], 128);

  if( argc == 6 )
    {
      tsettle_set = (uint8_t) strtol(argv[++iarg], NULL, 16) & 0xFF;
      tstable_set = (uint8_t) strtol(argv[++iarg], NULL, 16) & 0xFF;
      delay_set   = (uint8_t) strtol(argv[++iarg], NULL, 16) & 0xFF;
      pattern_set = (uint8_t) strtol(argv[++iarg], NULL, 16) & 0xFF;
      clock_set   = (uint8_t) strtol(argv[++iarg], NULL, 16) & 0xFF;
    }
  else
    {
      usage();
      rval = 1;
      exit(rval);
    }

  stat = vmeOpenDefaultWindows();
  if (stat != OK)
    {
      printf("vmeOpenDefaultwindows failed: code 0x%08x\n",stat);
      rval = 2;
      exit(rval);
    }

  vmeCheckMutexHealth(1);
  vmeBusLock();

  stat = heliInit(address, HELI_INIT_DEBUG);
  if (stat != OK)
    {
      printf("heliInit failed: code 0x%08x\n",stat);
      rval = 3;
    }
  else
    {
      stat = heliConfigure(tsettle_set, tstable_set, delay_set, pattern_set, clock_set);
      if (stat != OK)
	{
	  printf("heliConfig failed: code 0x%08x\n",stat);
	  rval = 3;
	}

      heliStatus(1);
    }

  vmeBusUnlock();

  stat = vmeCloseDefaultWindows();
  if (stat != OK)
    {
      printf("vmeCloseDefaultWindows failed: code 0x%08x\n",stat);
      rval = 1;
    }

  exit(rval);
}

/*
  Local Variables:
  compile-command: "make -k heliConfig"
  End:
 */
