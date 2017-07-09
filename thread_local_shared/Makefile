all: tl_shared

#so: libfoobar.so

libfoobar.so: foobar.c
	gcc -fPIC -g -c -O -Wall foobar.c
	gcc -shared -Wl,-soname,libfoobar.so -o libfoobar.so foobar.o

tl_shared: libfoobar.so tl_shared.c
	gcc -g -O -o tl_shared tl_shared.c -lpthread -ldl -rdynamic

clean:
	rm -f tl_shared libfoobar.so foobar.o
