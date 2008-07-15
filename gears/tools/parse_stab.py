#!/usr/bin/env python
#
# Copyright 2008, Google Inc.  All Rights Reserved
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

__author__ = 'zork@google.com (Zach Kuznia)'
""".stab to .js converter.

usage: %s [-Dxx=yy][-DOFFICIAL_BUILD=1] -DI18N_LANGUAGES=(locales) target source.stab translation_dir

    -Dxx=yy             : Replace instances of xx in strings with yy
    -DI18N_LANGUAGES=(locales)
                        : locales should be comma separated list of locales
                          that strings should be generated for.
    -DOFFICIAL_BUILD=1  : If OFFICIAL_BUILD is defined, warnings will be
                          treated as errors.
    source.stab         : File containing the source versions of the strings
    translation_dir     : Base directory for translated versions of the
                          strings.  It's assumed that each translated version
                          lives in a subdirectory named by its locale.

Example:
    parse_stab.py -DI18N_LANGUAGES=(en-US,ja) ../bin-dbg/win32-i386/ff2/genfiles/permissions_dialog.js ../ui/common/permissions_dialog.stab ../ui/generated

This takes in a set of string table files, and produces a .js file that can be
included in our html dialogs.
"""

import os
import re
import sys

kJavascriptTemplate = """
// Insert all localized strings for the specified locale into the div or span
// matching the id.
function loadI18nStrings(locale) {
  var rtl_languages = ['he', 'ar', 'fa', 'ur'];

  if (!locale) {
    locale = 'en-US';
  } else {
    if (!localized_strings[locale]) {
      // For xx-YY locales, determine what the base locale is.
      var base_locale = locale.split('-')[0];

      if (localized_strings[base_locale]) {
        locale = base_locale;
      } else {
        locale = 'en-US';
      }
    }
  }

  var strings = localized_strings[locale];

  // If the specified locale is an right to left language, change the direction
  // of the page.
  for (index in rtl_languages) {
    if (locale == rtl_languages[index]) {
      document.body.dir = "rtl";
      break;
    }
  }

  // Copy each string to the proper UI element, if it exists.
  for (name in strings) {
    if (name == 'string-html-title') {
      window.title = strings[name];
    } else {
      var element = dom.getElementById(name);
      if (element) {
        element.innerHTML = strings[name];
      }
    }
  }
}
"""


def getStrings(filename):
  """Read in the strings from the filename, and store them in a dictionary.
  An empty file is considered a valid input.
  """
  contents = open(filename, 'r').read()

  # Match anything in the form <string id="string-name">String data</string>
  string_regex = re.compile(r'<string\s+id="([^"]*)">(.*?)</string>', re.DOTALL)

  # In order to check that the file is properly formatted, we remove all
  # matches to our regex, then check that no non-whitespace characters remain.
  string_extra = string_regex.sub('', contents)
  if re.search(r'\S', string_extra):
    print "Error: Extraneous characters: %s" % string_extra
    sys.exit(1)

  string_matches = string_regex.findall(contents)

  strings = {}
  for match in string_matches:
    string_name = match[0]
    string_text = match[1]

    # Canonicalize the strings.
    strings[string_name] = re.sub(r'</?TRANS_BLOCK(?: desc="[^"]*")?>', '',
                                  string_text)
    strings[string_name] = re.sub(r'\s+', ' ', strings[string_name]).strip()

  return strings

def createJavaScriptFromStrings(localized_strings):
  """Generate .js code containing the strings.  It'll look like:

var localized_strings = {
  "en-US": {
    "string-pie": "pie is delicious"
  },
  "ja": {
    "string-pie": "pai ga oishii desu"
  }
}
  """
  output = 'var localized_strings = {'
  first_locale = True

  for locale, strings in localized_strings.items():
    if first_locale:
      first_locale = False
    else:
      output += ','
    output += '\n  "%s": {' % (locale)

    first_string = True
    for id, string in strings.items():
      if first_string:
        first_string = False
      else:
        output += ','

      string = string.replace('"', '\\"')
      output += '\n    "%s": "%s"' % (id, string)

    output += '\n  }'

  output += '\n};\n'

  # Append the function that loads the strings into the dialog.
  output += kJavascriptTemplate
  return output


def getDefines(argv):
  """Extract any defines from the arg list, and put them in a dictionary.

  Args:
    argv - list of arguments to extract defines from.
  """
  defines = {}

  for arg in argv[1:]:
    if arg.startswith('-D'):
      # Check for any arguments of the format -Dxx=yy or -Dxx="yy"
      match = re.search(r'-D([^=]*)="?(.*?)"?$', arg)
      if not match:
        print 'Bad argument: %s' % arg
        sys.exit(1)

      define_name = match.group(1)
      define_value = match.group(2)
      if define_name == '' or define_value == '':
        print 'Bad argument: %s' % arg
        sys.exit(1)

      defines[define_name] = define_value

  return defines


def main(argv):
  if len(argv) < 4:
    print __doc__ % argv[0]
    sys.exit(1)

  locales = []

  translation_dir = argv.pop(-1)
  source_file = argv.pop(-1)
  target_file = argv.pop(-1)
  defines = getDefines(argv)

  # If this is an official build, warnings are treated as errors.
  treat_warnings_as_errors = defines.get('OFFICIAL_BUILD') != None

  # Languages are specified via a define.
  if defines.has_key('I18N_LANGUAGES'):
    raw_locales = re.sub(r'[()]', '', defines['I18N_LANGUAGES'])
    locales = re.split(r',', raw_locales)

  filename = os.path.basename(source_file)

  # The file specified as the source is considered the most up to date set of
  # strings.  The translated files are compared to this to determine if the
  # localized strings match.
  source_strings = getStrings(source_file)

  strings = {}
  for locale in locales:
    localized_strings = {}
    try:
      localized_strings = getStrings(os.path.join(translation_dir, locale,
                                                  filename))
    except:
      print "Warning: %s missing for locale %s" % (filename, locale)
      if treat_warnings_as_errors:
        sys.exit(2)

    # This block simply checks if the localized strings are out of date.
    if locale == "en-US":
      if len(localized_strings) > len(source_strings):
        print "Warning: Strings are out of date, build is not localized."
        if treat_warnings_as_errors:
          sys.exit(2)
      else:
        for string_id, string in source_strings.items():
          # If the english string is missing or different from the source, the
          # string is out of date.
          if ((not localized_strings.has_key(string_id))
              or localized_strings[string_id] != source_strings[string_id]):
            print "Warning: Strings are out of date, build is not localized."
            if treat_warnings_as_errors:
              sys.exit(2)
            break

    strings[locale] = {}

    for id in source_strings.keys():
      # If there is no localized string, substitute the string from the source.
      string = strings.get(id, source_strings[id])

      # Replace any macros as specified on the commandline
      for define, value in defines.items():
        string = string.replace(define, value)

      strings[locale][id] = string

  try:
    output_file = open(target_file, 'w')
  except IOError, err:
    print "Could not open %s for writing: %s\n" % (target_file, err.strerror)
    sys.exit(3)

  print >> output_file, createJavaScriptFromStrings(strings)

  output_file.close()


if __name__ == '__main__':
  main(sys.argv)
