# A script for creating pre-compiled binaries for Mac OS X.
# This script is intended to be executed unter Mac OS X 10.6.

# Usage:
# macosx-bindist.sh [10.4|10.5|10.6]

# Version, passed in the first argument
# Default: 10.6.
if [ -z $1 ]; then
  version=10.6
else
  version=$1
fi

# Extract the directory name of this script.
dirname=`dirname $0`

# The configure script is located in the same directory as this script.
cfg_script=$dirname/configure

ldflags="-mmacosx-version-min=$version"

# Remember where we are.
here=`pwd`

# The binaries will be installed here.
installdir=$here/macosx-$version

$cfg_script --enable-user-install \
            --with-user-install-dir=$installdir \
	    LDFLAGS="$ldflags"

# Make sure that the installation is empty before we start.
rm -rf $installdir
mkdir -p $installdir

make clean
make
make install

# Script for creating Mac OS X zip.
zip_script=$dirname/macosx-makezip.sh

$zip_script $installdir $version
