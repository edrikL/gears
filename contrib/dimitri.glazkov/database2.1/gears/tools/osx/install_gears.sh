#!/bin/bash

# Install Gears from build directory
# Usage: install_gears.sh [bin|opt]

SCRIPT_DIR=`pwd`/`dirname $0`
GEARS_DIR="$SCRIPT_DIR/../.."
BASE_DIR="$GEARS_DIR/bin-$1/installers/Safari"
GEARS_PLUGIN_PATH="$HOME/Library/Internet Plug-Ins/Gears.plugin"

#default to bin-dbg
if [ ! -n "$1" ]; then
    BASE_DIR="$GEARS_DIR/bin-dbg/installers/Safari"
fi

if [ ! -d "$BASE_DIR" ]; then
  echo "Unable to install Gears from directory \"$BASE_DIR\" which doesn't exist."
  exit 1
fi

rm -Rf "$GEARS_PLUGIN_PATH"
ln -sf "$BASE_DIR/Gears.plugin" "$GEARS_PLUGIN_PATH"
"$SCRIPT_DIR/install_inputmanager.sh" "$BASE_DIR/GoogleGearsEnabler"
