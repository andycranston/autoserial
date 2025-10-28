autoserial: autoserial.c
	gcc -o autoserial autoserial.c

install:
	cp -p autoserial $(HOME)/bin/autoserial
	chmod u=rwx,go=rx $(HOME)/bin/autoserial
