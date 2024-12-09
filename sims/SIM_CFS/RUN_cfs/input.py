trick.sim_control_panel_set_enabled(True)
trick.real_time_enable()
trick.freeze()

#######################################################################################################################
## Add app definitions. This section only specifies the parameters of an app. It does not mean the app is included in 
## the run. This allows the user to keep definitions around without a bunch of comments.
## There are two methods available for specifying CFS apps to execute. The first uses the cfe_es_startup.scr file as
## provided in the CFS build. The second is to add lib and app definitions, then provide a list of apps to execute in 
## this run. For this teplate, we choose the CFS build approach.

import trick_cfs

## Obtain app definitions from the CFS build
trick_cfs.parse_cfe_es_startup()

## Run the es_startup as configured in the CFS mission.
trick_cfs.run_default_es_startup()
#######################################################################################################################

### Example input file for adding a global data structure in a CFS app to Trick MemoryManager for TV or data logging.
# Use the optional alias argument to name the global data instance to "to_lab"
trick_cfs.add_data(cfs_core, "TO_LAB_GlobalData_t", "TO_LAB_Global", "to_lab")

cfs_core.sched.addScheduledCmd(int('0x1880', 0), 1.0, 0, 0)

exec(open('Log_data/log_to_lab.py').read())
log_to_lab("to_lab", 0.01)
