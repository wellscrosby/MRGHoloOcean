#!/bin/bash

# This script packages the entire project

ue4 setroot /home/ue4/UnrealEngine

packagename="TestWorlds"
commit="$1"

echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"
echo "⚠ Packaging $packagename..."
echo "⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠"

# #move our worlds into it
# mkdir Content/Worlds
# mv our_worlds/* Content/Worlds/

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
cp ../Content/Config/*.json .
cp ../Content/Config/*.csv .

echo "👉 Compressing contents into $commit.zip..."
zip -r "$commit.zip" *

echo "👉 Deleting config files for $commit..."
rm *.json # This may be removable? Or we may want to add in removing the csv files too
cd ..

echo "👉 Moving $commit.zip into final folder..."
mkdir final
mkdir tag
mv "dist/$commit.zip" final
cp final/$commit.zip final/latest.zip
cp final/$commit.zip tag/Linux.zip

echo "👉 Done packaging package $commit"

echo "👉 Sucessfully packaged all the packages 🎉🎉"
