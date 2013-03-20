#!/bin/bash
all()
{
	./configure --build=i686-pc-linux-gnu --host=i686-pc-linux-gnu --program-prefix= --disable-dependency-tracking --prefix=/usr --exec-prefix=/usr --bindir=/usr/bin --sbindir=/usr/sbin --sysconfdir=/etc --datadir=/usr/share --includedir=/usr/include --libdir=/usr/lib --libexecdir=/usr/libexec --localstatedir=/var --sharedstatedir=/var/lib --mandir=/usr/share/man --infodir=/usr/share/info --disable-static
	make
	make install
}

make_and_install()
{
	make
	make install
}

if [ $1 = "all" ]; then
	all
else
	make_and_install
fi
exit
