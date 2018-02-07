# testClientCPP

This contains code similar to:

1) pvAccessCPP/examples/monitorme.cpp
The example does not trap signals and does not use a WorkQueue.
It terminates when the enter key is pressed.

2) exampleCPP/exampleClient/src/monitor.cpp
The example terminates when the enter key is pressed.
It produces output like monitorme rather that like the original monitor.

In addition this project has code named monitormerate and monitorrate.
These are used to measure performance in events per second.


## Building

If a proper RELEASE.local file exists one directory levels above **testClientCPP**, then just type:

    make

It can also be built by:

    cp configure/ExampleRELEASE.local configure/RELEASE.local
    edit file configure/RELEASE.local
    make

In **configure/RELEASE.local** it may only be necessary to change the definitions
of **EPICS4_DIR** and **EPICS_BASE**.

## Starting IOCs for test

There are files named fast1.db, ..., fast4.db.
These are databases for use with **softIocPVA** that is provided with pvAccessCPP.
This creates an IOC that has DBRecords and also provider **qsrv**.

To use these a softlink can be created to **softIocPVA**.
For example I created  soft link as follows:

    ln -s /home/epicsv4/masterCPP/pva2pva/bin/linux-x86_64/softIocPVA softIocPVA

Then each can be used to create an IOC.
For example:

     ./softIocPVA -d fast1.db

Similarly IOCs using fast2.db, fast3.db, and fast4.db can be started.

## Performance

File performResult shows performance results.





