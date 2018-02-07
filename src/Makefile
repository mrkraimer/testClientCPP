TOP=..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE


PROD_HOST += monitorme
monitorme_SRCS += monitorme.cpp
monitorme_LIBS += pvaClient pvAccess pvAccessCA pvData ca Com


PROD_HOST += monitor
monitor_SRCS += monitor.cpp
monitor_LIBS += pvaClient pvAccess pvAccessCA pvData ca Com

PROD_HOST += monitormerate
monitormerate_SRCS += monitormerate.cpp
monitormerate_LIBS += pvaClient pvAccess pvAccessCA pvData ca Com


PROD_HOST += monitorrate
monitorrate_SRCS += monitorrate.cpp
monitorrate_LIBS += pvaClient pvAccess pvAccessCA pvData ca Com


#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

