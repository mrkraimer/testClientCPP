# testClientCPP

This contains tests to show problems in the C++ code for EPICS V4.

## Building

If a proper RELEASE.local file exists one directory levels above **testClientCPP**, then just type:

    make

It can also be built by:

    cp configure/ExampleRELEASE.local configure/RELEASE.local
    edit file configure/RELEASE.local
    make

In **configure/RELEASE.local** it may only be necessary to change the definitions
of **EPICS4_DIR** and **EPICS_BASE**.




