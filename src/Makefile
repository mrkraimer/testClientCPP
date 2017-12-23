TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE


PROD_HOST += putNoBlock
putNoBlock_SRCS += putNoBlock.cpp
putNoBlock_LIBS += pvaClient pvAccess pvData ca Com

PROD_HOST += putBlock
putBlock_SRCS += putBlock.cpp
putBlock_LIBS += pvaClient pvAccess pvData ca Com


PROD_HOST += putMany
putMany_SRCS += putMany.cpp
putMany_LIBS += pvaClient pvAccess pvData ca Com

PROD_HOST += testPvaClientPut
testPvaClientPut_SRCS += testPvaClientPut.cpp
testPvaClientPut_LIBS += pvaClient pvAccess pvData ca Com

PROD_HOST += monitorme
monitorme_SRCS += monitorme.cpp
monitorme_LIBS += pvaClient pvAccess pvAccessCA pvData ca Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

