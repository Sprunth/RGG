#!/usr/bin/env python


#App="$1" # argument is the application to fixup
#LibrariesPrefix="Contents/Libraries"
#echo ""
#echo "Fixing up $App"
#echo "All required frameworks/libraries will be placed under $App/$LibrariesPrefix"
#echo ""
#echo "----------------------------"
#echo "Locating all executables and dylibs already in the package ... "
#
## the sed-call removes the : Mach-O.. suffix that "file" generates
#executables=`find $App | xargs file | grep -i "Mach-O.*executable" | sed "s/:.*//" | sort`
#
#echo "----------------------------"
#echo "Found following executables:"
#for i in $executables; do
#  echo $i
#done
#
## for each executable, find any external library.
#
#
#libraries=`find $App | xargs file | grep -i "Mach-O.*shared library" | sed "s/:.*//" | sort`
#
## command to find all external libraries referrenced in package:
## find paraview.app | xargs file | grep "Mach-O" | sed "s/:.*//" | xargs otool -l | grep " name" | sort | uniq | sed "s/name\ //" | grep -v "@executable"
#
## find non-system libs
## find paraview.app | xargs file | grep "Mach-O" | sed "s/:.*//" | xargs otool -l | grep " name" | sort | uniq | sed "s/name\ //" | grep -v "@executable" | grep -v "/System/" | grep -v "/usr/lib/"

import commands
import sys
import os.path
import os
import re
import shutil
import logging

logging.basicConfig(level=logging.INFO)

class Library(object):
  def __init__(self, path):
    # This is the actual path to a physical file
    self.RealPath = os.path.realpath(path)

    # This is the id for shared library.
    self.Id = _getid(self.RealPath)

    # this is all the paths to a physical file
    # that we have found, this can be caused by multiple symlinks to
    # the same file. We have to fixup both the absolute file and the
    # symlink paths
    self.Paths = set( [self.RealPath,self.Id,path] )

    # These are names for symbolic links to this file.
    self.SymLinks = []

    self.__dependencies = None
    pass

  def __hash__(self):
    return self.Id.__hash__()

  def __eq__(self, other):
    #if two items are equal we are going to be super
    #sneaky and union the paths, and update each items paths
    are_equal = self.Id == other.Id
    if are_equal:
      updated_paths = self.Paths.union(other.Paths)
      self.Paths = updated_paths
      other.Paths = updated_paths
    return are_equal

  def __repr__(self):
    return "Library(%s : %s)" % (self.Id, self.RealPath)

  def generate_install_commands(self):
    commands = []
    for path in self.Paths:
      commands += ["-change", '"%s"' % path, '"%s"' % self.Id]
    return commands


  def dependencies(self, exepath):
    if self.__dependencies:
      return self.__dependencies
    collection = set()
    for dep in getdependencies(self.RealPath):
      if not re.match(r".*libcubit.*\.dylib", dep): #(dep):
        collection.add(Library.createFromReference(dep, exepath))
      else:
        logging.info("SKIPPING THIS DEP: %s"%dep)
    self.__dependencies = collection
    return self.__dependencies

  def copyToApp(self, app, fakeCopy=False):
    if _isframework(self.RealPath):
      m = re.match(r'(.*)/(\w+\.framework)/(.*)', self.RealPath)
      # FIXME: this could be optimized to only copy the particular version.
      if not fakeCopy:
        logging.info("Copying %s/%s ==> %s" % (m.group(1), m.group(2), ".../Contents/Frameworks/"))
        dirdest = os.path.join(os.path.join(app, "Contents/Frameworks/"), m.group(2))
        filedest = os.path.join(dirdest, m.group(3))
        if not os.path.exists(dirdest):
          shutil.copytree(os.path.join(m.group(1), m.group(2)), dirdest, symlinks=True)
      self.Id = "@executable_path/../Frameworks/%s" % (os.path.join(m.group(2), m.group(3)))
      if not fakeCopy:
        #frameworks can be coming for a system installed place so we need to
        #modify the permission to allow us to run install_name_tool
        commands.getoutput('chmod u+w "%s"' % filedest)
        commands.getoutput('install_name_tool -id "%s" %s' % (self.Id, filedest))
        commands.getoutput('chmod u-w "%s"' % filedest)
    else:
      if not fakeCopy:
        d = os.path.dirname(os.path.join(app, "Contents/Libraries/"))
        if not os.path.exists(d):
          os.makedirs(d)
        logging.info("Copying %s ==> %s" % (self.RealPath, ".../Contents/Libraries/%s" % os.path.basename(self.RealPath)))
        shutil.copy(self.RealPath, d )
      self.Id = "@executable_path/../Libraries/%s" % os.path.basename(self.RealPath)
      if not fakeCopy:
        filedest = os.path.join(app, "Contents/Libraries/%s" % os.path.basename(self.RealPath))
        commands.getoutput('chmod u+w "%s"' % filedest)
        commands.getoutput('install_name_tool -id "%s" %s' % (self.Id, filedest))
        commands.getoutput('chmod u-w "%s"' % filedest)

      # Create symlinks for this copied file in the install location
      # as were present in the source dir.
      destdir = os.path.join(app, "Contents/Libraries")
      # sourcefile is the file we copied already into the app bundle. We need to create symlink
      # to it itself in the app bundle.
      sourcefile = os.path.basename(self.RealPath)
      for symlink in self.SymLinks:
        logging.info("Creating Symlink %s ==> .../Contents/Libraries/%s" % (symlink, os.path.basename(self.RealPath)))
        if not fakeCopy:
          commands.getoutput("ln -s %s %s" % (sourcefile, os.path.join(destdir, symlink)))

  @classmethod
  def createFromReference(cls, ref, exepath):
    path = ref.replace("@executable_path", exepath)
    if not os.path.exists(path):
      path = _find(ref)
    return cls.createFromPath(path)

  @classmethod
  def createFromPath(cls, path):
    if not os.path.exists(path) and _isframework(path):
      path = os.path.join('/Library/Frameworks/',path)
    if not os.path.exists(path) and re.match(r"libQt.*\.dylib", path):
      path = os.path.join('/usr/lib/',path)

    if not os.path.exists(path):
      errorMsg = "%s is not a filename" % path
      raise Exception(errorMsg)
      exit(1)

    lib = Library(path)

    # locate all symlinks to this file in the containing directory. These are used when copying.
    # We ensure that we copy all symlinks too.
    dirname = os.path.dirname(lib.RealPath)
    symlinks = commands.getoutput("find -L %s -samefile %s" % (dirname, lib.RealPath))
    symlinks = symlinks.split()
    try:
      symlinks.remove(lib.RealPath)
    except ValueError:
      pass
    linknames = []
    for link in symlinks:
      linkname = os.path.basename(link)
      linknames.append(linkname)
    lib.SymLinks = linknames
    return lib


