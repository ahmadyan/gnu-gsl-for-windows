#
# Python script for running GSL DLL tests 
#
# Run this from the build.vc11\dlltest directory
#
import os
import shutil
import string
import copy
import subprocess

c_dirs = [ 'Win32\\Release', 'Win32\\Debug', 'x64\\Release', 'x64\\Debug']

vcxproj_list = []
cw, f = os.path.split(__file__)
os.chdir(cw)
for root, dirs, files in os.walk('./') :
  for file in files :
    x = os.path.splitext(file)
    if x[1] == '.vcxproj' and x[0] != 'runtests' :
      vcxproj_list += [x[0]]
vcxproj_list.sort()

exe_list = [ [], [], [], [] ]
for i in range(4) :
  exe_dir = '..\\..\\dll\\' + c_dirs[i]
  if os.path.exists(exe_dir) :
    for root, dirs, files in os.walk(exe_dir) :
      for file in files :
        x = os.path.splitext(file)
        if x[1] == '.exe' :
          exe_list[i] += [x[0]]

for i in range(4) :
  if not len(exe_list[i]) :
    continue
  print("Tests for ", c_dirs[i])
  build_fail = 0
  run_ok = 0
  run_fail = 0
  exe_list[i].sort()
  for app in vcxproj_list :
    if app in exe_list[i] :
      ef = '..\\..\\dll\\' + c_dirs[i] + '\\' + app + '.exe'
      prc = subprocess.Popen( ef, stdout = subprocess.PIPE,
        stderr = subprocess.STDOUT, creationflags = 0x08000000 )
      output = str(prc.communicate()[0])
      if prc.returncode :
        print(app, ': ERROR (', prc.returncode, ' )')
        run_fail += 1
      else :
        print(app, ': success')
        run_ok += 1
      if output :
        v = output.partition('[')
        if v[0] == 'Completed ' and v[1] == '[' :
          v = v[2].partition(']')
          w = v[0].partition('/')
          if w[0] != w[2] :
            print(output, end='')
      else :
        print(output, end = '')
    else :
      print("Build failure for %s" % app)
      build_fail += 1
  print(build_fail + run_ok + run_fail, "tests:")
  if build_fail > 0 :
    print("\t%i failed to build" % build_fail)
  if run_ok > 0 :
    print("\t%i ran correctly" % run_ok)
  if run_fail > 0 :
    print("\t%i failed" % run_fail)
input(".. completed - press ENTER")
