CC= gcc
CFLAGS= -std=c89 -pedantic -D_GNU_SOURCE

default: 
	-make clean
	-make utente
	-make nodo
	-make master
	-touch *

lib.o: lib.h
	${CC} ${CFLAGS} lib.h -o lib.o

utente.o: lib.o
	${CC} ${CFLAGS} -c utente.c -o utente.o

nodo.o: lib.o 
	${CC} ${CFLAGS} -c nodo.c -o nodo.o

master.o: lib.o
	${CC} ${CFLAGS} -c master.c -o master.o

utente: utente.o lib.o
	${CC} ${CFLAGS} utente.c -o utente
	-chmod ugo+x utente 

nodo: nodo.o lib.o
	${CC} ${CFLAGS} nodo.c -o nodo
	-chmod ugo+x nodo

master: nodo utente master.o lib.o
	${CC} ${CFLAGS} master.c -o master
	-chmod ugo+x master


clean:
	-rm -f master.o
	-rm -f master 
	-rm -f nodo.o
	-rm -f nodo
	-rm -f utente.o
	-rm -f utente
	-rm -f lib.o	
	-rm -f *.txt
	-clear 

