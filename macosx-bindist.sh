# A script for creating pre-compiled binaries for Mac OS X.
# This script is intended to be executed unter Mac OS X 10.6.

# Usage:
# macosx-bindist.sh [10.4|10.5|10.6]

# Version, passed in the first argument, can be 10.4, 10.5 or 10.6.
# Default: 10.6.
if [ -z $1 ]; then
  version=10.6
else
  case $1 in
    10.4 | 10.5 | 10.6 )
      version=$1
      ;;
    * )
      echo Bad version \"$1\".
      echo The version must be 10.4, 10.5 or 10.6.
      exit 1
      ;;
  esac
fi

# We need a local installtion of Lisa-net Gimp port.
if [ ! -d ~/src/macports/Gimp-app ]; then
  echo "Directory ~/src/macports/Gimp-app does not exist."
  exit 1
fi

# Create in /tmp/skl a symbolic link Gimp.app to ~/src/macports/Gimp-app.
rm -rf /tmp/skl
mkdir -p /tmp/skl
cd /tmp/skl
ln -s ~/src/macports/Gimp-app Gimp.app
cd -

# We need this directory a few times in this script.
app=/tmp/skl/Gimp.app/Contents/Resources

# Extract the directory name of this script.
dirname=`dirname $0`

# The configure script is located in the same directory as this script.
cfg_script=$dirname/configure

# Add /tmp/skl/Gimp.app/Contents/Resources/bin to PATH.
export PATH=$PATH:$app/bin

# Variables needed in cross-compiling for 10.4 and 10.5.
if [ $version = 10.4 ]; then
  export OSX_SDK="/Developer/SDKs/MacOSX10.4u.sdk"
  export MACOSX_DEPLOYMENT_TARGET="10.4"
elif [ $version = 10.5 ]; then
  export OSX_SDK="/Developer/SDKs/MacOSX10.5.sdk"
  export MACOSX_DEPLOYMENT_TARGET="10.5"
fi

# Variables passed to configure script.
if [ $version = 10.4 ]; then
  cc="gcc-4.0"
  cflags="-isysroot $OSX_SDK -arch i386 -O2"
  ldflags="-Wl,-syslibroot,$OSX_SDK -arch i386"
elif [ $version = 10.5 ]; then
  cc=
  cflags="-isysroot $OSX_SDK -arch i386 -O2"
  ldflags="-Wl,-syslibroot,$OSX_SDK -arch i386"
elif [ $version = 10.6 ]; then
  cc=
  cflags="-O2"
  ldflags=
fi

# Remember where we are.
here=`pwd`

# The binaries will be installed here.
installdir=$here/macosx-$version

$cfg_script --enable-user-install \
            --with-user-install-dir=$installdir \
            --with-prefix=/tmp/skl/Gimp.app/Contents/Resources \
	    CC="$cc" CFLAGS="$cflags" LDFLAGS="$ldflags"

# Make sure that the installation is empty before we start.
rm -rf $installdir
mkdir -p $installdir

make
make install

# Script for creating Mac OS X zip.
zip_script=$dirname/macosx-makezip.sh

$zip_script $installdir $version
