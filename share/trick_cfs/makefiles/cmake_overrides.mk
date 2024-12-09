TRICKCFS_SCHEDULER_CLASS?=SchLabTrickCFSScheduler
CFS_SCH_NAME?=sch_lab
CFS_CFE_HOME?=${CFS_MISSION_HOME}/cfe
CFS_OSAL_HOME?=${CFS_MISSION_HOME}/osal
CFS_PSP_HOME?=${CFS_MISSION_HOME}/psp
CFS_APPS_HOME?=${CFS_MISSION_HOME}/apps
CFS_BUILD_HOME?=${CFS_MISSION_HOME}/build
CFS_CPU?=cpu1

ifeq ("$(wildcard ${CFS_CFE_HOME})", "")
  $(error CFS_CFE_HOME is set to ${CFS_CFE_HOME} but doesn\'t exist)
endif

ifeq ("$(wildcard ${CFS_OSAL_HOME})", "")
  $(error CFS_OSAL_HOME is set to ${CFS_OSAL_HOME} but doesn\'t exist)
endif

ifeq ("$(wildcard ${CFS_PSP_HOME})", "")
  $(error CFS_PSP_HOME is set to ${CFS_PSP_HOME} but doesn\'t exist)
endif

ifeq ("$(wildcard ${CFS_APPS_HOME})", "")
  $(error CFS_APPS_HOME is set to ${CFS_APPS_HOME} but doesn\'t exist)
endif

ifeq ("$(wildcard ${CFS_APPS_HOME}/${CFS_SCH_NAME})", "")
  $(error CFS_SCH_NAME is set to ${CFS_SCH_NAME} but ${CFS_APPS_HOME}/${CFS_SCH_NAME} doesn\'t exist)
endif

ifeq ("$(wildcard ${TRICKCFS_HOME}/apps/sch_trick/fsw/src/${TRICKCFS_SCHEDULER_CLASS}.cpp)", "")
  $(error TRICKCFS_SCHEDULER_CLASS is set to ${TRICKCFS_SCHEDULER_CLASS} but ${TRICKCFS_HOME}/apps/sch_trick/fsw/src/${TRICKCFS_SCHEDULER_CLASS}.cpp doesn\'t exist)
endif

TRICK_SFLAGS += -DTRICKCFS_SCHEDULER_CLASS=${TRICKCFS_SCHEDULER_CLASS}

define ADDFPIC_template =
  $(eval CHECKFPIC:=$(shell grep fPIC $(1) | grep -q fPIC))
  $(eval $(shell if [ "${CHECKFPIC}" != "fPIC" ]; then origTime=`date -R -r $(1)`; echo -e "C_FLAGS += -fPIC\nCXX_FLAGS += -fPIC" >> $(1); touch -d "$${origTime}" $(1); fi))
endef

ifndef CLEAN_TARGETS
CLEAN_TARGETS:=tidy clean spotless distclean apocalypse
endif
ifeq ($(findstring ${MAKECMDGOALS},$(CLEAN_TARGETS)),)
S_source.hh : $(CFS_BUILD_HOME)/Makefile
Makefile_sim : $(CFS_BUILD_HOME)/Makefile

ifeq "${PREP_MAKE_CMD}" ""
   PREP_MAKE_CMD:=$(PREP_OPTIONS) prep
endif
PREP := $(shell if [ ! -f $(CFS_BUILD_HOME)/Makefile ]; then mkdir -p ${PWD}/build; cd $(CFS_MISSION_HOME); $(PREP_PRECMDS) $(MAKE) ${PREP_MAKE_CMD} > ${PWD}/build/make_cfs_prep.txt 2>&1; fi;)
CHECKPREP:=$(shell echo $$?)
ifneq "${CHECKPREP}" "0"
   $(error Build error executing "cd $(CFS_MISSION_HOME) && $(PREP_PRECMDS) ${PREP_CMD}. See ${PWD}/build/make_cfs_prep.txt")
endif

ifeq ("$(wildcard ${CFS_BUILD_HOME})", "")
  $(error CFS_BUILD_HOME is set to ${CFS_BUILD_HOME} but doesn\'t exist)
endif

CPU_BUILD_DIR := $(realpath $(shell ${TRICKCFS_HOME}/bin/find_osal ${CFS_BUILD_HOME} ${CFS_CPU})/..)
ifeq "${CPU_BUILD_DIR}" "/"
   $(error Unable to resolve CPU_BUILD_DIR automatically. Command used was "${TRICKCFS_HOME}/bin/find_osal ${CFS_BUILD_HOME} ${CFS_CPU}")
endif
$(info CPU_BUILD_DIR = $(CPU_BUILD_DIR))

ifeq "${INSTALL_MAKE_CMD}" ""
   INSTALL_MAKE_CMD:=$(INSTALL_OPTIONS) install
endif

FLAGS_MAKE_FILES := $(shell ${TRICKCFS_HOME}/bin/find_build_flags ${CPU_BUILD_DIR})
$(foreach ii,$(FLAGS_MAKE_FILES),$(call ADDFPIC_template,$(ii)))
endif

CFS_INSTALL_SUBDIR ?= cf

CFS_APPS_IN_BUILD := $(CFS_APPS)

define CFLAGS_template =
  $(eval TEMP_VAR :=$(strip $(if $(findstring $(1), $(TRICK_CFLAGS)),,$(1))))
  $(eval TRICK_CFLAGS+=$(TEMP_VAR))
  $(eval TRICK_CXXFLAGS+=$(TEMP_VAR))
endef

define PROCESSED_PATH_template =
 $(eval TEMP_VAR := $(if $(findstring -I,$(1)),$(if $(findstring $(strip $(1)),$(PROCESSED_CFS_APP_PATHS)),,$(strip $(1))),))
 $(eval PROCESSED_CFS_APP_PATHS+=$(TEMP_VAR))
endef

define ALL_CMAKEFLAGS_template =
 $(eval C_DEFINES:=)
 $(eval C_FLAGS:=)
 $(eval C_INCLUDES:=)
 $(eval CXX_DEFINES:=)
 $(eval CXX_FLAGS:=)
 $(eval CXX_INCLUDES:=)
 $(eval include ${1})
 $(eval RAW_ALL_FLAGS:=$(C_DEFINES) $(C_FLAGS) $(C_INCLUDES) $(CXX_DEFINES) $(CXX_FLAGS) $(CXX_INCLUDES))
 $(eval ALL_FLAGS:=$(filter-out -include %.h,$(RAW_ALL_FLAGS)))
 $(foreach d,$(ALL_FLAGS),$(eval $(call CFLAGS_template,$(d))))
 $(foreach d,$(ALL_FLAGS),$(eval $(call PROCESSED_PATH_template,$(d))))
endef

$(foreach ii,$(FLAGS_MAKE_FILES),$(eval $(call ALL_CMAKEFLAGS_template,$(ii))))

$(foreach d,$(shell ${TRICKCFS_HOME}/bin/find_subdir_includes ${CFS_OSAL_HOME}/src/bsp/generic-linux),$(eval $(call CFLAGS_template,$(d))))

## Grab some object files from the CFS-side of the build. 
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/$(CFS_BUILD_HOME)/src/cfe_mission_strings.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/src/target_config.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/cfe_module_version_table.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/cfe_psp_module_list.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/cfe_static_symbol_list.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/cfe_static_module_list.c.o
TRICK_LDFLAGS += $(CPU_BUILD_DIR)/$(CFS_CPU)/CMakeFiles/core-$(CFS_CPU).dir/cfe_build_env_table.c.o


# Include directories for glue code
TRICK_CFLAGS += -I${TRICKCFS_HOME}/models -I${TRICKCFS_HOME}/osal/src/bsp -I${TRICKCFS_HOME}/osal/src/os -I${TRICKCFS_HOME}/psp/fsw
TRICK_CXXFLAGS += -I${TRICKCFS_HOME}/models -I${TRICKCFS_HOME}/osal/src/bsp -I${TRICKCFS_HOME}/osal/src/os -I${TRICKCFS_HOME}/psp/fsw
CFS_ES_INCLUDES:= $(shell ${TRICKCFS_HOME}/bin/find_subdir_includes ${CFS_CFE_HOME}/modules/es/fsw)
CFS_SB_INCLUDES:= $(shell ${TRICKCFS_HOME}/bin/find_subdir_includes ${CFS_CFE_HOME}/modules/sb/fsw)
TRICK_CFLAGS += ${CFS_ES_INCLUDES} ${CFS_SB_INCLUDES}
TRICK_CXXFLAGS += ${CFS_ES_INCLUDES} ${CFS_SB_INCLUDES}

# exclude CFS directores we cannot control
CFS_ICG_EXCLUDES:=
ifndef TRICK_ICG_EXCLUDE
TRICK_ICG_EXCLUDE := ${CFS_ICG_EXCLUDES}
else
TRICK_ICG_EXCLUDE := $(TRICK_ICG_EXCLUDE):${CFS_ICG_EXCLUDES}
endif

CFS_SWIG_EXCLUDES:=${CFS_OSAL_HOME}:${CFS_PSP_HOME}:${CFS_CFE_HOME}
ifndef TRICK_SWIG_EXCLUDE
TRICK_SWIG_EXCLUDE := ${CFS_SWIG_EXCLUDES}
else
TRICK_SWIG_EXCLUDE := $(TRICK_ICG_EXCLUDE):${CFS_SWIG_EXCLUDES}
endif

# Rules for compatability
EQUUELEUS_RC1:= draco-rc5 equuleus-rc1
SUPPORTED_COMPAT_TAGS:= ${EQUUELEUS_RC1}
ifneq "${CFS_COMPAT_TAG}" ""
   ifneq ($(findstring ${CFS_COMPAT_TAG},$(SUPPORTED_COMPAT_TAGS)),)
      TRICK_CFLAGS+=-DTRICKCFS_CFS_COMPAT=${CFS_COMPAT_TAG}
      TRICK_CXXFLAGS+=-DTRICKCFS_CFS_COMPAT=${CFS_COMPAT_TAG}

      include
      ifeq ($(findstring ${CFS_COMPAT_TAG},$(EQUUELEUS_RC1)),)
         ifndef TRICK_ICG_EXCLUDE
            TRICK_ICG_EXCLUDE:=${CFS_CFE_HOME}/modules/core_api/fsw/inc/cfe_msg_api_typedefs.h
         else
            TRICK_ICG_EXCLUDE:=${CFS_CFE_HOME}/modules/core_api/fsw/inc/cfe_msg_api_typedefs.h:$(TRICK_ICG_EXCLUDE)
         endif
      endif
   else
      $(error Unknown compat tag specified ${CFS_COMPAT_TAG}. Supported tags are "${SUPPORTED_COMPAT_TAGS}")
   endif
endif


CFS_APPS_INCLUDES := $(foreach d, $(CFS_APPS) , $(shell ${TRICKCFS_HOME}/bin/find_subdir_includes ${CFS_APPS_HOME}/$(d)/fsw))
$(foreach d,$(CFS_APPS_INCLUDES),$(eval $(call CFLAGS_template,$(d))))
$(foreach d,$(CFS_APPS_INCLUDES),$(eval $(call PROCESSED_PATH_template,$(d))))
CFS_APPS_LINK := $(addsuffix .so,$(CFS_APPS_IN_BUILD))
CFS_APPS_LINK := $(addprefix ${CFS_INSTALL_SUBDIR}/,$(CFS_APPS_LINK))
CFS_APPS_LINK += build/rebuilt_cfe/libpsp.so
CFS_APPS_LINK += build/rebuilt_cfe/libosal.so
TRICK_LDFLAGS += -Wl,--as-needed,-no-whole-archive,-rpath,.,-rpath,./${CFS_INSTALL_SUBDIR}
TRICK_LDFLAGS += $(CFS_APPS_LINK) $(CFS_APPS_LINK) -Wl,--no-as-needed,-whole-archive
TRICK_LDFLAGS += @build/rebuilt_cfe/static_lib_list

$(eval space:=)
$(eval space+= )
TEMP_VAR := $(subst -I,,$(subst $(space)-I,:,$(filter -I%,$(PROCESSED_CFS_APP_PATHS))))
TRICK_SWIG_EXCLUDE := $(TRICK_SWIG_EXCLUDE)${TEMP_VAR}
#TRICK_SWIG_EXCLUDE:=${CFS_OSAL_HOME}/src/os/inc/osapi-task.h
#TRICK_SWIG_EXCLUDE+=:${CFS_PSP_HOME}/fsw/inc/cfe_psp.h

TRICK_CFS_APPS_DIRS := $(addprefix ${TRICKCFS_HOME}/apps/,$(TRICK_CFS_APPS))
TRICK_CFS_APPS_INCLUDES := $(foreach d, $(TRICK_CFS_APPS_DIRS) , $(shell ${TRICKCFS_HOME}/bin/find_subdir_includes $(d)/fsw))
TRICK_CFLAGS += $(TRICK_CFS_APPS_INCLUDES)
TRICK_CXXFLAGS += $(TRICK_CFS_APPS_INCLUDES)

SCH_TRICK_APP := sch_trick
TRICK_CFLAGS += -I${TRICKCFS_HOME}/apps/$(SCH_TRICK_APP)/fsw/src
TRICK_CXXFLAGS += -I${TRICKCFS_HOME}/apps/$(SCH_TRICK_APP)/fsw/src

CFS_SCH_INC_DIRS:=$(shell ${TRICKCFS_HOME}/bin/find_subdir_includes ${CFS_APPS_HOME}/${CFS_SCH_NAME})
TRICK_CFLAGS+=${CFS_SCH_INC_DIRS}
TRICK_CXXFLAGS+=${CFS_SCH_INC_DIRS}

TRICK_CFS_APP_BUILD := $(CURDIR)/trick_cfs_apps_build

TRICK_CFS_APPS_LIBS := $(foreach d, $(TRICK_CFS_APPS),${CFS_INSTALL_SUBDIR}/$(d).so)
SCH_TRICK_APP_LIB := ${CFS_INSTALL_SUBDIR}/$(SCH_TRICK_APP).so

TRICK_LDFLAGS := ${TRICK_CFS_APPS_LIBS} ${TRICK_LDFLAGS}
TRICK_LDFLAGS := ${SCH_TRICK_APP_LIB} ${TRICK_LDFLAGS}

TRICK_CFS_APPS_SRCS := $(foreach d, $(TRICK_CFS_APPS),$(wildcard ${TRICKCFS_HOME}/apps/$(d)/fsw/src/*.c))
SCH_TRICK_APP_SRCS := ${TRICKCFS_HOME}/apps/sch_trick/fsw/src/BaseTrickCFSScheduler.cpp ${TRICKCFS_HOME}/apps/sch_trick/fsw/src/${TRICKCFS_SCHEDULER_CLASS}.cpp

TRICK_CFS_APPS_OBJS_RAW := $(patsubst %.c,%.o,$(TRICK_CFS_APPS_SRCS))
TRICK_CFS_APPS_OBJS := $(subst ${TRICKCFS_HOME}/apps,$(TRICK_CFS_APP_BUILD),$(TRICK_CFS_APPS_OBJS_RAW))
SCH_TRICK_APP_OBJS_RAW := $(patsubst %.cpp,%.o,$(SCH_TRICK_APP_SRCS))
SCH_TRICK_APP_OBJS := $(subst ${TRICKCFS_HOME}/apps,$(TRICK_CFS_APP_BUILD),$(SCH_TRICK_APP_OBJS_RAW))

TRICK_CFS_APPS_OBJ_DIRS := $(foreach d, $(TRICK_CFS_APPS),$(TRICK_CFS_APP_BUILD)/$(d)/fsw/src) $(TRICK_CFS_APP_BUILD)/$(SCH_TRICK_APP)/fsw/src

ISCFLAGS32BIT:=$(filter -m32,$(TRICK_CFLAGS))

TRICK_CFS_APPS_CMD := $(foreach d, $(TRICK_CFS_APPS), $(TRICK_CC) -shared -g $(ISCFLAGS32BIT) -o ${CFS_INSTALL_SUBDIR}/$(d).so $(TRICK_CFS_APP_BUILD)/$(d)/fsw/src/*.o &&)
SCH_TRICK_APP_CMD := $(TRICK_CXX) -shared -g $(ISCFLAGS32BIT) -o $(SCH_TRICK_APP_LIB) $(TRICK_CFS_APP_BUILD)/$(SCH_TRICK_APP)/fsw/src/*.o


$(TRICK_CFS_APPS_OBJ_DIRS):
	mkdir -p $(TRICK_CFS_APPS_OBJ_DIRS)

$(TRICK_CFS_APP_BUILD)/%.o : ${TRICKCFS_HOME}/apps/%.c $(TRICK_CFS_APPS_OBJ_DIRS)
	$(TRICK_CC) $(TRICK_CFLAGS) $(TRICK_SYSTEM_CFLAGS) -c $< -o $@

$(TRICK_CFS_APP_BUILD)/%.o : ${TRICKCFS_HOME}/apps/%.cpp $(TRICK_CFS_APPS_OBJ_DIRS)
	$(TRICK_CXX) $(TRICK_CXXFLAGS) $(TRICK_SYSTEM_CFLAGS) -c $< -o $@

$(TRICK_CFS_APPS_LIBS) : $(TRICK_CFS_APPS_OBJS) cf_files
	$(TRICK_CFS_APPS_CMD) :
	@echo "Done creating TrickCFS-built Apps \"$(TRICK_CFS_APPS)\"";

$(SCH_TRICK_APP_LIB) : $(SCH_TRICK_APP_OBJS) cf_files
	$(SCH_TRICK_APP_CMD)
	@echo "Done creating TrickCFS SCH_TRICK app";

$(CFS_INSTALL_SUBDIR):
	mkdir -p ${CFS_INSTALL_SUBDIR}

build/rebuilt_cfe:
	mkdir -p build/rebuilt_cfe

build_cfs:
	@echo "Now compile cfs mission"
	cd $(CFS_MISSION_HOME); $(INSTALL_PRECMDS) $(MAKE) $(INSTALL_MAKE_CMD) 

ifeq "$(MAKECMDGOALS)" "clean"
clean: clean_cfs 
endif

spotless: spotless_cfs

spotless_cfs:
	-cd $(CFS_MISSION_HOME) && $(MAKE) distclean
	rm -rf cf $(TRICK_CFS_APP_BUILD) ${PWD}/build/make_cfs_prep.txt

clean_cfs:
	-cd $(CFS_MISSION_HOME) && $(MAKE) clean
	rm -rf cf $(TRICK_CFS_APP_BUILD)

PSP_STRIP_FUNCS := OS_Application_Run CFE_PSP_AttachExceptions
PSP_BUILD_DIR := $(CPU_BUILD_DIR)/psp
PSP_STRIP_CMD := $(TRICK_CC) -shared -g $(ISCFLAGS32BIT) -o build/rebuilt_cfe/libpsp.so `find ${PSP_BUILD_DIR} -name "*.o"`
OSAL_STRIP_FUNCS := OS_TaskCreate OS_TaskCreate_Impl
OSAL_BUILD_DIR := $(CPU_BUILD_DIR)/osal
OSAL_STRIP_CMD := $(TRICK_CC) -shared -g $(ISCFLAGS32BIT) -o build/rebuilt_cfe/libosal.so `find ${OSAL_BUILD_DIR} -name "*.o" | grep -v ut_assert | grep -v ut-stubs`
OSAL_BSP_STRIP_FUNCS := main
#OSAL_BSP_STRIP_CMD := strip $(addprefix --strip-symbol=,${OSAL_BSP_STRIP_FUNCS}) build/rebuilt_cfe/libosal_bsp.a

cf_files: build_cfs $(CFS_INSTALL_SUBDIR) build/rebuilt_cfe
	cp -r $(CFS_BUILD_HOME)/exe/$(CFS_CPU)/${CFS_INSTALL_SUBDIR}/* cf/;
	cp ${CFS_INSTALL_SUBDIR}/cfe_es_startup.scr ${CFS_INSTALL_SUBDIR}/cfe_es_startup.orig
	${PSP_STRIP_CMD}
	${OSAL_STRIP_CMD}
	${OSAL_BSP_STRIP_CMD}
	find $(CPU_BUILD_DIR) -name "*.a" | grep -v osal | grep -v psp |grep -v libtblobj > build/rebuilt_cfe/static_lib_list

TRICKCFS_MSG_OBJ := $(patsubst %.c,%.o,$(TRICKCFS_MSGTBL_SRC))
TRICKCFS_MSG_OBJ := $(addprefix $(TRICK_CFS_APP_BUILD),$(TRICKCFS_MSG_OBJ))
TRICKCFS_MSG_TBL := $(patsubst %.c,%.tbl,$(TRICKCFS_MSGTBL_SRC))
TRICKCFS_MSG_TBL := $(addprefix $(CURDIR)/${CFS_INSTALL_SUBDIR}/, $(TRICKCFS_MSG_TBL))

TRICKCFS_SCH_OBJ := $(patsubst %.c,%.o,$(TRICKCFS_SCHTBL_SRC))
TRICKCFS_SCH_OBJ := $(addprefix $(TRICK_CFS_APP_BUILD),$(TRICKCFS_SCH_OBJ))
TRICKCFS_SCH_TBL := $(patsubst %.c,%.tbl,$(TRICKCFS_SCHTBL_SRC))
TRICKCFS_SCH_TBL := $(addprefix $(CURDIR)/${CFS_INSTALL_SUBDIR}, $(TRICKCFS_SCH_TBL))

TRICKCFS_TBLS := $(TRICKCFS_SCH_TBL) $(TRICKCFS_MSG_TBL)

$(TRICKCFS_SCH_OBJ) : $(TRICKCFS_SCHTBL_SRC)
	mkdir -p `dirname $@`; \
	$(TRICK_CC) $(TRICK_CFLAGS) $(TRICK_SYSTEM_CFLAGS) -c $< -o $@


$(TRICKCFS_MSG_OBJ) : $(TRICKCFS_MSGTBL_SRC)
	mkdir -p `dirname $@`; \
	$(TRICK_CC) $(TRICK_CFLAGS) $(TRICK_SYSTEM_CFLAGS) -c $< -o $@

$(TRICKCFS_SCH_TBL) : $(TRICKCFS_SCH_OBJ) cf_files
	cd ${CFS_INSTALL_SUBDIR}; \
	$(CFS_BUILD_HOME)/exe/host/elf2cfetbl $<

$(TRICKCFS_MSG_TBL) : $(TRICKCFS_MSG_OBJ) cf_files
	cd ${CFS_INSTALL_SUBDIR}; \
	$(CFS_BUILD_HOME)/exe/host/elf2cfetbl $<

sch_trick_tables: $(TRICKCFS_TBLS)
	echo 'No default SCH_TRICK tables provided by default'

$(S_MAIN): cf_files $(SCH_TRICK_APP_LIB) $(TRICK_CFS_APPS_LIBS) sch_trick_tables
