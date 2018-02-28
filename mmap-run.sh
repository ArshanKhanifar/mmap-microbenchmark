j=256;
n=25;
for i in $(jot $n); do
	j=$(($j*2));
	echo 'SIZE: '$j
	./mmap -l 1 -f $j -p ./sample.txt mmap_read;
done;
