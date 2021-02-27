# Makefile for the EPICS V4 exampleCPP module

TOP = .
include $(TOP)/configure/CONFIG

EMBEDDED_TOPS += $(TOP)/testManyChannels
EMBEDDED_TOPS += $(TOP)/testClient
EMBEDDED_TOPS += $(TOP)/testMultiplePutGet

DIRS := configure

DIRS += $(EMBEDDED_TOPS)

DIRS += test
test_DEPEND_DIRS = configure

# No inter-module dependencies

include $(TOP)/configure/RULES_TOP
