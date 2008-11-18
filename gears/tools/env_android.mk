######################################################################
# OS == android
######################################################################
#
# The following environment variables are used to build Gears on
# Android:
#
# ANDROID_BUILD_TOP
#       This refers to the top level directory in the Android source
#       tree.
#
# HOST_OS (optional)
#       The host operating system, one of "linux", "darwin" or "windows".
#       This is normally automatically detected, but can be manually set
#       otherwise.
#
# ANDROID_PRODUCT_OUT (optional)
#       The output directory for the build. Generated native, managed
#       binaries, and data go in the tree under here. This defaults
#       to $ANDROID_BUILD_TOP/out/product/* if there is only one
#       built target.
#
# ANDROID_TOOLCHAIN (optional)
#       The full base path of the target toolchain. This is normally
#       found as the only "*arm*" toolchain in the Android source
#       prebuilt tree. If this cannot be determined, it can be
#       manually specified.
#
# This makefile include sets the following variables if not set
# manually by the user:
#
# HOST_OS
#       One of: "linux", "darwin", "windows"
#
# HOST_ARCH
#       One of: "x86", "ppc"
#
# ANDROID_TOOLCHAIN
#       The executable (bin) path to the target toolchain.
#
# ANDROID_PRODUCT_OUT
#       The path to the Android source tree output files for the built
#       product.
#
# CROSS_PREFIX
#       The path and partial command to prefix target toolchain
#       executables with, e.g $(CROSS_PREFIX)gcc.

ifeq ($(ANDROID_BUILD_TOP),)
$(error "Please set ANDROID_BUILD_TOP to the top level of your Android source.")
endif

ifeq ($(HOST_OS),)
# Set HOST_OS to linux, darwin or cygwin
# Translate cygwin -> windows
# Translate macintosh -> darwin
HOST_OS		:= $(shell uname -s | tr '[:upper:]' '[:lower:]')
ifeq ($(HOST_OS),cygwin)
HOST_OS		:= windows
endif
ifeq ($(HOST_OS),macintosh)
HOST_OS		:= darwin
endif
ifeq ($(HOST_OS),)
$(error "Unable to determine host OS. Please set HOST_OS manually.")
endif
endif  # !HOST_OS

ifeq ($(HOST_ARCH),)
# Set HOST_ARCH to x86 or ppc. The raw architecture may be a variant
# such as "x86_64".
RAW_HOST_ARCH	:= $(shell uname -m | tr '[:upper:]' '[:lower:]')
HOST_ARCH	:= unknown
ifneq (,$(findstring 86,$(RAW_HOST_ARCH)))
HOST_ARCH	:= x86
endif
ifneq (,$(findstring power,$(RAW_HOST_ARCH)))
HOST_ARCH	:= ppc
endif
ifeq ($(HOST_ARCH),)
$(error "Unable to determinate host arch. Please set HOST_ARCH manually.")
endif
endif  # !HOST_ARCH

ifeq ($(ANDROID_TOOLCHAIN),)
# Find the target toolchain. Assume ARM architecture output.
ANDROID_TOOLCHAIN := $(wildcard $(ANDROID_BUILD_TOP)/prebuilt/$(HOST_OS)-$(HOST_ARCH)/toolchain/arm*/bin)
# Check that there is exactly one toolchain this refers to.
ifeq ($(ANDROID_TOOLCHAIN),)
$(error "Cannot determine location of the target toolchain. Please set ANDROID_TOOLCHAIN manually.")
endif
ifneq ($(words $(ANDROID_TOOLCHAIN)),1)
$(error "Multiple target toolchains. Please set ANDROID_TOOLCHAIN manually.")
endif
endif  # !ANDROID_TOOLCHAIN

ifeq ($(ANDROID_PRODUCT_OUT),)
# Find the output directory. Assume the only target output directory.
ANDROID_PRODUCT_OUT := $(wildcard $(ANDROID_BUILD_TOP)/out/target/product/*)
# Check that there is exactly one build directory this refers to.
ifeq ($(ANDROID_PRODUCT_OUT),)
$(error "Cannot determine location of the Android build directory. Please set ANDROID_PRODUCT_OUT manually.")
endif
ifneq ($(words $(ANDROID_PRODUCT_OUT)),1)
$(error "Multiple target Android builds. Please set ANDROID_PRODUCT_OUT manually.")
endif
endif  #!ANDROID_PRODUCT_OUT

# Figure out the cross-compile prefix by finding the *-gcc executable
# and taking the '*' as the prefix for the rest.
CROSS_PREFIX	:= \
		$(shell ls $(ANDROID_TOOLCHAIN)/*-gcc | \
		sed 's|\(.*/.*\-\)gcc|\1|g')
