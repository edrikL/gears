# This script is run from the gears/tools/osx directory

#Navigate to the gears/directory
cd ../..

GEN_FILE_DIR="$BUILD_DIR/genfiles"
# Ensure that the directory for the generated files exists
/bin/mkdir -p $GEN_FILE_DIR

# Convert the version informaton into a format than can be sourced into this shell script
/usr/bin/sed -Ee 's/[ ()]//g' -e 's/^#.*//g' tools/version.mk > "$GEN_FILE_DIR/GearsVersion"
source "$GEN_FILE_DIR/GearsVersion"

FRIENDLY_NAME="Google Gears"
SHORT_NAME="gears"
# Create the product_constants.h file
/usr/bin/m4 --prefix-builtins -DDEBUG=1 -DPRODUCT_VERSION=$VERSION -DPRODUCT_VERSION_MAJOR=$MAJOR -DPRODUCT_VERSION_MINOR=$MINOR -DPRODUCT_VERSION_BUILD=$BUILD -DPRODUCT_VERSION_PATCH=$PATCH -DPRODUCT_OS=osx -DPRODUCT_ARCH="x86" -DPRODUCT_GCC_VERSION="gcc4" -DPRODUCT_MAINTAINER="google" -DPRODUCT_TARGET_APPLICATION="safari" -DI18N_LANGUAGES="(en-US)" -DPRODUCT_FRIENDLY_NAME_UQ="$FRIENDLY_NAME" -DPRODUCT_SHORT_NAME_UQ="$SHORT_NAME" base/common/product_constants.h.m4 > "$GEN_FILE_DIR/product_constants.h"

echo "$GEN_FILE_DIR/product_constants.h" built.

#-------- Now Generate the UI Files ---------
# This script is run from the gears/tools/osx directory

COMMON_RESOURCES="	ui/common/button_bg.gif ui/common/button_corner_black.gif ui/common/button_corner_blue.gif ui/common/button_corner_grey.gif ui/common/icon_32x32.png ui/common/local_data.png ui/common/location_data.png"
RESOURCES_DIR="$TARGET_BUILD_DIR/$TARGET_NAME/Contents/Resources/"
GEN_UI_DIR="$BUILD_DIR/genfiles/ui"

# TODO(playmobil):Add support for localizations.
mkdir -p "$RESOURCES_DIR/en-US"
mkdir -p "$GEN_UI_DIR/en-US"
/usr/bin/m4 --prefix-builtins -DPRODUCT_FRIENDLY_NAME_UQ="$FRIENDLY_NAME" -DPRODUCT_SHORT_NAME_UQ="$SHORT_NAME" -DPRODUCT_VERSION=$VERSION ui/generated/en-US/settings_dialog.html.m4 > "$GEN_UI_DIR/en-US/settings_dialog.html"
/usr/bin/m4 --prefix-builtins -DPRODUCT_FRIENDLY_NAME_UQ="$FRIENDLY_NAME" -DPRODUCT_SHORT_NAME_UQ="$SHORT_NAME" -DPRODUCT_VERSION=$VERSION ui/generated/en-US/permissions_dialog.html.m4 > "$GEN_UI_DIR/en-US/permissions_dialog.html"
/usr/bin/m4 --prefix-builtins -DPRODUCT_FRIENDLY_NAME_UQ="$FRIENDLY_NAME" -DPRODUCT_SHORT_NAME_UQ="$SHORT_NAME" -DPRODUCT_VERSION=$VERSION ui/generated/en-US/shortcuts_dialog.html.m4 > "$GEN_UI_DIR/en-US/shortcuts_dialog.html"

tools/osx/webarchiver/webarchiver "$RESOURCES_DIR/en-US/settings_dialog.webarchive" "$GEN_UI_DIR/en-US/settings_dialog.html" $COMMON_RESOURCES
tools/osx/webarchiver/webarchiver "$RESOURCES_DIR/en-US/permissions_dialog.webarchive" "$GEN_UI_DIR/en-US/permissions_dialog.html" $COMMON_RESOURCES
tools/osx/webarchiver/webarchiver "$RESOURCES_DIR/en-US/shortcuts_dialog.webarchive" "$GEN_UI_DIR/en-US/shortcuts_dialog.html" $COMMON_RESOURCES

#-------- Generate header files to include ---------
xxd -i "$RESOURCES_DIR/en-US/settings_dialog.webarchive" > "$GEN_FILE_DIR/settings_dialog.h"
xxd -i "$RESOURCES_DIR/en-US/permissions_dialog.webarchive" > "$GEN_FILE_DIR/permissions_dialog.h"
xxd -i "$RESOURCES_DIR/en-US/shortcuts_dialog.webarchive" > "$GEN_FILE_DIR/shortcuts_dialog.h"

"$SRCROOT/gen_resource_list.py" "$GEN_FILE_DIR/resource_list.h" "$GEN_FILE_DIR/settings_dialog.h" "$GEN_FILE_DIR/permissions_dialog.h" "$GEN_FILE_DIR/shortcuts_dialog.h"

exit 0
