#!/usr/bin/env python
#
# Copyright 2008 Google Inc.  All Rights Reserved.
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

""".stab to .js converter.

usage: ./parse_stab [-Dxx=yy][-DOFFICIAL_BUILD=1] -DI18N_LANGUAGES=(locales)
                    target source.stab translation_dir

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
    ./parse_stab.py -DI18N_LANGUAGES=(en-US,ja)
                    ../bin-dbg/win32-i386/ff2/genfiles/permissions_dialog.js
                    ../ui/common/permissions_dialog.stab ../ui/generated

This takes in a set of string table files, and produces a .js file that can be
included in our html dialogs.
"""

__author__ = 'zork@google.com (Zach Kuznia)'

import codecs
import os
import re
import sys

JAVASCRIPT_TEMPLATE = """
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
      if (!browser.ie_mobile) {
        // IE Mobile dialogs don't have a title bar.
        // Furthermore, document.title is undefined in IE Mobile on WinMo 5.
        // It's also impossible to add properties to the window object.
        // (see http://code.google.com/apis/gears/mobile.html)
        document.title = strings[name];
      }
    } else {
      var element = dom.getElementById(name);
      if (element) {
        element.innerHTML = strings[name];
      }
    }
  }

  return strings;
}
"""


# Template for the header of .rc files.  This is basically just the standard
# include files.
RC_TEMPLATE = u"""
#ifdef OS_WINCE
  #include "aygshell.h"
  #include "afxres.h"
  #include "genfiles/product_constants.h"
#else
  #include "WinResrc.h"
#endif
#include "ui/ie/string_table.h"
"""


# This is the set of languages and the LANGIDs associated with each.
LANGUAGE_IDS = {'ar': ['0401', '0x01', '0x01'],
                'bg': ['0402', '0x02', '0x01'],
                'ca': ['0403', '0x03', '0x01'],
                'cs': ['0405', '0x05', '0x01'],
                'da': ['0406', '0x06', '0x01'],
                'de': ['0407', '0x07', '0x01'],
                'el': ['0408', '0x08', '0x01'],
                'en-GB': ['0809', '0x09', '0x02'],
                'en-US': ['0409', '0x09', '0x01'],
                'es': ['0c0a', '0x0a', '0x01'],
                'et': ['0425', '0x25', '0x01'],
                'fa': ['0429', '0x29', '0x01'],
                'fi': ['040b', '0x0b', '0x01'],
                'fil': ['0464', '0x64', '0x01'],
                'fr': ['080c', '0x0c', '0x01'],
                'he': ['040d', '0x0d', '0x01'],
                'hi': ['0439', '0x39', '0x01'],
                'hr': ['041a', '0x1a', '0x01'],
                'hu': ['040e', '0x0e', '0x01'],
                'id': ['0421', '0x21', '0x01'],
                'is': ['040f', '0x0f', '0x01'],
                'it': ['0410', '0x10', '0x01'],
                'ja': ['0411', '0x11', '0x01'],
                'ko': ['0412', '0x12', '0x01'],
                'lt': ['0427', '0x27', '0x01'],
                'lv': ['0426', '0x26', '0x01'],
                'ms': ['083e', '0x3e', '0x01'],
                'nl': ['0413', '0x13', '0x01'],
                'no': ['0414', '0x14', '0x01'],
                'pl': ['0415', '0x15', '0x01'],
                'pt-BR': ['0416', '0x16', '0x01'],
                'pt-PT': ['0816', '0x16', '0x02'],
                'ro': ['0418', '0x18', '0x01'],
                'ru': ['0419', '0x19', '0x01'],
                'sk': ['041b', '0x1b', '0x01'],
                'sl': ['0424', '0x24', '0x01'],
                'sr': ['0c1a', '0x1a', '0x02'],
                'sv': ['081d', '0x1d', '0x01'],
                'th': ['041e', '0x1e', '0x01'],
                'tr': ['041f', '0x1f', '0x01'],
                'uk': ['0422', '0x22', '0x01'],
                'ur': ['0820', '0x20', '0x01'],
                'vi': ['042a', '0x2a', '0x01'],
                'zh-CN': ['0804', '0x04', '0x02'],
                'zh-TW': ['0404', '0x04', '0x01'],
                'ml': ['044c', '0x4c', '0x01'],
                'te': ['044a', '0x4a', '0x01'],
                'kn': ['044b', '0x4b', '0x01'],
                'gu': ['0447', '0x47', '0x01'],
                'or': ['0448', '0x48', '0x01'],
                'bn': ['0445', '0x45', '0x01'],
                'ta': ['0449', '0x49', '0x01'],
                'mr': ['044e', '0x4e', '0x01'],
                }


