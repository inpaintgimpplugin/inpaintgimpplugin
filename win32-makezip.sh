# A script for creating the zip package of Win32 binaries.

# Usage: sh win32-makezip.sh <dir>
# <dir> is the directory, where the Win32 binaries are installed.

if [ -z $1 ] ; then
    echo "Please supply the directory containing the Win32 binaries."
    echo "Usage: $0 <dir>"
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
    $cfg_script -V | \
    head -n 1      | \
    cut -f 1,3 -d " " --output-delimiter="-"`

# Append '-win32.zip' to package name to get the name of zip package.
zipname=$packagename"-$2.zip"

# Remember where we are and step into the win32 directory.
here=`pwd`
cd $1

# Create a Win32 README file.
readme_win32="README-win32.txt"
cat >$readme_win32 2>/dev/null <<EOF
Gimp Plug-in for "Inpainting"

This zip file contains a pre-compiled binary of the inpainting Gimp Plug-in 
(http://martinjrobins.github.io/inpaintGimpPlugin/). This comes as a zip file
which should be extracted into your GIMP user directory. 
For example, C:\Users\username\.gimp-2.8\ would be the correct directory for
user "username" and GIMP version 2.8.


EOF

# Create a copy of package README.
readme="README.txt"
package_readme=$here/$dirname/README
cp $package_readme $readme

# Append a file listing to Win32 readme.
tree -n >> $readme_win32

# For the convenience of Win32 users, convert the line endings to DOS.
todos -p $readme
todos -p $readme_win32

# Now put everything into the zip archive.
rm -f $here/$zipname
zip $here/$zipname -qr9D .

# Go back.
cd $here

# Congratulations.
echo "Created '$zipname'."
