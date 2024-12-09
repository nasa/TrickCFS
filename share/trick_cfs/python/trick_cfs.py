import os
import trick

def add_data(cfsSimObject, varType, varName, varAlias = ""):
  if varAlias:
     cfsSimObject.iface.addInstance(varName, varType + ' ' + varAlias)
  else:
     cfsSimObject.iface.addInstance(varName, varType + ' ' + varName)
  

def find(name, path):
    for root, dirs, files in os.walk(path):
        if name in files:
            return os.path.join(root, name)

# Specify mapping of libs to entry point.
# The form libs_map[<Library Name>] = [<Entry Point Function>]
#   will be output in as:
#   CFE_LIB,   /cf/<lower case Library Name>.so, <Entry Point Function>, <Library Name>, 0, 0, 0x0, 0;
#
# E.x. 
#   libs_map['IO_LIB'] = ['IO_LibInit']
#   CFE_LIB,   /cf/io_lib.so,  IO_LibInit, IO_LIB, 0, 0, 0x0, 0;
libs_map = {}

def add_lib_definition( lib_name, lib_entry ):
  libs_map[lib_name] = [lib_entry]

# Specify mapping of apps to entry point, priority, and stacksize
# Note that SCH_trick is automatically added.
# The form apps_map[<Library Name>] = [<Entry Point Function>, <priority>, <stacksize>]
#   will be output in as:
#   CFE_APP,   /cf/<lower case Library Name>.so, <Entry Point Function>, <Library Name>, <priority>, <stacksize>, 0x0, 0;
#
# E.x. 
#   libs_map['SBN'] = ['SBN_AppMain', 50, 131072]
#   CFE_APP, /cf/sbn.so,       SBN_AppMain,       SBN,        50,  131072, 0x0, 0;
apps_map = {}

def add_app_definition( app_name, app_entry, prior, stackSize, objName=None ):
  if objName is None:
     apps_map[app_name] = [app_entry, prior, stackSize]
  else:
     apps_map[app_name] = [app_entry, prior, stackSize, objName]

es_entries = []

origSchAppName = None
TargetSchAppName = 'SCH_LAB_APP'

def set_target_sch_app_name(nameIn):
  global TargetSchAppName
  TargetSchAppName = nameIn

def parse_cfe_es_startup():
  global origSchAppName
  es_file = find("cfe_es_startup.orig", ".")
  interDir = es_file.partition("cf/")[2]
  interDir = interDir.rpartition("/")[0]
  
  if interDir:
    cfDir = "/cf/" + interDir + "/"
  else:
    cfDir = "/cf/"

  inFile = open(es_file, 'r')
  for line in inFile:
     origLine = line
     line = line.strip()
     line = ' '.join(line.split())
     if line.startswith('!'):
        break
     if not line:
        continue

     line = line.replace(',', '')
     line = line.replace(';', '')
     lineFields = line.split(' ')
     if len(lineFields) == 8:
        if lineFields[0] == 'CFE_APP':           
           if lineFields[3] == TargetSchAppName:
              origSchAppName = lineFields[3]
              print("Found and replaced SCH app entry {}".format(TargetSchAppName))
           else:
              apps_map[lineFields[3]] = [lineFields[2], str(lineFields[4]), str(lineFields[5]), lineFields[1]]
              es_entries.append(lineFields[3])
        elif lineFields[0] == 'CFE_LIB':
           libs_map[lineFields[3]] = [lineFields[2], lineFields[1]]
           es_entries.append(lineFields[3])
        else:
           print("Unknown startup entry {}".format(origLine))
     else:
        print("Error parsing {0} {1}".format(es_file, origLine))
  if origSchAppName == None:
     FAIL = '\033[91m'
     ENDC = '\033[0m'
     print('{0}, WARNING: Unable to find target scheduler app for removal {1} {2}'.format(FAIL, TargetSchAppName, ENDC))

def run_default_es_startup():
  run_with_apps(es_entries)


def run_with_apps( appList ):
  es_file = find("cfe_es_startup.scr", ".")
  
  interDir = es_file.partition("cf/")[2]
  interDir = interDir.rpartition("/")[0]
  
  if interDir:
    cfDir = "/cf/" + interDir + "/"
  else:
    cfDir = "/cf/"
  
  maxAppNameSize = len('SCH_trk')
  for mkey in libs_map.keys():
    if len(mkey) > maxAppNameSize:
       maxAppNameSize = len(mkey)
  for mkey in apps_map.keys():
    if len(mkey) > maxAppNameSize:
       maxAppNameSize = len(mkey)
  maxAppNameSize += 1
  maxAppFileSize = maxAppNameSize + len(cfDir) + len('.so,')
  
  maxAppFuncNameSize = len('SCH_TRICK_AppMain')
  for mval in libs_map.values():
    if len(mval[0]) > maxAppFuncNameSize:
       maxAppFuncNameSize = len(mval[0])
  for mval in apps_map.values():
    if len(mval[0]) > maxAppFuncNameSize:
       maxAppFuncNameSize = len(mval[0])
  maxAppFuncNameSize += 2
  
  outFile = open(es_file, 'w')

  appFile = (cfDir + 'sch_trick.so,').ljust(maxAppFileSize)
  appFunc = ('SCH_TRICK_AppMain,').ljust(maxAppFuncNameSize)
  if origSchAppName != None:
     appName = (origSchAppName + ',').ljust(maxAppNameSize)
  else:
     appName = ('SCH,').ljust(maxAppNameSize)
  appPrior = ('80,').rjust(5)
  appSize = ('8192,').rjust(9)
  outFile.write('CFE_APP, ' + appFile + appFunc + appName + appPrior + appSize + ' 0x0, 0;\n')
  
  for app in appList:
     try:
        libItem = libs_map[app]
        if len(libItem) == 2:
           libFile = (libItem[1] + ',').ljust(maxAppFileSize)
        else:
           libFile = (cfDir + app.lower() + '.so,').ljust(maxAppFileSize)
        libFunc = (libItem[0] + ',').ljust(maxAppFuncNameSize)
        libName = (app + ',').ljust(maxAppNameSize)
        libPrior = '0,'.rjust(5)
        libSize = '0,'.rjust(9)
        outFile.write('CFE_LIB, ' + libFile + libFunc + libName + libPrior + libSize + ' 0x0, 0;\n')
        continue
     except KeyError:
        pass
     try:
        appItem = apps_map[app]
        if len(appItem) == 4:
           appFile = (appItem[3] + ',').ljust(maxAppFileSize)
        else:
           appFile = (cfDir + app.lower() + '.so,').ljust(maxAppFileSize)
        appFunc = (appItem[0] + ',').ljust(maxAppFuncNameSize)
        appName = (app + ',').ljust(maxAppNameSize)
        appPrior = (str(appItem[1]) + ',').rjust(5)
        appSize = (str(appItem[2]) + ',').rjust(9)
        outFile.write('CFE_APP, ' + appFile + appFunc + appName + appPrior + appSize + ' 0x0, 0;\n')
     except KeyError:
        if app is not 'TTE':
           print('Unable to find \"' + app + '\"')
        pass
  
  outFile.close()

