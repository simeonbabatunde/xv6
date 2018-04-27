# Makefile for Lab 12
CC = gcc
CFLAGS = -g -Wall

OBJS= fscheck1.o

INCS=fscheck.h

fscheck: ${OBJS}
	@echo
	@echo Linking $@
	${CC} -o $@ ${CFLAGS} ${OBJS}

${OBJS}: ${INCS}

clean:
	rm -f *.o *.out fscheck
