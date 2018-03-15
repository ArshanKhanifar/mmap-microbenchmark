This repository contains a microbenchmark that uses mmap(2) to load a
file from the system. The microbenchmark follows the same methodologies
as the [syscall\_timing microbenchmark tool] (https://github.com/freebsd/freebsd/tree/master/tools/tools/syscall_timing) in FreeBSD.
It is used for testing `NVDIMM`'s performance in Gen9 machines.

## usage
```
$ cc -o mmap mmap.c
mmap [-i iterations] [-l loops] [-s seconds] -f filesize -p path
```

## sample screenshot of microbenchmark
```
$ ./mmap -f 100 -p sample.txt
Clock resolution: 0.000001000
test	loop	time	iterations	per-iteration
mmap_read	0	1.005074000	102038056	0.000000009
mmap_read	1	1.002209000	97507225	0.000000010
mmap_read	2	1.005069000	101301978	0.000000009
mmap_read	3	1.001100000	98339145	0.000000010
mmap_read	4	1.001014000	98030109	0.000000010
mmap_read	5	1.000977000	98597120	0.000000010
mmap_read	6	1.001026000	100037525	0.000000010
mmap_read	7	1.002810000	99390593	0.000000010
mmap_read	8	1.004042000	101932768	0.000000009
mmap_read	9	1.003278000	101852432	0.000000009
```

## results:
The microbenchmark was tried with a 32GiB file on normal filesystem,
and a 1GB file on `tmpfs` and `nvdimm`. The `mmap-run` script runs 
the executable with a size-range from 2KiBs to 1GiB for `tmpfs` and 
`nvdimm`, and up to 32GiBs for the file on the normal file system.

Since the microbenchmark strides randomly through the `mmap`ed region,
for bigger sizes, there are more cache misses, and thus the average 
read time increases. This is not, however, measuring specific cache 
level's access times. The effect of higher-level caches is more 
apparent as the mapped region's size increases.

![alt text](/gen9-data/chart.png)