def GetStrings(filename):
  """Read in the strings from the filename, and store them in a dictionary.

  An empty file is considered a valid input.  Otherwise, a set of lines that
  look like: '<string id="STRING_ID">STRING_TEXT</string> is expected.

  Args:
    filename: Name of the file to open.

  Returns:
    A dict mapping string ids to strings.
  """
  contents = open(filename, 'r').read()

  # Match anything in the form <string id="string-name">String data</string>
  string_regex = re.compile(r'<string\s+id="([^"]*)">(.*?)</string>', re.DOTALL)

  # In order to check that the file is properly formatted, we remove all
  # matches to our regex, then check that no non-whitespace characters remain.
  string_extra = string_regex.sub('', contents)
  if re.search(r'\S', string_extra):
    print >>sys.stderr, ('Error: Extraneous characters in %s: %s' %
                         (filename, string_extra))
    return None

  string_matches = string_regex.findall(contents)

  strings = {}
  for match in string_matches:
    string_name = match[0]
    string_text = match[1]

    if string_name in strings:
      print >>sys.stderr, ('Error: Duplicate string id encountered: %s' %
                           string_name)
      return None

    # Canonicalize the strings.
    strings[string_name] = re.sub(r'</?TRANS_BLOCK(?: desc="[^"]*")?>', '',
                                  string_text)
    strings[string_name] = re.sub(r'\s+', ' ', strings[string_name]).strip()

  return strings


def CreateJavaScriptFromStrings(target_file, localized_strings):
  """Generate .js code containing the strings.

  The Javascript will look like:

  var localized_strings = {
    "en-US": {
      "string-pie": "pie is delicious"
    },
    "ja": {
      "string-pie": "pai ga oishii desu"
    }
  }

  Args:
    target_file: Name of the file to write to.
    localized_strings: Dict of the localized strings to write to the file.

  Returns:
    True on success, False on failure.
  """
  output = ['var localized_strings = {']
  first_locale = True

  for locale, strings in localized_strings.items():
    if first_locale:
      first_locale = False
    else:
      output.append(',')
    output.append('\n  "%s": {' % (locale))

    first_string = True
    for string_id, cur_string in strings.items():
      if first_string:
        first_string = False
      else:
        output.append(',')

      cur_string = cur_string.replace('"', '\\"')
      output.append('\n    "%s": "%s"' % (string_id, cur_string))

    output.append('\n  }')

  output.append('\n};\n')

  # Append the function that loads the strings into the dialog.
  output.append(JAVASCRIPT_TEMPLATE)

  try:
    output_file = open(target_file, 'w')
  except IOError, err:
    print >>sys.stderr, ('Could not open %s for writing: %s\n' %
                         (target_file, err.strerror))
    return False

  output_file.write(''.join(output))

  output_file.close()

  return True


def CreateRCFromStrings(target_file, localized_strings):
  """Generate .rc script containing the strings.

  The output will look like:

  LANGUAGE 0x09, 0x01
  STRINGTABLE DISCARDABLE
  BEGIN
    IDS_LOCALE "en-US"
    IDS_STRING_PIE "pie is delicious"
  END

  LANGUAGE 0x11, 0x01
  STRINGTABLE DISCARDABLE
  BEGIN
    IDS_LOCALE "ja"
    IDS_STRING_PIE "pai ga oishii desu"
  END

  Args:
    target_file: Name of the file to write to.
    localized_strings: Dict of the localized strings to write to the file.

  Returns:
    True on success, False on failure.
  """
  # Start with the .rc boilerplate.
  output = [RC_TEMPLATE]

  for locale, strings in localized_strings.items():
    if locale not in LANGUAGE_IDS:
      print 'Unknown locale: %s' % locale
    return False

    lang_id = unicode(LANGUAGE_IDS[locale][1])
    sublang_id = unicode(LANGUAGE_IDS[locale][2])
    outputi.append((u'\n'
                    u'LANGUAGE %s, %s'
                    u'STRINGTABLE DISCARDABLE'
                    u'BEGIN'
                    u'  IDS_LOCALE "%s"\n') % (lang_id, sublang_id, locale))

    for string_id, string in strings.items():
      string = string.replace('"', r'""')
      string = string.replace('\\n', r'\012')
      string = string.replace('\n', r'\012')
      output.append(u'  %s "%s"\n' % (unicode(string_id, 'utf_8'),
                                      unicode(string, 'utf_8')))

    output.append(u'END\n')

  try:
    output_file = codecs.open(target_file, 'w', 'utf-16')
  except IOError, err:
    print >>sys.stderr, ('Could not open %s for writing: %s\n' %
                         (target_file, err.strerror))
    return False

  output_file.write(''.join(output))

  output_file.close()

  return True


