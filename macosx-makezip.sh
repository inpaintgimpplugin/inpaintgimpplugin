# A script for creating the zip package of Mac OS binaries.

# Usage: sh macosx-makezip.sh <dir> <version>
# <dir> is the directory, where the Mac OS X binaries are installed.
# <version> is one of 10.4, 10.5 or 10.6.

if [ -z $1 ] ; then
    echo "Please supply the directory containing the Mac OS binaries."
    echo "Usage: $0 <dir> <version>"
    exit 1
fi

if [ ! -d $1 ] ; then
    echo "Directory '$1' does not exist."
    echo "Exiting..."
    exit 1
fi

# Extract the directory name of this script.
dirname=`dirname $0`

# The configure script is located in the same directory as this
# script.
cfg_script=$dirname/configure

# Interrogate package name from the configure script.
packagename=`\
    $cfg_script -V    | \
    head -n 1         | \
    cut -f 1,3 -d " " | \
    sed -e s/\ /-/g`

# Remember where we are and step into the Mac OS directory.
here=`pwd`
cd $1

if [ -z $2 ] ; then
    echo "Assuming Mac OS X 10.6"
    version="10.6"
else
    version=$2
fi

# Append '-macosx-<version>.zip' to package name to get the name of
# zip package.
zipname=$packagename"-macosx-"$version".zip"

# Create a Mac OS README file.
readme_macos="README-macosx-"$version".txt"
cat >$readme_macos 2>/dev/null <<EOF
Gimp Plug-in for "Image Registration"
http://gimp-image-reg.sourceforge.net/

This is the binary distribution of '$packagename'
for Mac OS X $version. To install this package, unpack the contents of
this zip file into your *personal* gimp-2.x directory named:

  ~/Library/Application\ Support/Gimp

After installing the files and restarting Gimp, the plug-in will be
available as "Image Registration..." under the menu "Tools".

Please note, that currently there is no uninstaller for this package.
Therefore, for uninstalling the package you must manually remove
the files listed here:

EOF

# Create a copy of package README.
readme="README.txt"
package_readme=$here/$dirname/README
cp $package_readme $readme

# Append a file listing to Mac OS X readme.
tree --charset ascii -n >> $readme_macos

# Now put everything into the zip archive.
rm -f $here/$zipname
zip $here/$zipname -qr9D .

# Go back.
cd $here

# Congratulations.
echo "Created '$zipname'."
