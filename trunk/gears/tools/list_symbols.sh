#!/bin/bash

# The first argument specifies the toolchain prefix.
# If the second argument is "defined", list all symbols defined in these files.
# If the second argument is "undefined", list all external dependencies.
# Read all symbols for all files specified in the remaining arguments.
# Sort, remove duplicates.
# Output to stdout.

TOOLCHAIN_PREFIX="$1"

if [ "$2" = "defined" ]
then
  GREP="grep -v" # Filter out lines with "UND" in them
elif [ "$2" = "undefined" ]
then
  GREP="grep" # Keep only lines with "UND" in them
else
  echo "Please specify 'defined' or 'undefined'" >&2
  exit 1
fi

# Get rid of arguments 1 and 2.
shift
shift

# Call readelf, process output.
"${TOOLCHAIN_PREFIX}readelf" -sW $* |
        egrep '[[:digit:]]+:' |
        cut -c46- |
        ${GREP} '^  UND' |
        cut -c7- |
        sort |
        uniq
