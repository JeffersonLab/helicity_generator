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

// output parameters
#define tsettleOutString    "HELTSETTLEout"
#define tstableOutString    "HELTSTABLEout"
#define delayOutString      "HELDELAYout"
#define patternOutString    "HELPATTERNout"
#define clockOutString      "HELCLOCKout"

// input parameters
#define tsettleInString    "HELTSETTLEin"
#define tstableInString    "HELTSTABLEin"
#define delayInString      "HELDELAYin"
#define patternInString    "HELPATTERNin"
#define clockInString      "HELCLOCKin"

#define MAX_SIGNALS 20

/** Class definition for the HelicityGenerator class
 */
class HelicityGenerator : public asynPortDriver {

public:
  HelicityGenerator(const char *portName, int32_t boardNum);

  virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
  virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
  virtual asynStatus getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high);

protected:
  int32_t P_tsettleOut;
  int32_t P_tstableOut;
  int32_t P_delayOut;
  int32_t P_patternOut;
  int32_t P_clockOut;

  int32_t P_tsettleIn;
  int32_t P_tstableIn;
  int32_t P_delayIn;
  int32_t P_patternIn;
  int32_t P_clockIn;

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
  createParam(tsettleOutString, asynParamInt32, &P_tsettleOut);
  createParam(tstableOutString, asynParamInt32, &P_tstableOut);
  createParam(delayOutString, asynParamInt32, &P_delayOut);
  createParam(patternOutString, asynParamInt32, &P_patternOut);
  createParam(clockOutString, asynParamInt32, &P_clockOut);

  createParam(tsettleInString, asynParamInt32, &P_tsettleIn);
  createParam(tstableInString, asynParamInt32, &P_tstableIn);
  createParam(delayInString, asynParamInt32, &P_delayIn);
  createParam(patternInString, asynParamInt32, &P_patternIn);
  createParam(clockInString, asynParamInt32, &P_clockIn);

  vmeOpenDefaultWindows();
  heliInit(0xa00000, 0);
}

asynStatus
HelicityGenerator::getBounds(asynUser *pasynUser, epicsInt32 *low, epicsInt32 *high)
{
  int function = pasynUser->reason;

  // Module registers are 8 bit numbers
  *low = 0;

  if((function == P_tsettleOut) || (function == P_tsettleIn))
    *high = HELI_TSETTLE_MASK;

  else if((function == P_tstableOut) || (function == P_tstableIn))
    *high = HELI_TSTABLE_MASK;

  else if((function == P_delayOut) || (function == P_delayIn))
    *high = HELI_DELAY_MASK;

  else if((function == P_patternOut) || (function == P_patternIn))
    *high = HELI_PATTERN_MASK;

  else if((function == P_clockOut)  || (function == P_clockIn))
    *high = HELI_CLOCK_MASK;

  else
    return(asynError);

  return(asynSuccess);
}

asynStatus
HelicityGenerator::readInt32(asynUser *pasynUser, epicsInt32 *value)
{
  int addr;
  int function = pasynUser->reason;
  int status=0;
  uint8_t hwValue = 0;
  uint8_t tsettle = 0, tstable = 0, delay = 0, pattern = 0, clock = 0;
  static const char *functionName = "readInt32";

  this->getAddress(pasynUser, &addr);

  if((function == P_tsettleIn) || (function == P_tstableIn) || (function == P_delayIn) ||
     (function == P_patternIn) || (function == P_clockIn) )
    {
      // Get the values from HW
      vmeBusLock();
      status = heliGetSettings((uint8_t *) &tsettle, (uint8_t *) &tstable,
			       (uint8_t *) &delay, (uint8_t *) &pattern, (uint8_t *) &clock);
      vmeBusUnlock();

      // Set all parameters in asyn
      setIntegerParam(addr, P_tsettleIn, (epicsInt32) tsettle);
      setIntegerParam(addr, P_tstableIn, (epicsInt32) tstable);
      setIntegerParam(addr, P_delayIn, (epicsInt32) delay);
      setIntegerParam(addr, P_patternIn, (epicsInt32) pattern);
      setIntegerParam(addr, P_clockIn, (epicsInt32) clock);

      if(function == P_tsettleIn)
	{
	  *value = tsettle;
	}
      else if(function == P_tstableIn)
	{
	  *value = tstable;
	}
      else if(function == P_delayIn)
	{
	  *value = delay;
      }
      else if(function == P_patternIn)
      {
	*value = pattern;
      }
      else if(function == P_clockIn)
      {
	*value = clock;
      }

      if (status == 0)
	{
	  asynPrint(pasynUser, ASYN_TRACEIO_DRIVER,
		    "%s:%s, port %s, read %d from address %d\n",
		    driverName, functionName, this->portName, *value, addr);
	}
      else
	{
	  asynPrint(pasynUser, ASYN_TRACE_ERROR,
		    "%s:%s, port %s, ERROR reading from address %d, status=%d\n",
		    driverName, functionName, this->portName, addr, status);
	}
    }
  else
    {
      // Other functions we call the base class method
      status = asynPortDriver::readInt32(pasynUser, value);
    }

  callParamCallbacks(addr);
  return (status==0) ? asynSuccess : asynError;
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

  // Get the other parameters from asyn
  getIntegerParam(addr, P_tsettleOut, &tsettle);
  getIntegerParam(addr, P_tstableOut, &tstable);
  getIntegerParam(addr, P_delayOut, &delay);
  getIntegerParam(addr, P_patternOut, &pattern);
  getIntegerParam(addr, P_clockOut, &clock);

  // Set the value in asyn
  setIntegerParam(addr, function, value);

  if(function == P_tsettleOut)
    {
      tsettle = value;
    }
  else if(function == P_tstableOut)
    {
      tstable = value;
    }
  else if(function == P_delayOut)
    {
      delay = value;
    }
  else if(function == P_patternOut)
    {
      pattern = value;
    }
  else if(function == P_clockOut)
    {
      clock = value;
    }

  // Set the value in HW
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
