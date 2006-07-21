#! /bin/sh

aclocal -I m4
autoheader
autoconf
automake -a -c -f
echo autogen.sh: now run ./configure --enable-maintainer-mode
