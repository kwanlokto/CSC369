FLAGS = -Wall -g -std=gnu99
DEPENDENCIES = ext2.h

ext2_ls : ext2_ls.o ext2_general.o
	gcc ${FLAGS} -o $@ $^ -lm

%.o: %.c ${DEPENDENCIES}
	gcc ${FLAGS} -c $<

clean:
	rm -f *.o ext2_ls
