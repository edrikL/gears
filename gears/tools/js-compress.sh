#!/bin/bash

# Usage: ./js-compress.sh <html file> <locale> <output directory>
#
# This script takes the HTML files used for the Gears dialog
# and do various operations to reduce their size:
# - use a single locale (i.e. en-US) and strip the others
# - extract CSS and js content and compress them using YUICompressor
#   (http://developer.yahoo.com/yui/compressor/)
# - strip HTML comments and empty lines

COMPRESSOR_PATH="../../../third_party/yuicompressor/2.3.5/"
COMPRESSOR="java -jar $COMPRESSOR_PATH/yuicompressor-2.3.5.jar"

# Extract the entire javascript from <html file>
# and save it in the <html file>.js.sed.1

sed -n '
/<script>/ {
:gather
  n
  /<\/script>/ ! {
    /<script>/ ! {
      H
      b gather
    }
  }
}
$ b endhold
b
:endhold
x
w '$1.js.sed.1'
' $1

# Extract the locale from <html file>.js.sed.1
# and save it in a <locale>.locale file

sed -e '/"'$2'":/,/}/ w '$2'.locale.pre' $1.js.sed.1 2&>/dev/null
sed -e '$ s/},/}/' $2.locale.pre > $2.locale
rm $2.locale.pre

# Remove all the localized strings in <html file>.js.sed.1
# and replace them with a tag LOCALIZED_STRINGS and save
# the result in <html file>.js.sed.2

sed '
/var localized_strings = {/,/};/ c\
    LOCALIZED_STRINGS
' $1.js.sed.1 > $1.js.sed.2

# Replace LOCALIZED_STRINGS in <html file>.js.sed.2
# with the content of the <locale>.locale file and
# write the result in the <html file>.js.sed.3 file.

sed '
/LOCALIZED_STRINGS/ {
  a\
var localized_strings = {
  r '$2'.locale
  a\
};
  d
}
' $1.js.sed.2 > $1.js.sed.3

# At this stage, we can run the compression on <html file>.js.sed.3
# and save the result in <html file>.js.final

$COMPRESSOR  --line-break 80 --type js -o $1.js.final $1.js.sed.3

# Now let's play with the css...
# extract the entire css from <html file>
# and save it in <html file>.css.sed.1

sed -n '
/<style/ {
:gather
  n
  /<\/style>/ ! {
    /<style/ ! {
      H
      b gather
    }
  }
}
$ b endhold
b
:endhold
x
w '$1.css.sed.1'
' $1

# We compress the CSS in <html file>.css.sed.1
# and save it to <html file>.css.final

$COMPRESSOR  --line-break 80 --type css -o $1.css.final $1.css.sed.1

# Now we can integrate everything...

# First, let's remove comments and blank lines in <html file>

# ...Remove html comments, save the resulting file
# in <html file>.sed.1

sed '
/<!--/!b
:x
/-->/!{
N
bx
}
s/<!--.*-->//
' $1 > $1.sed.1

# ...Remove blank lines, save the resulting file
# in <html file>.sed.2

sed '
/^[ \t]*$/ d
' $1.sed.1 > $1.sed.2

# Then replace CSS and JS by tags in <html file>.sed.2
# and output the result in <html file>.sed.3

sed '
/<style/,/<\/style>/ c \
  CSS_INCLUDE_FILE
/<script/,/<\/script>/ c \
  JS_INCLUDE_FILE
' $1.sed.2 > $1.sed.3

# Coalesce CSS_INCLUDE_FILES

sed -n '
/CSS_INCLUDE_FILE/ {
  p
:continue
  n
  /CSS_INCLUDE_FILE/ b continue
}
p
' $1.sed.3 > $1.sed.4

# Coalesce JS_INCLUDE_FILES

sed -n '
/JS_INCLUDE_FILE/ {
  p
:continue
  n
  /JS_INCLUDE_FILE/ b continue
}
p
' $1.sed.4 > $1.sed.5

# Now we replace the tags by the compiled
# files and save the result in
# <output directory>/<html file>.compress

sed '
/CSS_INCLUDE_FILE/ {
  a\
  <style type="text/css">
  r '$1'.css.final
  a\
  </style>
  d
}
' $1.sed.5 > $1.sed.6

sed '
/JS_INCLUDE_FILE/ {
  a\
  <script>
  r '$1'.js.final
  a\
  </script>
  d
}
' $1.sed.6 > $3/`basename $1`.compress

# Finally, do some cleanup

rm $1.css.*
rm $1.js.*
rm $1.sed.*
