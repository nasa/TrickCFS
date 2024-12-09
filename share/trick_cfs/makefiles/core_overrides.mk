#============================================================================
#  Start of Trick-in-CFS provided S_overrides.mk
#============================================================================
TRICK_SFLAGS += -I${TRICKCFS_HOME}/share/trick_cfs
TRICK_PYTHON_PATH += :${TRICKCFS_HOME}/share/trick_cfs/python


include ${TRICKCFS_HOME}/share/trick_cfs/makefiles/cmake_overrides.mk

## Common C compiler flags that do not apply to C++ compilation
C_TO_CPP_FLAGS_TO_FILTER := -std=c99 -Wstrict-prototypes -pedantic -Wwrite-strings -Wpointer-arith -Wcast-align
C_TO_CPP_FLAGS_TO_FILTER += -std=gnu99
TRICK_CXXFLAGS := $(filter-out $(C_TO_CPP_FLAGS_TO_FILTER),$(TRICK_CXXFLAGS))
## Problematic compiler flags from FSW to not apply to trick compiles
FSW_PROBLEM_FLAGS := -Werror
TRICK_CFLAGS := $(filter-out $(FSW_PROBLEM_FLAGS),$(TRICK_CFLAGS))
TRICK_CXXFLAGS := $(filter-out $(FSW_PROBLEM_FLAGS),$(TRICK_CXXFLAGS))
