target :socket_xml demos
socket_xml :socket_xml.o
	gcc -I/usr/include/libxml2/ -lxml2 socket_xml.o -o socket_xml
socket_xml.o :socket_xml.c
	gcc -I/usr/include/libxml2/ -lxml2 -c socket_xml.c
demos :demos.o
	gcc -I/usr/include/libxml2/ -lxml2 demos.o -o demos
demos.o :demos.c
	gcc -I/usr/include/libxml2/ -lxml2 -c demos.c
clean :
	-rm -rf  *.o socket_xml
