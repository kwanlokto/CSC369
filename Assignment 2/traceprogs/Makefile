SRCS = simpleloop.c bubblesort.c matmul.c blocked.c
PROGS = simpleloop bubblesort matmul blocked

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit bubblesort
	./runit matmul 100
	./runit blocked 100 25 

.PHONY: clean
clean : 
	rm -f simpleloop bubblesort matmul blocked tr-*.ref *.marker *~