def _getid(lib):
  """Returns the id for the library"""
  val = commands.getoutput("otool -D %s" % lib)
  m = re.match(r"[^:]+:\s*([^\s]+)", val)
  if m:
    return m.group(1)
  errorMsg = "Could not determine id for %s" % lib
  raise Exception(errorMsg)

def getdependencies(path):
  val = commands.getoutput('otool -l %s| grep " name" | sort | uniq | sed "s/name\ //" | sed "s/(offset.*)//"' % path)
  return val.split()

def isexcluded(id):
  if re.match(r".*Qt\w+\.framework/",id):
    #make an exception for Qt, we always need to package
    #qt, even if it is installed in a system library location
    return False
  if re.match(r".*libQt.*\.dylib",id):
    #make an exception for external built qt libraries
    #even when it is installed in a system location like
    #user/local/Cellar.
    return False
  if re.match(r"^libz\.1\.dylib", id):
    return True
  if re.match(r"^/System/Library", id):
    return True
  if re.match(r"^/Library/Frameworks/Python\.framework/", id):
    return True
  if re.match(r"^/usr/lib", id):
    return True
  if re.match(r"^/usr/local", id):
    return True
  #if re.match(r".*libcubit.*\.dylib", id):
  # return True
  return False

def _isframework(path):
  if re.match(".*\.framework.*", path):
    return True

def _find(ref):
  name = os.path.basename(ref)
  for loc in SearchLocations:
    output = commands.getoutput('find "%s" -name "%s"' % (loc, name)).strip()
    if output:
      return output
  return ref

def _makeSymlink(ref):
  orig_dir,app_name = os.path.split(ref)
  old_path,directory = os.path.split(orig_dir)
  if(" " in directory):
    safe_dir = os.path.join(old_path,directory.replace(" ","_"))
    if not os.path.exists(safe_dir):
      #create a symlink from directory to safe_dir
      os.symlink(orig_dir,safe_dir)
    return os.path.join(safe_dir,app_name)
  return ref

def _removeSymlink(ref):
  sym_dir,app_name = os.path.split(ref)
  if os.path.isdir(sym_dir) and os.path.islink(sym_dir):
    os.unlink(sym_dir)
    return True
  return False

