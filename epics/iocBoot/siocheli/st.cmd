#!../../bin/linux-x86_64/heli

## You may have to change heli to something else
## everywhere it appears in this file

< envPaths

cd ${TOP}

## Register all support components
dbLoadDatabase("dbd/heli.dbd")
heli_registerRecordDeviceDriver(pdbbase)

## Load record instances
dbLoadRecords("db/heli.db","user=moffitHost")

cd ${TOP}/iocBoot/${IOC}
iocInit()

## Start any sequence programs
#seq sncExample,"user=moffitHost"
