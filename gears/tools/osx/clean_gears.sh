#!/bin/sh

# Remove all files relating to Gears for Safari, except for the Gears data
# directory in "~/Library/Application Support/Google/Gears for Safari/".
rm -rf '~/Library/Internet Plugins/Gears.plugin'
sudo rm -rf "/Library/Internet Plug-Ins/Gears.plugin/"

# Make sure script is run as root.
if [ $USER != 'root' ]; then
  sudo $0 $*
  exit 0
fi

rm -rf "/Library/InputManagers/GearsEnabler"
rm -rf "/Library/Receipts/Gears.pkg"
rm -rf "/Library/Receipts/Google Gears.pkg"

echo "Removed Gears for Safari."
