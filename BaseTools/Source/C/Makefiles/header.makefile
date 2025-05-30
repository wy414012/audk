## @file
#
# The makefile can be invoked with
# HOST_ARCH = x86_64 or x64 for EM64T build
# HOST_ARCH = ia32 or IA32 for IA32 build
# HOST_ARCH = Arm or ARM for ARM build
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

EDK2_PATH ?= $(MAKEROOT)/../../..
EDK2_OBJPATH ?= $(MAKEROOT)/obj/edk2

ifndef HOST_ARCH
  #
  # If HOST_ARCH is not defined, then we use 'uname -m' to attempt
  # try to figure out the appropriate HOST_ARCH.
  #
  uname_m = $(shell uname -m)
  $(info Attempting to detect HOST_ARCH from 'uname -m': $(uname_m))
  ifneq (,$(strip $(filter $(uname_m), x86_64 amd64)))
    HOST_ARCH=X64
  endif
  ifeq ($(patsubst i%86,IA32,$(uname_m)),IA32)
    HOST_ARCH=IA32
  endif
  ifneq (,$(findstring aarch64,$(uname_m)))
    HOST_ARCH=AARCH64
  else ifneq (,$(findstring arm64,$(uname_m)))
    HOST_ARCH=AARCH64
  else ifneq (,$(findstring arm,$(uname_m)))
    HOST_ARCH=ARM
  endif
  ifneq (,$(findstring riscv64,$(uname_m)))
    HOST_ARCH=RISCV64
  endif
  ifneq (,$(findstring loongarch64,$(uname_m)))
    HOST_ARCH=LOONGARCH64
  endif
  ifndef HOST_ARCH
    $(info Could not detected HOST_ARCH from uname results)
    $(error HOST_ARCH is not defined!)
  endif
  $(info Detected HOST_ARCH of $(HOST_ARCH) using uname.)
endif

CYGWIN:=$(findstring CYGWIN, $(shell uname -s))
LINUX:=$(findstring Linux, $(shell uname -s))
DARWIN:=$(findstring Darwin, $(shell uname -s))
CLANG:=$(shell $(CC) --version | grep clang)
ifneq ($(CLANG),)
CC ?= $(CLANG_BIN)clang
CXX ?= $(CLANG_BIN)clang++
AS ?= $(CLANG_BIN)clang
AR ?= $(CLANG_BIN)llvm-ar
LD ?= $(CLANG_BIN)llvm-ld
else ifeq ($(origin CC),default)
CC = $(GCC_PREFIX)gcc
CXX = $(GCC_PREFIX)g++
AS = $(GCC_PREFIX)gcc
AR = $(GCC_PREFIX)ar
LD = $(GCC_PREFIX)ld
endif
LINKER ?= $(CC)
ifeq ($(HOST_ARCH), IA32)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/Ia32/

else ifeq ($(HOST_ARCH), X64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/X64/

else ifeq ($(HOST_ARCH), ARM)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/Arm/

else ifeq ($(HOST_ARCH), AARCH64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/AArch64/

else ifeq ($(HOST_ARCH), RISCV64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/RiscV64/

else ifeq ($(HOST_ARCH), LOONGARCH64)
ARCH_INCLUDE = -I $(EDK2_PATH)/MdePkg/Include/LoongArch64/

else
$(error Bad HOST_ARCH)
endif

INCLUDE = $(TOOL_INCLUDE) -I $(MAKEROOT) -I $(MAKEROOT)/Include/Common -I $(MAKEROOT)/Include/ -I $(MAKEROOT)/Include/IndustryStandard -I $(MAKEROOT)/Common/ -I .. -I . $(ARCH_INCLUDE)

INCLUDE += -I $(EDK2_PATH)/MdePkg/Include/ -I $(EDK2_PATH)/MdeModulePkg/Include/ -I $(EDK2_PATH)/OpenCorePkg/User/Include

EDK2_INCLUDE = -include $(MAKEROOT)/Include/Common/AutoGen.h

CPPFLAGS = $(INCLUDE)

# keep EXTRA_OPTFLAGS last
BUILD_OPTFLAGS = -O2 $(EXTRA_OPTFLAGS)

ifeq ($(DARWIN),Darwin)
# assume clang or clang compatible flags on OS X
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign -Wno-unused-result -nostdlib -g
else
ifneq ($(CLANG),)
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-self-assign \
-Wno-unused-result -nostdlib -g
else
CFLAGS = -MD -fshort-wchar -fno-strict-aliasing -fwrapv \
-fno-delete-null-pointer-checks -Wall -Werror \
-Wno-deprecated-declarations -Wno-stringop-truncation -Wno-restrict \
-Wno-unused-result -nostdlib -g
endif
endif
ifneq ($(CLANG),)
LDFLAGS =
CXXFLAGS = -Wno-deprecated-register -Wno-unused-result -std=c++14
else
LDFLAGS =
CXXFLAGS = -Wno-unused-result
endif

CPPFLAGS += -fshort-wchar -flto -DUSING_LTO
ifeq ($(HOST_ARCH), X64)
  CPPFLAGS += "-DEFIAPI=__attribute__((ms_abi))"
endif

#
# Provide fat binaries on Darwin
#
ifeq ($(DARWIN),Darwin)
  CFLAGS        += -arch x86_64 -arch arm64 -mmacosx-version-min=10.9
  CPPFLAGS      += -arch x86_64 -arch arm64 -mmacosx-version-min=10.9
  EXTRA_LDFLAGS += -arch x86_64 -arch arm64 -mmacosx-version-min=10.9
endif

CFLAGS += -DUEFI_IMAGE_FORMAT_SUPPORT_SOURCES=0x02

# keep BUILD_OPTFLAGS last
CFLAGS   += $(BUILD_OPTFLAGS)
CXXFLAGS += $(BUILD_OPTFLAGS)

# keep EXTRA_LDFLAGS last
LDFLAGS += $(EXTRA_LDFLAGS)

.PHONY: all
.PHONY: install
.PHONY: clean

all:

$(MAKEROOT)/libs:
	mkdir $(MAKEROOT)/libs

$(MAKEROOT)/bin:
	mkdir $(MAKEROOT)/bin
