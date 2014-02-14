# A script for creating pre-compiled binaries for Win32.

# Usage:
# win32-bindist.sh

# Extract the directory name of this script.
dirname=`dirname $0`

# The configure script is located in the same directory as this
# script.
cfg_script=$dirname/configure

# Remember where we are.
here=`pwd`

# The binaries will be installed here.
installdir=$here/win32

# Execute the configure script passing the specific options for Win32.
$cfg_script \
    --with-prefix=/scratch/robinsonm/w64/usr_gtk2_22 \
    --host=i686-w64-mingw32               \
    --enable-user-install=yes              \
    --with-user-install-dir="$installdir"

# Make sure that the installation is empty before we start.
rm -rf $installdir
mkdir -p $installdir

make clean
make
make install

# Script for creating Win32 zip.
zip_script=$dirname/win32-makezip.sh

$zip_script $installdir
