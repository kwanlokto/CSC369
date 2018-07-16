for image in *.img
do
	echo $image
	file="${image//".img"/}"
	xxd $image > ${file}.txt
done