SearchLocations = []
if __name__ == "__main__":
  App = _makeSymlink(sys.argv[1])
  SearchLocations = [sys.argv[2]]
  if len(sys.argv) > 3:
    QtPluginsDir = sys.argv[3]
  else:
    QtPluginsDir = None
  LibrariesPrefix = "Contents/Libraries"

  logging.info( "------------------------------------------------------------" )
  logging.info(  "Fixing up " + str(App) )
  logging.info(  "All required frameworks/libraries will be placed under %s/%s" % (App, LibrariesPrefix))
  logging.info(  "" )

  executables = commands.getoutput('find %s -type f| xargs file | grep -i "Mach-O.*executable" | sed "s/:.*//" | sort' % App)
  executables = executables.split()
  logging.info( "------------------------------------------------------------" )
  logging.info( "Found executables : " )
  for exe in executables:
    logging.info( "    %s/%s" % (os.path.basename(App) ,os.path.relpath(exe, App)) )
  logging.info( "" )


  # Find libraries inside the package already.
  libraries = commands.getoutput('find %s -type f | xargs -0 file | grep -i "Mach-O.*shared library" | sed "s/:.*//" | sort' % App)


  libraries = libraries.split()

  logging.info( "Found %d libraries within the package." % len(libraries) )

  # Find external libraries. Any libraries referred to with @.* relative paths are treated as already in the package.
  # ITS NOT THIS SCRIPT'S JOB TO FIX BROKEN INSTALL RULES.

  external_libraries = commands.getoutput(
    'find %s | xargs file | grep "Mach-O" | sed "s/:.*//" | xargs otool -l | grep " name" | sort | uniq | sed "s/name\ //" | grep -v "@" | sed "s/ (offset.*)//"' % App)

  mLibraries = set()
  for lib in external_libraries.split():
    if not isexcluded(lib):
      logging.info( "Processing " + str(lib) + "" )
      mLibraries.add(Library.createFromReference(lib, "%s/Contents/MacOS/foo" % App))

  logging.info( "Found %d direct external dependencies." % len(mLibraries) )

  def recursive_dependency_scan(base, to_scan):
    dependencies = set()
    for lib in to_scan:
      dependencies.update(lib.dependencies("%s/Contents/MacOS" % App))
    dependencies -= base
    # Now we have the list of non-packaged dependencies.
    dependencies_to_package = set()
    for dep in dependencies:
      if not isexcluded(dep.RealPath):
        dependencies_to_package.add(dep)
    if len(dependencies_to_package) > 0:
      new_base = base | dependencies_to_package
      dependencies_to_package |= recursive_dependency_scan(new_base, dependencies_to_package)
      return dependencies_to_package
    return dependencies_to_package

  indirect_mLibraries = recursive_dependency_scan(mLibraries, mLibraries)
  logging.info( "Found %d indirect external dependencies." % (len(indirect_mLibraries)) )
  logging.info( indirect_mLibraries )
  logging.info( "" )
  mLibraries.update(indirect_mLibraries)

  logging.info( "------------------------------------------------------------" )
  install_name_tool_command = []
  for dep in mLibraries:
    dep.copyToApp(App)
    install_name_tool_command += dep.generate_install_commands()
  logging.info( "" )

  install_name_tool_command = " ".join(install_name_tool_command)

  # If Qt Plugins dir is specified, copies those in right now.
  # We need to fix paths on those too.
  # Currently, we are not including plugins in the external dependency search.
  if QtPluginsDir:
    logging.info( "------------------------------------------------------------" )
    logging.info( "Copying Qt plugins " )
    logging.info( "  %s ==> .../Contents/Plugins" % QtPluginsDir )
    commands.getoutput('cp -R "%s/" "%s/Contents/Plugins"' % (QtPluginsDir, App))

  logging.info( "------------------------------------------------------------" )
  logging.info( "Running 'install_name_tool' to fix paths to copied files." )
  logging.info( "" )
  # Run the command for all libraries and executables.
  # The --separator for file allows helps use locate the file name accurately.
  binaries_to_fix = commands.getoutput('find %s -type f | xargs file --separator ":--:" | grep -i ":--:.*Mach-O" | sed "s/:.*//" | sort | uniq ' % App).split()


  result = ""
  for dep in binaries_to_fix:
    commands.getoutput('chmod u+w "%s"' % dep)
    logging.info('install_name_tool %s "%s"' % (install_name_tool_command, dep))
    output = commands.getoutput('install_name_tool %s "%s"' % (install_name_tool_command, dep))
    logging.info(output)

    commands.getoutput('chmod a-w "%s"' % dep)
