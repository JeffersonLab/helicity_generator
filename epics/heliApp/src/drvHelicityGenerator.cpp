/*
 * Copyright 2022, Jefferson Science Associates, LLC.
 * Subject to the terms in the LICENSE file found in the top-level directory.
 *
 *     Authors: Bryan Moffit
 *              moffit@jlab.org                   Jefferson Lab, MS-12B3
 *              Phone: (757) 269-5660             12000 Jefferson Ave.
 *              Fax:   (757) 269-5800             Newport News, VA 23606
 *
 * Description: Driver for JLab helicity generator module using asynPortDriver base class
 *
 */

#include <iocsh.h>
#include <asynPortDriver.h>

extern "C"
{
#include "jvme.h"
#include "heliLib.h"
}

#include <epicsExport.h>

static const char *driverName = "HelicityGenerator";

// Analog output parameters
#define tsettleSetString    "TSETTLE_SET"
#define tstableSetString    "TSTABLE_SET"
#define delaySetString      "DELAY_SET"
#define patternSetString    "PATTERN_SET"
#define clockSetString      "CLOCK_SET"

#define MAX_SIGNALS 10

/** Class definition for the HelicityGenerator class
 */
class HelicityGenerator : public asynPortDriver {

public:
  HelicityGenerator(const char *portName, int32_t boardNum);

  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);

protected:
  int32_t P_tsettleSet;
  int32_t P_tstableSet;
  int32_t P_delaySet;
  int32_t P_patternSet;
  int32_t P_clockSet;

private:
  int32_t P_boardNum;

};

/** Constructor for the HelicityGenerator class
 * The only instantiation is done by the IOC at startup
 */
HelicityGenerator::HelicityGenerator(const char* portName,int32_t boardNum)
  : asynPortDriver(portName, MAX_SIGNALS, //NUM_PARAMS, deprecated
		   asynInt32Mask | asynDrvUserMask,
		   0,
		   ASYN_CANBLOCK, 1,
		   0, 0),
    P_boardNum(boardNum)
{
  createParam(tsettleSetString, asynParamInt32, &P_tsettleSet);

  vmeOpenDefaultWindows();
  heliInit(0xa00000, 0);
}

asynStatus
HelicityGenerator::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  // Module registers are 8 bit numbers
  *low = 0;

  if(function == P_tsettleSet)
    *high = HELI_TSETTLE_MASK;

  else if(function == P_tstableSet)
    *high = HELI_TSTABLE_MASK;

  else if(function == P_delaySet)
    *high = HELI_DELAY_MASK;

  else if(function == P_patternSet)
    *high = HELI_PATTERN_MASK;

  else if(function == P_clockSet)
    *high = HELI_CLOCK_MASK;

  else
    return(asynError);

  return(asynSuccess);
}

asynStatus
HelicityGenerator::writeInt32(asynUser *pasynUser, epicsInt32 value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  static const char *functionName = "writeInt32";
  epicsInt32 tsettle = 0, tstable = 0, delay = 0, pattern = 0, clock = 0;

  this->getAddress(pasynUser, &addr);
  getIntegerParam(addr, P_tsettleSet, &tsettle);
  getIntegerParam(addr, P_tstableSet, &tstable);
  getIntegerParam(addr, P_delaySet, &delay);
  getIntegerParam(addr, P_patternSet, &pattern);
  getIntegerParam(addr, P_clockSet, &clock);

  setIntegerParam(addr, function, value);

  if(function == P_tsettleSet)
    {
      tsettle = value;
      tstable = 0;
      delay = 0;
      pattern = 0;
      clock = 0;
    }
  else if(function == P_tstableSet)
    {
      tstable = value;
    }
  else if(function == P_delaySet)
    {
      delay = value;
    }
  else if(function == P_patternSet)
    {
      pattern = value;
    }
  else if(function == P_clockSet)
    {
      clock = value;
    }

  vmeBusLock();
  status = heliConfigure((uint8_t) tsettle, (uint8_t) tstable,
			 (uint8_t) delay, (uint8_t) pattern, (uint8_t) clock);
  vmeBusUnlock();

  callParamCallbacks(addr);
  if (status == 0) {
    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
	      "%s:%s, port %s, wrote %d to address %d\n",
	      driverName, functionName, this->portName, value, addr);
  } else {
    asynPrint(pasynUser, ASYN_TRACE_ERROR,
	      "%s:%s, port %s, ERROR writing %d to address %d, status=%d\n",
	      driverName, functionName, this->portName, value, addr, status);
  }
  return (status==0) ? asynSuccess : asynError;
}

/** Configuration command, called directly or from iocsh */
extern "C" int HelicityGeneratorConfig(const char *portName, int boardNum)
{
  HelicityGenerator *pHelicityGenerator = new HelicityGenerator(portName, boardNum);
  pHelicityGenerator = NULL;  /* This is just to avoid compiler warnings */
  return(asynSuccess);
}


static const iocshArg configArg0 = { "Port name",      iocshArgString};
static const iocshArg configArg1 = { "Board number",      iocshArgInt};
static const iocshArg * const configArgs[] = {&configArg0,
                                              &configArg1};
static const iocshFuncDef configFuncDef = {"HelicityGeneratorConfig", 2, configArgs};
static void configCallFunc(const iocshArgBuf *args)
{
  HelicityGeneratorConfig(args[0].sval, args[1].ival);
}

void drvHelicityGeneratorRegister(void)
{
  iocshRegister(&configFuncDef,configCallFunc);
}

extern "C" {
  epicsExportRegistrar(drvHelicityGeneratorRegister);
}
