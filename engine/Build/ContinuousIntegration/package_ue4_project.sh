#!/bin/bash

# This script packages the entire project

ue4 setroot /home/ue4/UnrealEngine

packagename="Ocean"
commit="$1"
branch="$2"

echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"
echo "⚠ Packaging $packagename..."
echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"

#move our worlds into it
mkdir Content/Worlds
mv holodeck-ocean-worlds/* Content/Worlds/

# Package it up
echo "👉 Starting Packaging Process..."
ue4 package Development

# Make sure it worked
code=$?
if [ code -ne 0 ]; then
    >&2 echo "(╯°□°)╯︵ ┻━┻ Packaging $commit failed with code $code!"
    exit $code
fi

mkdir dist

# Open up the permissions in the output
chmod 777 dist

# Create the zip file
cd dist

echo "👉 Copying config files into output directory..."
cp ../Content/Worlds/Config/*.json .

echo "👉 Compressing contents into $commit.zip..."
zip -r "$commit.zip" *

echo "👉 Deleting config files for $commit..."
rm *.json
cd ..

echo "👉 Moving $commit.zip into $packagename folder..."
mkdir $packagename
mv "dist/$commit.zip" $packagename
cp $packagename/$commit.zip $packagename/latest.zip


echo "👉 Done packaging package $commit"

echo "👉 Sucessfully packaged all the packages 🎉🎉"
