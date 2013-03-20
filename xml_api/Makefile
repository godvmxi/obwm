target :tree.o parse.o
	gcc -I/usr/include/libxml2/ -lxml2 create.o -o create
	gcc -I/usr/include/libxml2/ -lxml2 parse.o  -o parse
tree.o :create.c
	gcc -I/usr/include/libxml2/ -lxml2 -c create.c
parse.o :parse.c
	gcc -I/usr/include/libxml2/ -lxml2 -c parse.c
clean :
	rm -rf create parse  *.o
