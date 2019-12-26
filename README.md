# Nuclei N/NX Embedded Software Development Kit

**Nuclei N/NX Embedded Software Development Kit(Nuclei SDK)** is developed for developing and evaluating software using our FPGA evaluation board.

This Nuclei SDK is built based on the **NMSIS** framework, user can access all the APIs provided by **NMSIS** and also the APIs that provided by Nuclei SDK which mainly for on-board peripherals access such as GPIO, UART, SPI and I2C etc.

We also intergated two RTOSes into Nuclei SDK, which are **FreeRTOS** and **UCOSII**, you can easily find it in the *OS* folder.

## Requirements
* Ubuntu Linux 18.04 LTS
* GNU Make
* [Nuclei RISCV GNU GCC Toolchain](https://nucleisys.com/download.php)
* [Nuclei OpenOCD](https://nucleisys.com/download.php)

## Directory Structure

Here is the directory structure for this Nuclei SDK.

~~~
$NUCLEI_SDK_ROOT
├── application
│   ├── baremetal
│   ├── freertos
│   └── ucosii
├── Board
│   └── hbird
├── Build
│   ├── Makefile.base
│   ├── Makefile.board
│   ├── Makefile.board.hbird
│   ├── Makefile.conf
│   ├── Makefile.core
│   ├── Makefile.files
│   ├── Makefile.files.hbird
│   ├── Makefile.misc
│   ├── Makefile.os
│   ├── Makefile.os.FreeRTOS
│   ├── Makefile.os.UCOSII
│   └── Makefile.rules
├── NMSIS
│   └── Include
├── OS
│   ├── FreeRTOS
│   └── UCOSII
├── LICENSE
├── Makefile
└── setup.sh
~~~

* **application**

  This directory contains all the application softwares for this Nuclei SDK.

  The application code can be divided into mainly 3 parts, which are:
  - **Baremetal** applications, which will provide baremetal applications without any OS usage, these applications will be placed in *application/baremetal/* folder.
  - **FreeRTOS** applications, which will provide FreeRTOS applications using FreeRTOS RTOS, placed in *application/freertos/* folder.
  - **UCOSII** applications, which will provide UCOSII applications using UCOSII RTOS, placed in *application/ucosii/* folder.

* **Board**

  This directory contains all the supported boards for this Nuclei SDK.

  Here we mainly support Nuclei Hummingbird FPGA evaluation board, the support package placed in *Board/hbird/*.

* **Build**

  This directory contains the key part of the build system based on Makefile for Nuclei SDK.

* **NMSIS**

  This directory contains the NMSIS-Core header files, which is widely used in this Nuclei SDK.
  We will also sync the changes in NMSIS project when it provided a new release.

* **OS**

  This directory provided two RTOS package we suppported which are **FreeRTOS** and **UCOSII**.

* **LICENSE**

  Nuclei SDK license file.

* **Makefile**

  An external Makefile just for build, run, debug application without cd to any coresponding application directory, such as *application/baremetal/helloworld/*.

* **setup.sh**

  Nuclei SDK environment setup script. You need to create your own `setup_config.sh`.
  ~~~
  NUCLEI_TOOL_ROOT=/path/to/your_tool_root
  ~~~

  In the $NUCLEI_TOOL_ROOT, you need to have Nuclei RISC-V GNU GCC toolchain and OpenOCD installed as below.
  ~~~
  $NUCLEI_TOOL_ROOT
  ├── gcc
  │   ├── bin
  │   ├── include
  │   ├── lib
  │   ├── libexec
  │   ├── riscv-nuclei-elf
  │   └── share
  └── openocd
      ├── bin
      ├── contrib
      ├── distro-info
      ├── OpenULINK
      ├── scripts
      └── share
  ~~~

## How to use
1. Create and modify your own setup config `setup_config.sh` in $NUCLEI_SDK_ROOT.
2. Source the environment script: `source setup.sh`
3. Build and run application.
   * Assume that you will run *application/baremetal/helloworld/*.
   * cd *application/baremetal/helloworld/*
   * you can run *make help* to show help message.
   * We provided different Nuclei Core configurations(CORE=<your_core>) we supported, see *Build/Makefile.core*.
     - such as `CORE=n305`
   * We support three download modes(DOWNLOAD=<mode>) for different applications.
     - **flashxip**: Program will to be download into flash and run directly in Flash
     - **flash**: Program will be download into flash, when running, program will be copied to ilm/ram and run in ilm/ram
     - **ilm**: Program will be download into ilm/ram and run directly in ilm/ram, program lost when poweroff
   * For example, if you want to build your application for *CORE=n305 DOWNLOAD=ilm*, you can easily run this command:
     ~~~
     make CORE=n305 DOWNLOAD=ilm all
     ~~~
   * If you want to upload your application for *CORE=n305 DOWNLOAD=ilm*, you can easily run this command:
     ~~~
     make CORE=n305 DOWNLOAD=ilm upload
     ~~~
   * If you want to debug your application for *CORE=n305 DOWNLOAD=ilm*:
     - First open a new terminal in the same application folder and run: `make CORE=n305 DOWNLOAD=ilm run_openocd`
     - Then run this command `make CORE=n305 DOWNLOAD=ilm run_gdb` in the existing terminal, you need to type `load` to load the application again, and debug it.
   * If you want to use UART terminal tool to view the UART message, you can choose `screen` or `minicom`, the default UART baudrate we use is `115200`.

## Knowledge book
* If you need to build a new application, or change **CORE** or **DOWNLOAD** option, please make sure that you have clean the project by `make clean`

* If you want to specify additional compiler flags, please follow this guidance in your application Makefile.
  - Pass common compiler flags to all c/asm/cpp compiler, you can use **COMMON_FLAGS** in Makefile, such as `COMMON_FLAGS := -O3 -funroll-loops -fpeel-loops`
  - Pass C compiler only flags to c compiler, you can use **CFLAGS** in Makefile, such as `CFLAGS := -O3 -funroll-loops -fpeel-loops`
  - For ASM compiler only flags, you can use **ASMFLAGS**, for CPP compiler only flags, you can use **CXXFLAGS**.

* If you want to pass additional linker flags, you can use **LDFLAGS**, and if you have additional library directories, you can use **LIBDIRS** to specify library directories.

* The preprovided applications and its makefile is the best startup examples about how to use Nuclei SDK.