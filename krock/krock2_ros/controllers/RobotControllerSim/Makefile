###
### Standard Makefile for Webots controllers
###
### Platform: Windows, Mac OS X, Linux
### Language: C, C++ and Java
###
### Authors: Olivier Michel - www.cyberbotics.com
### Revised: Yvan Bourquin - September 30th, 2009.
###
### Uncomment the variables to customize the Makefile

### -----C Sources-----
###
### if your controller uses several C sources files:
# C_SOURCES = my_controller.c my_clever_algo.c my_gui.c

### -----C++ Sources-----
###
### if your controller uses several C++ sources files:
# CPP_SOURCES = my_controller.cpp my_clever_algo.cpp my_gui.cpp
###     or
# CC_SOURCES = my_controller.cc my_clever_algo.cc my_gui.cc

#MAKEFLAGS += --jobs=8

CTRL_HOME = $(shell pwd)
CPUS ?= $(shell grep -c ^processor /proc/cpuinfo)
#MAKEFLAGS += --jobs=$(CPUS)
CXX_SOURCES = 	source/robotSim.cpp main.cpp \
				source/controller.cpp

# source/controller_imageProcessing.cpp
#CFLAGS=-O2 -O3

### -----C/C++ Options----- source/controller_optimizationStuff.cpp
###
### if special CFLAGS are necessary, for example to set optimization level or
### to find include files:
# CFLAGS=-O3 -I/my_library_path/include
# CTRL_HOME = /home/thorvat/BioRobLab/WEBOTS/robot2/controllers/CppOnlineCntrl

INCLUDE_DIRS = -I$(CTRL_HOME)/header -I$(CTRL_HOME) 

DISABLE_WARNING_FLAGS = -Wno-unused-variable -Wno-unused-but-set-variable -Wno-strict-overflow -Wno-write-strings \
						-Wno-unused-value
CFLAGS= -g  $(INCLUDE_DIRS)  -std=c++11 -std=gnu++11 $(DISABLE_WARNING_FLAGS)
 
###
### if your controller needs additional libraries:
# LIBRARIES=-L/path/to/my/library -lmy_library -lmy_other_library
LIBRARIES= 
###
### if you want to use the C API in your C++ program add:
# USE_C_API=1

### -----Java Options-----
###
### if your Java controller needs additional libraries, you should define
### the CLASSPATH environment variable as explained in the Webots User Guide
# CLASSPATH=relative/mylib.jar

### Do not modify: this includes Webots global Makefile.include
WEBOTS_HOME:=/usr/local/webots
CC=/usr/bin/gcc
CXX=/usr/bin/g++
space :=
space +=
WEBOTS_HOME_PATH=$(subst $(space),\ ,$(strip $(subst \,/,$(WEBOTS_HOME))))
#include $(WEBOTS_HOME_PATH)/resources/projects/default/controllers/Makefile.include
#include /usr/local/webots-old/resources/projects/default/controllers/Makefile.include
include $(WEBOTS_HOME_PATH)/resources/Makefile.include
