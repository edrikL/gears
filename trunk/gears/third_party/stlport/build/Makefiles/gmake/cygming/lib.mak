# -*- makefile -*- Time-stamp: <03/07/15 18:23:04 ptr>
# $Id: lib.mak 2384 2006-05-30 20:25:17Z dums $

ifeq (gcc, $(COMPILER_NAME))
LIB_PREFIX := lib
endif

LIB_PREFIX ?=

ifneq (bcc, $(COMPILER_NAME))
DBG_SUFFIX := g
else
DBG_SUFFIX := d
endif

STLDBG_SUFFIX := stl${DBG_SUFFIX}

ifdef STLP_BUILD_FORCE_STATIC_RUNTIME
LIB_TYPE := _x
else
LIB_TYPE := 
endif

ifdef STLP_BUILD_LIB_MOTIF
LIB_SUFFIX := _$(STLP_BUILD_LIB_MOTIF).${MAJOR}.${MINOR}
else
LIB_SUFFIX := .${MAJOR}.${MINOR}
endif

# Shared libraries:
SO_NAME_BASE := $(LIB_PREFIX)${LIBNAME}${LIB_TYPE}${LIB_SUFFIX}
SO_NAME        := ${SO_NAME_BASE}.$(SO)
LIB_NAME        := ${SO_NAME_BASE}.$(LIB)
#EXP_NAME        := ${SO_NAME_BASE}.$(EXP)

SO_NAME_OUT    := $(OUTPUT_DIR)/${SO_NAME}
LIB_NAME_OUT    := $(OUTPUT_DIR)/${LIB_NAME}
#EXP_NAME_OUT    := $(OUTPUT_DIR)/${EXP_NAME}

SO_NAME_DBG_BASE := $(LIB_PREFIX)${LIBNAME}${DBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}
SO_NAME_DBG    := ${SO_NAME_DBG_BASE}.$(SO)
LIB_NAME_DBG    := ${SO_NAME_DBG_BASE}.$(LIB)
#EXP_NAME_DBG    := ${SO_NAME_DBG_BASE}.$(EXP)

SO_NAME_OUT_DBG    := $(OUTPUT_DIR_DBG)/${SO_NAME_DBG}
LIB_NAME_OUT_DBG    := $(OUTPUT_DIR_DBG)/${LIB_NAME_DBG}
#EXP_NAME_OUT_DBG    := $(OUTPUT_DIR_DBG)/${EXP_NAME_DBG}

SO_NAME_STLDBG_BASE := $(LIB_PREFIX)${LIBNAME}${STLDBG_SUFFIX}${LIB_TYPE}${LIB_SUFFIX}
SO_NAME_STLDBG    := ${SO_NAME_STLDBG_BASE}.$(SO)
LIB_NAME_STLDBG    := ${SO_NAME_STLDBG_BASE}.$(LIB)
#EXP_NAME_STLDBG    := ${SO_NAME_STLDBG_BASE}.$(EXP)

SO_NAME_OUT_STLDBG    := $(OUTPUT_DIR_STLDBG)/${SO_NAME_STLDBG}
LIB_NAME_OUT_STLDBG    := $(OUTPUT_DIR_STLDBG)/${LIB_NAME_STLDBG}
#EXP_NAME_OUT_STLDBG    := $(OUTPUT_DIR_STLDBG)/${EXP_NAME_STLDBG}

# Static libraries:
ifdef STLP_BUILD_FORCE_DYNAMIC_RUNTIME
A_LIB_TYPE := _statix
else
A_LIB_TYPE := _static
endif

ifeq (gcc, $(COMPILER_NAME))
A_NAME := ${SO_NAME_BASE}.$(ARCH)
else
A_NAME := $(LIB_PREFIX)${LIBNAME}${A_LIB_TYPE}${LIB_SUFFIX}.$(ARCH)
endif
A_NAME_OUT := $(OUTPUT_DIR_A)/$(A_NAME)

ifeq (gcc, $(COMPILER_NAME))
A_NAME_DBG := ${SO_NAME_DBG_BASE}.$(ARCH)
else
A_NAME_DBG := $(LIB_PREFIX)${LIBNAME}${DBG_SUFFIX}${A_LIB_TYPE}${LIB_SUFFIX}.${ARCH}
endif
A_NAME_OUT_DBG := $(OUTPUT_DIR_A_DBG)/$(A_NAME_DBG)

ifeq (gcc, $(COMPILER_NAME))
A_NAME_STLDBG := ${SO_NAME_STLDBG_BASE}.$(ARCH)
else
A_NAME_STLDBG := ${LIB_PREFIX}${LIBNAME}${STLDBG_SUFFIX}${A_LIB_TYPE}${LIB_SUFFIX}.${ARCH}
endif
A_NAME_OUT_STLDBG := $(OUTPUT_DIR_A_STLDBG)/$(A_NAME_STLDBG)
