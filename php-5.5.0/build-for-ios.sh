#!/bin/bash

# NOTE: PHP thinks "finite" is available, but it's not. You have to disable it manually in the config.h file.

PLATFORM=iPhoneOS # iPhoneSimulator # iPhoneOS
HOST=arm-apple-darwin10 # i386-apple-darwin10 # arm-apple-darwin10
ARCH=armv7s # i386 # armv7s #armv7
SDK_VERSION=7.0

XCODE_ROOT=`xcode-select -print-path`
PLATFORM_PATH=$XCODE_ROOT/Platforms/$PLATFORM.platform/Developer
SDK_PATH=$PLATFORM_PATH/SDKs/$PLATFORM$SDK_VERSION.sdk
FLAGS="-isysroot $SDK_PATH -arch $ARCH -miphoneos-version-min=$SDK_VERSION" # -miphoneos-version-min=$SDK_VERSION -mios-simulator-version-min=$SDK_VERSION"
PLATFORM_BIN_PATH=$XCODE_ROOT/Toolchains/XcodeDefault.xctoolchain/usr/bin
CC=$PLATFORM_BIN_PATH/clang
CXX=$PLATFORM_BIN_PATH/clang++
CFLAGS="$FLAGS -std=gnu99"
CXXFLAGS="$FLAGS -std=gnu++11 -stdlib=libc++"
LDFLAGS=$FLAGS

export CC CXX CFLAGS CXXFLAGS LDFLAGS

CONFIGURE_FLAGS="--host=$HOST --enable-embed=static --without-pear --disable-opcache --without-iconv --disable-cgi --disable-shared --disable-cli --enable-mysqlnd --with-pdo-mysql --with-mysqli --with-mysql --with-tsrm-pthreads"
./configure $CONFIGURE_FLAGS