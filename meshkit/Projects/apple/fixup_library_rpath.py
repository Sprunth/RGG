#!/usr/bin/env python
# This is simple script that can be used to fixup a library (dylib or so)
# Usage:
#   ./fixup_library_rpath.py <dir> <lib_name>
# So for lib name 'boost' we will find all libraries that start with
# libboost and make sure that any links to other libboost libs are switched
# to absolute paths which are rooted in <dir>

import commands
import sys
import os.path
import re
import shutil
from fixup_bundle import *

#grab the lib dir and name and remove and starting and trailing quotes
lib_dir = sys.argv[1].strip("'\"")
lib_name = sys.argv[2].strip("'\"")
prefix_new = lib_dir

#treat the lib_name to be the prefix for
libs_to_fix = []
for file in os.listdir(lib_dir):
  if file.startswith('lib'+lib_name) and file.endswith('dylib'):
    libs_to_fix.append(file)

print "Found", len(libs_to_fix), "libraries to fix."
print "\n".join(libs_to_fix)
print ""

#first build up a static '-change string for all libraries'
change_str = ''
for lib in libs_to_fix:
  base_name = os.path.basename(lib)
  full_path = os.path.join(prefix_new,base_name)
  change_str += ' -change %s %s '%(base_name,full_path)

for lib in libs_to_fix:
  base_name = os.path.basename(lib)
  #new_id also happens to be the path
  new_id = os.path.join(prefix_new,base_name)

  commands.getoutput('chmod u+w "%s"' % lib)
  commands.getoutput('install_name_tool -id "%s" %s' % (new_id,new_id))
  commands.getoutput('install_name_tool %s %s' % (change_str,new_id))
  commands.getoutput('chmod a-w "%s"' % lib)

print "Finished fixing up " + lib_name + " libraries"
