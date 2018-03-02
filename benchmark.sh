./mmap-run.sh sample.txt > harddrive.log
cp sample.txt /tmpfs/.
./mmap-run.sh /tmpfs/sample.txt > ram.log
cp sample.txt /nvdimm/.
./mmap-run.sh /nvdimm/sample.txt > nvdimm.log
x86info -a | grep Cache > cacheinfo.log

