#!/usr/bin/env python3

import os, sys, argparse
import common
from common import *

cmdLineParser = argparse.ArgumentParser()
cmdLineParser.add_argument("-o", "--output", dest="output", help="Specify output directory formatting artifacts.",
                           default='{0}/artifacts/formatting'.format(topDir))
cmdLineParser.add_argument("-f", "--format", dest="format", help="Format the code",
                           action="store_true", default=False)

args = cmdLineParser.parse_args()

allProducts = False
artDir = args.output
initArtifactArea(args.output)

retCode = 0

def runCheckFormatting(productDirectory, productName):
  global retCode

  configOut = os.path.join(artDir, 'formattingCheck.txt')

  if args.format:
    ProcessCmd('find {0} \( -path "*/build" -o -path "*/gmock" -o -path "*/boost" -o -path "*/cute" \) -prune -type f -o -iname "*.h" -o -iname "*.hh" -o -iname "*.hpp" -o -iname "*.c" -o -iname "*.cc" -o -iname "*.cpp" -type f | xargs clang-format -i -style=file --verbose'.format(productDirectory), 'Format {0}'.format(productName), configOut, shell=True).join()
  else:
    ProcessCmd('git fetch origin dev; git diff --name-only origin/dev | xargs -I _ find _ -iname "*.h" -o -iname "*.hh" -o -iname "*.hpp" -o -iname "*.c" -o -iname "*.cc" -o -iname "*.cpp" | xargs clang-format -style=file:{0}/.clang-format -Werror --dry-run --verbose'.format(topDir), 'Check Formatting for {0}'.format(productName), configOut, productDirectory, shell=True).join()

def trickcfs():
  runCheckFormatting('.', 'TrickCFS')

trickcfs()


outMsg("Script complete")
sys.exit(retCode)