def GetDefines(argv):
  """Extract any defines from the arg list, and put them in a dictionary.

  Args:
    argv: List of arguments to extract defines from.

  Returns:
    A dict mapping define names to their values.
  """
  defines = {}

  for arg in argv[1:]:
    if arg.startswith('-D'):
      # Check for any arguments of the format -Dxx=yy or -Dxx="yy"
      match = re.match(r'-D([^=]*)="?(.*?)"?$', arg)
      if not match:
        print 'Bad argument: %s' % arg
        return None

      define_name = match.group(1)
      define_value = match.group(2)
      if not define_name or not define_value:
        print 'Bad argument: %s' % arg
        return None

      defines[define_name] = define_value

  return defines


def main(argv):
  if len(argv) < 4:
    print __doc__
    sys.exit(2)

  locales = []

  translation_dir = argv[-1]
  source_file = argv[-2]
  target_file = argv[-3]
  defines = GetDefines(argv)
  if not defines:
    sys.exit(2)

  # If this is an official build, warnings are treated as errors.
  treat_warnings_as_errors = 'OFFICIAL_BUILD' in defines

  # Languages are specified via a define.
  if 'I18N_LANGUAGES' in defines:
    raw_locales = re.sub(r'[()]', '', defines['I18N_LANGUAGES'])
    locales = re.split(r',', raw_locales)

  filename = os.path.basename(source_file)

  # The file specified as the source is considered the most up to date set of
  # strings.  The translated files are compared to this to determine if the
  # localized strings match.
  source_strings = GetStrings(source_file)

  if not source_strings:
    sys.exit(1)

  strings = {}
  for locale in locales:
    localized_strings = {}
    try:
      localized_strings = GetStrings(os.path.join(translation_dir, locale,
                                                  filename))
    except IOError:
      print 'Warning: %s missing for locale %s' % (filename, locale)
      if treat_warnings_as_errors:
        sys.exit(1)

    # This block simply checks if the localized strings are out of date.
    if locale == 'en-US':
      if len(localized_strings) > len(source_strings):
        print 'Warning: Strings are out of date, build is not localized.'
        if treat_warnings_as_errors:
          sys.exit(1)
      else:
        for string_id, source_string in source_strings.items():
          # If the english string is missing or different from the source, the
          # string is out of date.
          if localized_strings.get(string_id) != source_string:
            print 'Warning: Strings are out of date, build is not localized.'
            if treat_warnings_as_errors:
              sys.exit(1)
            break

    strings[locale] = {}

    for string_id, source_string in source_strings.items():
      # If there is no localized string, substitute the string from the source.
      cur_string = localized_strings.get(string_id, source_string)

      # Replace any macros as specified on the commandline
      for define, value in defines.items():
        cur_string = cur_string.replace(define, value)

      strings[locale][string_id] = cur_string

  # Extract the file extension from the target filename.
  match = re.search(r'\.(.*?)$', target_file)
  if not match:
    print 'Target %s does not have a file extension' % target_file
    sys.exit(1)
  file_type = match.group(1)

  parse_funcs = {'js': CreateJavaScriptFromStrings,
                 'rc': CreateRCFromStrings}

  if file_type in parse_funcs:
    if not parse_funcs[file_type](target_file, strings):
      sys.exit(1)
  else:
    print 'Unknown output file type: %s\n' % (file_type)
    sys.exit(1)


if __name__ == '__main__':
  main(sys.argv)
