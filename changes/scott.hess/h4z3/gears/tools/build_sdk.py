#!/usr/bin/python

# Copyright 2007, Google Inc.
#
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
#
#  1. Redistributions of source code must retain the above copyright notice, 
#     this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#  3. Neither the name of Google Inc. nor the names of its contributors may be
#     used to endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Creates the Gears SDK zipfile.
#
# Execute in your /googleclient/gears directory: ./tools/build_sdk.py
#
# Before running this script, create a directory in the /gears/sdk directory
# called targets and drag all the XPIs and EXEs you want to include, like
# this:
#
# [~/src/googleclient/gears]$ ls sdk/targets/
# GoogleGearsSetup.exe    gears-0.1.51.0-linux.xpi
# gears-0.1.45.0-osx.xpi  gears-0.1.52.0-win32.xpi

import glob
import os
import sys
import time
import zipfile

def AddFile(z, filename, prefix='', strip=0):
  if strip:
    fname = os.path.basename(filename)
  else:
    fname = filename
  if (prefix):
    prefix += '/'
  z.write(filename, prefix + fname)

def AddFiles(z, filenames, prefix='', strip=0):
  for f in filenames:
    AddFile(z, f, prefix, strip)

def main():
  # Make sure we are running from the right location.
  if (not os.path.exists('sdk')):
    print 'Please run this script from /googleclient/gears.'
    sys.exit(1)

  # Note: this will overwrite any existing GearsSDK.zip file, instead of
  # appending the new files. (That is the behavior we want.)
  z = zipfile.ZipFile('GearsSDK.zip', 'w', zipfile.ZIP_DEFLATED)
  os.chdir('sdk')
  AddFile(z, 'README.html')
  AddFiles(z, glob.glob('doc/*'))
  AddFiles(z, glob.glob('samples/*'))

  ff_installers = glob.glob('targets/*.xpi')
  if (ff_installers):
    AddFiles(z, ff_installers, 'install/Firefox', 1)
  else:
    print 'WARNING: no Firefox installer(s) found.  Please put in /sdk/targets.'

  ie_installers = glob.glob('targets/*.exe')
  if (ie_installers):
    AddFiles(z, ie_installers, 'install/IE', 1)
  else:
    print 'WARNING: no IE installer(s) found.  Please put in /sdk/targets.'

if __name__ == '__main__':
  main()
