#!/usr/bin/env python
# This is simple script is used to remove a function from code

import commands
import sys
import os.path


input_file = open(sys.argv[1], "r")
output_file = open(sys.argv[2], "w")

cont=False

for l in input_file:
  if "#END REMOVE ME FROM PACKAGE" in l:
     cont = False
  elif "#BEGIN REMOVE ME FROM PACKAGE" in l:
     cont = True
  elif cont:
     continue
  else:
     output_file.write(l)

print "Finished cleaning file"
