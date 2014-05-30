A framework to support multiple projects
========================================

ChibiOS
-------

Initialise once:
```
 git submodule init
```
And make sure it's up to date:
```
 git submodule update
```

Projects
--------

The project supports multiple boards and multiple projects.

Project RuuviTracker
--------------------

RuuviTracker projects supports only RuuviTracker C2 board. To compile it, you must first edit the settings in the beginning of src/main-tracker.c and compile it with the following command:
```
 make SC_PROJECT_CONFIG=projects/rtc2_tracker.mk -j4
```

Project Test
------------
To build the generic "test" project for STM32 F4 Discovery, run:
```
 make SC_PROJECT_CONFIG=projects/f4_discovery_test.mk -j4

```

Project Power
-------------
To build the sleep mode test project for STM32 F4 Discovery, run:
```
 make SC_PROJECT_CONFIG=projects/f4_discovery_power.mk -j4

```
