# CFS_MISSION_HOME must be set to the CFS core directory
ifndef CFS_MISSION_HOME
$(error "CFS_MISSION_HOME environment variable must be set.")
endif

# TRICKCFS_HOME must be set to the CFS-in-Trick directory
TRICKCFS_HOME ?= $(realpath $(CURDIR)/../..)

TRICK_CFLAGS += -g
TRICK_CXXFLAGS += -g

#######################################################################
# Add any S_overrides.mk lines that are required for simulation below #
#######################################################################
# Specify which scheduler class to user, defaults to SchLabTrickCFSScheduler
#TRICKCFS_SCHEDULER_CLASS:=SchLabTrickCFSSchedulerEquuleusRC1
# Specify the directory name of the CFS sch app, defaults to sch_lab
#CFS_SCH_NAME=sch

#Optional CFS compatability tag.
#CFS_COMPAT_TAG:=equuleus-rc1

# Specify directory locations of the CFS components d
#CFS_CFE_HOME:=${CFS_MISSION_HOME}/cfe
#CFS_OSAL_HOME:=${CFS_MISSION_HOME}/osal
#CFS_PSP_HOME:=${CFS_MISSION_HOME}/psp
#CFS_APPS_HOME:=${CFS_MISSION_HOME}/apps
#CFS_BUILD_HOME:=${CFS_MISSION_HOME}/build

# Specify the build target name as specified in the cmake variable MISSION_CPUNAMES, defaults to cpu1
#CFS_CPU:=lx1

# Specify list of CFS apps needed for Trick's MemoryManager but not 
# compiled by Trick.
CFS_APPS:= to_lab

# Specify list of CFS apps needed for Trick's MemoryManager to also be 
# compiled by Trick
TRICK_CFS_APPS:= 

## Add shell commands to execute prior to 'make prep'
## and options to pass to 'make prep' command.
## i.e. $(PREP_PRECMDS) make $(PREP_OPTIONS) prep;
PREP_PRECMDS :=
PREP_OPTIONS := SIMULATION=native

## Add shell commands to execute prior to 'make install'
## and options to pass to 'make install' command.
## i.e. $(INSTALL_PRECMDS) make $(INSTALL_OPTIONS) install;
INSTALL_PRECMDS := 
INSTALL_OPTIONS :=
#######################################################################

# This is the overrides file for the CFS core code (do not modify)
include ${TRICKCFS_HOME}/share/trick_cfs/makefiles/core_overrides.mk
