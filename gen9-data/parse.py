import sys
data = sys.stdin.readlines()

output=[]
n = 0
s = 0
size=''
for line in data: 
    words = line.split()
    if words[0] == 'SIZE:':
        if n != 0:
            avg = s/n
            output = output + [[size, str(avg)]]
        size = ''.join([str(s) for s in words[1] if s.isdigit()])
        n = 0
        s = 0
    if words[0] == 'mmap_read':
        s = s + float(words[4])*1e9
        n = n + 1    

avg = s/n
output = output + [[size, str(avg)]]

for line in output:
    print ", ".join(line)
