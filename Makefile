CC=gcc
FLAGS=-Wall -O2 -DUSE_MMLI -Immli
EXEC_NAME=beep
INSTALL_DIR=/usr/bin
MAN_FILE=beep.1.gz
MAN_DIR=/usr/share/man/man1

default : beep

clean :
	rm ${EXEC_NAME}

# beep : beep.c
# 	${CC} ${FLAGS} -o ${EXEC_NAME} beep.c
beep : beep.c mmli/mmli.c
	${CC} ${FLAGS} -o ${EXEC_NAME} $^

play : play.c mmli/mmli.c
	${CC} ${FLAGS} -o play $^

install :
	cp ${EXEC_NAME} ${INSTALL_DIR}
	# rm -f /usr/man/man1/beep.1.bz2
	cp ${MAN_FILE} ${MAN_DIR}
