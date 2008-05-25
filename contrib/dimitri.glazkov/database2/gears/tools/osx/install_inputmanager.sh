#!/bin/bash

# Leopard will only accept InputManagers if they're permissions are set just so.
# This script does the dirty work of installing the Gears InputManager 
# correctly.

SCRIPT_DIR=`pwd`/`dirname $0`

# Make sure script is run as root.
if [ $USER != 'root' ]; then
  sudo $0
  exit 0
fi

# Always install from release directory.
if [ ! -d "$SCRIPT_DIR/build/Release/GoogleGearsEnabler" ]; then
  echo "please compile the InputManager in release mode, then run this script."
  exit 1
fi

INPUT_MGR_DIR="/Library/InputManagers/"
BUNDLE_DESTINATION="$INPUT_MGR_DIR/GoogleGearsEnabler"

mkdir -p "$INPUT_MGR_DIR"
rm -rf "$BUNDLE_DESTINATION"
cp -R "$SCRIPT_DIR/build/Release/GoogleGearsEnabler" "$BUNDLE_DESTINATION"
chmod -R go-w "$BUNDLE_DESTINATION"
chown -R root:admin "$BUNDLE_DESTINATION"
