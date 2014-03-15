#!/usr/bin/env python
# This is simple script that can be used to fixup a library (dylib or so)
# Usage:
#   ./fixup_plugin.py <full path to lib to fix or dir with libraries to fix> "key=val" ["key=val" ...]
# This is simply replaces any referece to 'key' with 'val' in the libraries referred to
# by the plugin lib to fix. 'key' can be a Python regular expression.
# The order of the key=value pairs is significant. The expressions are
# tested in the order specified.

import commands
import sys
import os.path
import re
import shutil
from fixup_bundle import *

plugin_dir = sys.argv[1]
prefix_new = sys.argv[2]

libs_to_fix = commands.getoutput('find %s -type f | xargs file --separator ":--:" | grep -i ":--:.*Mach-O" | sed "s/:.*//" | sort | uniq ' % plugin_dir).split()
print "Found", len(libs_to_fix), "libraries to fix."
print "\n".join(libs_to_fix)
print ""

so_files = [x for x in libs_to_fix if ".so" in x]

for so in so_files:
  libs = getdependencies(so)
  for dylib in libs:
    if not isexcluded(dylib):
      if "libshiboken-python" in dylib:
        commands.getoutput('install_name_tool -change "%s" "@loader_path/%s" "%s"'%(dylib, os.path.basename(dylib), so))
      else:
        commands.getoutput('install_name_tool -change "%s" "@executable_path/../Libraries/%s" "%s"'%(dylib, os.path.basename(dylib), so))

for plugin_lib in libs_to_fix:
  base_name = os.path.basename(plugin_lib)
  new_id = os.path.join(prefix_new,base_name)
  commands.getoutput('chmod u+w "%s"' % plugin_lib)
  commands.getoutput('install_name_tool -id "%s" %s' % (new_id,plugin_lib))
  commands.getoutput('chmod a-w "%s"' % plugin_lib)

print "Finished fixing up python"
