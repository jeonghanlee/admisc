TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#==================================================
# build a support library

LIBRARY_IOC += admisc

# xxxRecord.h will be created from xxxRecord.dbd
#DBDINC += xxxRecord
# install admisc.dbd into <top>/dbd
#DBD += NDFileAscii.dbd
DBD += NDPluginFits.dbd
#DBD += NDPluginSource.dbd
#DBD += NDPluginImaging.dbd

# specify all source files to be compiled and added to the library
#admisc_SRCS += NDFileAscii.cpp
admisc_SRCS += NDPluginFits.cpp
#admisc_SRCS += NDPluginSource.cpp
#admisc_SRCS += NDPluginImaging.cpp

# support sources
# lmfit-7.0 (http://apps.jcns.fz-juelich.de/src/lmfit/)
admisc_SRCS += lmcurve.c lmmin.c

include $(ADCORE)/ADApp/commonLibraryMakefile

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

