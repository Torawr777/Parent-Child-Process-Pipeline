CPP = gcc
FLAGS = -Wall -pedantic -g

EXEC = extra
OBJS = extra.o


all:${EXEC}

clean:
	rm -f ${EXEC}
	rm -f *.o

run:
	./${EXEC}

valgrind: ${EXEC}
	valgrind --leak-check=full --track-origins=yes ./${EXEC}

${EXEC}:${OBJS}
	${CPP} ${FLAGS} -o ${EXEC} ${OBJS}

.c.o:
	${CPP} ${FLAGS} -c $<
