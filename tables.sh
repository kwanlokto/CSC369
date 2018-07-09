for alg in rand fifo clock lru opt
do
	for file in traceprogs/tr-simpleloop.ref traceprogs/tr-matmul.ref traceprogs/tr-blocked.ref traceprogs/tr-bubblesort.ref
	do
		for memsize in 50 100 150 200
		do
				./sim -s 4096 -m $memsize -f $file -a $alg >> $1
		done
	done
done
