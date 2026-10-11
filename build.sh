#!/usr/bin/env bash
RELEASES=`lsb_release -is`
if [ "$RELEASES" == "CentOS" ]; then
  echo "Compiling for Centos"
  ADDITIONAL_LIBS="/usr/local/lib64/libstdc++.a /usr/local/lib/libcryptopp.a"
elif [ "$RELEASES" == "Ubuntu" ]; then
  echo "Compiling for Ubuntu"
  ADDITIONAL_LIBS="/usr/lib/x86_64-linux-gnu/libcrypto++.a"
  else
  echo "Compiling for Debian"
  ADDITIONAL_LIBS="/usr/lib/libcrypto++.a"
fi

echo "Starting Time:" `date`

BUILD_NAME=$1
if [ -z "$BUILD_NAME" ]
then
    BUILD_NAME="fpanel-core"
fi

g++ -std=c++14 \
-g \
main.cpp \
Ini/ini.c \
plog/Log.h \
Ini/inireader.cpp \
Helpers/md5.cpp \
tsl/ordered_map.h \
-o $BUILD_NAME \
-L/usr/lib \
-pthread \
-I /usr/local/src/sources/fpanel/core/ \
-I /usr/include/ \
-I /usr/include/boost/ \
-I /usr/include/mysql/ \
$ADDITIONAL_LIBS \
/usr/lib/x86_64-linux-gnu/libmysqlclient.a \
/usr/local/lib/libboost_system.a \
/usr/local/lib/libboost_program_options.a \
/usr/local/lib/libboost_filesystem.a \
/usr/local/lib/libboost_thread.a \
/usr/local/lib/libboost_regex.a \
/usr/local/lib/libboost_date_time.a \
-Wl,--whole-archive -lpthread -Wl,--no-whole-archive \
-g2 \
-w \
-ggdb \
-O3 \
-Wall \
-ldl \
-Wunused-parameter \
-Wunused-variable \
-Wparentheses \
-Wno-sign-compare \
-fpermissive \
-pedantic \
-Wextra \
-Wno-missing-field-initializers \
-Wno-return-type \
-Wno-unused-parameter \
-Wno-unused-variable \
-Wno-maybe-uninitialized \
-Wno-unused \
-fext-numeric-literals
RETVAL=$?
if [ "$RETVAL" == "0" ]
    then
      cp -rP $BUILD_NAME /usr/local/fpanel/core/sbin/$BUILD_NAME
      if [ "$RELEASES" == "CentOS" ]; then
        cp -rP $BUILD_NAME /root/rpmbuild/SOURCES/panel-core-1.0/usr/local/fpanel/core/sbin/$BUILD_NAME
        else
        cp -rP $BUILD_NAME /root/panel/packages/panel-core/usr/local/fpanel/core/sbin/$BUILD_NAME
      fi
fi
echo "Finished Time:" `date`

# Not: -static parametresi ile derlendiginde getaddrinfo yuzunden seg fault hatasi ile islemler sonlandiriliyor.