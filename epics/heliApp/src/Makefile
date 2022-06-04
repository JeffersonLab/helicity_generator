TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

# build an ioc application

PROD_IOC = heli
# heli.dbd will be created and installed
DBD += heli.dbd

heli_DBD += base.dbd

# heli_registerRecordDeviceDriver.cpp will be created from heli.dbd
heli_SRCS += heli_registerRecordDeviceDriver.cpp
heli_SRCS_DEFAULT += heliMain.cpp
heli_SRCS_vxWorks += -nil-

# Add vxWorks support stuff
heli_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Add the support libraries needed by this application
#heli_LIBS += asyn

# To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file:
ifneq ($(SNCSEQ),)
    heli_DBD += sncExample.dbd
    heli_SRCS += sncExample.stt
    heli_LIBS += seq pv
endif

# Finally include the libraries from base
heli_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

