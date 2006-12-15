#!/bin/sh
#
# TODO: copy the patch to the SVN repository instead of fetching it from the web
#

rm -Rf x264-build
svn co svn://svn.videolan.org/x264/trunk x264-build
pushd x264-build
wget http://dbservice.com/ftpdir/tom/seom.patch
patch -p0 < ./seom.patch
./configure --enable-mp4-output --enable-pthread
make -j2
popd

rm -f seom-x264
cp x264-build/x264 seom-x264
rm -Rf x264-build
