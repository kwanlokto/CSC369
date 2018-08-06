FLAGS = -Wall -g -std=gnu99
FLAGS+=-DDEBUG_EN

DEPENDENCIES = ext2.h

all : ext2_rm ext2_ln ext2_mkdir ext2_cp ext2_ls ext2_rm_bonus

ext2_rm_bonus : ext2_rm_bonus.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

ext2_rm : ext2_rm.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

ext2_ln : ext2_ln.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

ext2_mkdir: ext2_mkdir.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

ext2_cp : ext2_cp.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

ext2_ls : ext2_ls.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

%.o: %.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

clean:
	rm -f *.o ext2_ls
