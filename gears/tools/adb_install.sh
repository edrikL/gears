#!/bin/bash

ARCH=arm
DEFAULT_MODE="dbg"
DEFAULT_URL="http://www.google.com"

VERSION_MK_PATH=../../opensource/gears/tools/version.mk
MAJOR=`cat $VERSION_MK_PATH | sed '/./!d; /#/d; /MAJOR =/!d; s/MAJOR = //'`
MINOR=`cat $VERSION_MK_PATH | sed '/./!d; /#/d; /MINOR =/!d; s/MINOR = //'`
BUILD=`cat $VERSION_MK_PATH | sed '/./!d; /#/d; /BUILD =/!d; s/BUILD = //'`
PATCH=`cat $VERSION_MK_PATH | sed '/./!d; /#/d; /PATCH =/!d; s/PATCH = //'`
VERSION=$MAJOR.$MINOR.$BUILD.$PATCH

ADB="$ANDROID_BUILD_TOP/out/host/linux-x86/bin/adb"

function usage() {
  echo "Usage: $0 [ dbg|opt [URL] ]"
  echo "Mode defaults to ${DEFAULT_MODE}"
  echo "URL defaults to ${DEFAULT_URL}"
}

# Check for incorrect number of arguments.
if [ $# -gt 2 ]; then
  # Too many arguments.
  usage
  exit 1
fi

# Check if mode argument specified.
if [ $# -ge 1 ]; then
  MODE="$1"
  if [ "$MODE" != "dbg" ] && [ "$MODE" != "opt" ]; then
    # Bad mode.
    usage
    exit 1
  fi
else
  MODE="${DEFAULT_MODE}"
fi

# Check if URL argument specified.
if [ $# -eq 2 ]; then
  URL="$2"
else
  URL="${DEFAULT_URL}"
fi

OUTPUT_PATH=../../opensource/gears/bin-$MODE/android-$ARCH/npapi

ANDROID_DEVICE_BROWSER_DIR=/data/data/com.android.browser
ANDROID_DEVICE_INSTALL_DIR=$ANDROID_DEVICE_BROWSER_DIR/app_plugins
NPAPI_MODULE_DLL=libgears.so
PLUGIN_NAME=gears.so
PLUGIN_RESOURCES_DIR=$ANDROID_DEVICE_INSTALL_DIR/gears-$VERSION
NPAPI_JAR=gears-android.jar
HTML_FILES_PATH=$OUTPUT_PATH/genfiles

if ! [ -e $OUTPUT_PATH/$NPAPI_MODULE_DLL ]; then
  echo "Cannot find $OUTPUT_PATH/$NPAPI_MODULE_DLL"
  echo "Have you built with MODE=${MODE}?"
  exit 1
fi

# Check if emulator/device connected.
$ADB get-serialno | grep -q unknown
CHECK_DEVICE=$?
if [ $CHECK_DEVICE = 0 ]; then
  echo "Android device not connected (or defunct adb)"
  echo "New version NOT pushed to the device"
  exit 1
fi
# Kill the browser process if running.
PID=$($ADB shell ps | grep browser | gawk '{print $2}')
if [ "${PID}" != "" ]; then
  echo "Killing browser process ${PID}"
  $ADB shell kill ${PID} 2>/dev/null
fi

echo "Installing $MODE build (v$VERSION) to target"
# Create directories on the target.
$ADB shell mkdir $ANDROID_DEVICE_BROWSER_DIR >/dev/null
$ADB shell mkdir $ANDROID_DEVICE_INSTALL_DIR >/dev/null
$ADB shell mkdir $PLUGIN_RESOURCES_DIR >/dev/null
# Make a copy of the library and strip it.
cp $OUTPUT_PATH/$NPAPI_MODULE_DLL $OUTPUT_PATH/$NPAPI_MODULE_DLL.stripped
$ANDROID_TOOLCHAIN/*-strip $OUTPUT_PATH/$NPAPI_MODULE_DLL.stripped
# Push the stripped library to the target.
$ADB push $OUTPUT_PATH/$NPAPI_MODULE_DLL.stripped \
          $ANDROID_DEVICE_INSTALL_DIR/$PLUGIN_NAME 2>/dev/null
# Push the .JAR file to the target, if built.
if [ -e $OUTPUT_PATH/$NPAPI_JAR ]; then
  $ADB push $OUTPUT_PATH/$NPAPI_JAR $PLUGIN_RESOURCES_DIR 2>/dev/null
else
  echo "No JAR file found, skipping"
fi
# Copy the unstripped library to the Android build directory.
cp $OUTPUT_PATH/$NPAPI_MODULE_DLL $ANDROID_PRODUCT_OUT/symbols/system/lib/$PLUGIN_NAME
# Push 

for htmlfile in `ls $HTML_FILES_PATH/*.html.compress`
do
  filename=$(basename $htmlfile .compress)
  $ADB push $htmlfile $PLUGIN_RESOURCES_DIR/$filename 2>/dev/null
done

echo "Restarting browser"
$ADB shell am start "${URL}"
