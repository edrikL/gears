#!/usr/bin/python2.2
#
# Copyright 2006 Google Inc. All Rights Reserved.
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

"""Unittest for jsoncpp module.

This file is based on test/runjsontests.py file in the original documentation.

It uses main-level setUp() function to dynamically create testSomething methods
in the JsoncppUnitTest class. One method is created for each testdata/*.json
file.

The testSomething methods then get run when googletest.main() is executed.
"""

import sys
import os
import os.path
import glob
import shutil

from google3.testing.pybase import googletest
from google3.pyglib import flags

FLAGS = flags.FLAGS

# ===========================================================================
class JsoncppUnitTest(googletest.TestCase):
  """Contains utility functions to test the jsoncpp library.

  The real test* methods are added in runtime by setUp().
  """

  def _CompareOutputs(self, expected, actual, message):
    """Compare the contents of expected and actual strings.

    Taken from runjsontests.py.
    """
    expected = expected.strip().replace('\r','').split('\n')
    actual = actual.strip().replace('\r','').split('\n')
    diff_line = 0
    max_line_to_compare = min(len(expected), len(actual))
    for index in xrange(0,max_line_to_compare):
      if expected[index].strip() != actual[index].strip():
        diff_line = index + 1
        break
    if diff_line == 0 and len(expected) != len(actual):
      diff_line = max_line_to_compare+1
    if diff_line == 0:
      return None

    def _SafeGetLine(lines, index):
      index += -1
      if index >= len(lines):
        return ''
      return lines[index].strip()

    return """  Difference in %s at line %d:
      Expected: '%s'
      Actual:   '%s'
      """ % (message, diff_line,
         _SafeGetLine(expected,diff_line),
         _SafeGetLine(actual,diff_line) )

  def _SafeReadFile(self, path):
    """Read the contents of a file or return an error string."""
    try:
        return file(path, 'rt').read()
    except IOError, e:
        return '<File "%s" is missing: %s>' % (path,e)

  def _RunOneTest(self, jsontest_executable_path, test_file):
    """Run json_test on one test and compare the results."""

    print os.path.basename(test_file),
    os.chdir(FLAGS.test_tmpdir)
    pipe = os.popen("%s %s" % (jsontest_executable_path, test_file))
    process_output = pipe.read()
    status = pipe.close()

    base_path = os.path.splitext(test_file)[0]
    actual_output = self._SafeReadFile(base_path + '.actual')
    actual_rewrite_output = self._SafeReadFile(base_path + '.actual-rewrite')
    file(base_path + '.process-output','wt').write(process_output)
    if status:
      print " Fail"
      self.fail('Parsing failed: ' + process_output)
    else:
      expected_output_path = os.path.splitext(test_file)[0] + '.expected'
      self.assert_(os.path.exists(expected_output_path))
      expected_output = file(expected_output_path, 'rt').read()
      detail = (self._CompareOutputs(expected_output, actual_output, 'input')
                or self._CompareOutputs(expected_output, actual_rewrite_output,
                                       'rewrite') )
      if detail:
        print " Fail"
        self.fail('Invalid output: ' + detail)
    print " Pass"

# ===========================================================================
# Top-level functions follow.

def _ReturnRunOneTest(json_test, dest_file):
  """Return a function object that calls it's own _runOneTest."""
  return lambda self: self._RunOneTest(json_test, dest_file)

def setUp():
  """Copy test files to temporary directory, add methods to test class.

  First third_party/jsoncpp/testdata/ directory is scanned, and all *.json an
  *.expected files are copied over to temp directory. For each *.json file, a
  method is added to JsoncppUnittest class.
  """
  test_srcdir = FLAGS.test_srcdir + '/google3/third_party/jsoncpp/testdata'
  assert(os.path.exists(test_srcdir))
  json_test = FLAGS.test_srcdir + '/google3/bin/third_party/jsoncpp/json_test'
  assert(os.path.exists(json_test))

  for filename in os.listdir(test_srcdir):
    source_file = test_srcdir + "/" + filename
    dest_file = FLAGS.test_tmpdir + "/" + os.path.basename(source_file)
    if source_file.endswith('.json') or source_file.endswith('.expected'):
      shutil.copy(source_file, dest_file)
      if filename.endswith('.json'):
        testname = os.path.splitext(filename)[0]
        # Make sure that the methods start with test.
        if not testname.startswith('test'):
          testname = 'test' + testname
        # Add a method to JsoncppUnitTest class by calling the
        # _ReturnRunOneTest function.
        setattr(JsoncppUnitTest, testname, _ReturnRunOneTest(json_test,
                                                             dest_file))

def tearDown():
  """Remove the test input and output files in the temporary directory."""
  paths = []
  for pattern in ['*.actual',
                  '*.actual-rewrite',
                  '*.rewrite',
                  '*.json',
                  '*.expected',
                  '*.process-output']:
    paths += glob.glob(FLAGS.test_tmpdir + "/" + pattern)

  for path in paths:
      os.unlink(path)

# ===========================================================================

if __name__ == '__main__':
  googletest.main()
