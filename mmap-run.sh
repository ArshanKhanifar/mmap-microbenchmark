j=1024;
k=1;
n=25;
for i in $(jot $n); do
	j=$(($j*2));
 	k=$(($k*2));
	echo 'SIZE: '$k'KiBs'
	./mmap -l 10 -f $j -p $1;
done;



