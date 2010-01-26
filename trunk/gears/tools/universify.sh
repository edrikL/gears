#!/bin/sh

# Creates a universal binaries for the Gecko SDK. Intended to be run like so:
#
#   universify.sh xulrunner-sdk-i386 xulrunner-sdk-ppc

rm -rf universify_output
mkdir universify_output
for filename in $( ls $1/sdk/lib ); do
  echo $filename
  lipo -create $1/sdk/lib/$filename $2/sdk/lib/$filename -output universify_output/$filename
done

