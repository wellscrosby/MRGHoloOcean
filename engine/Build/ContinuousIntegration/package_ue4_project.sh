#!/bin/bash

# This script packages the entire project

ue4 setroot /home/ue4/UnrealEngine

packagename="DEFAULT"

echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"
echo "⚠ Packaging $packagename..."
echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"

# Package it up
echo "👉 Starting Packaging Process..."
ue4 package Development

# Make sure it worked
code=$?
if [ code -ne 0 ]; then
    >&2 echo "(╯°□°)╯︵ ┻━┻ Packaging $packagename failed with code $code!"
    exit $code
fi

mkdir dist

# Open up the permissions in the output
chmod 777 dist

# Create the zip file
cd dist

echo "👉 Compressing contents into $packagename.zip..."
zip -r "$packagename.zip" *

echo "👉 Moving $packagename.zip out of dist/ folder..."
mv "$packagename.zip" ..

echo "👉 Deleting config files for $packagename..."
rm *.json

cd ..

echo "👉 Done packaging package $packagename"

echo "👉 Sucessfully packaged all the packages 🎉🎉"
