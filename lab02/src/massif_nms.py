import re, sys
lines = open(sys.argv[1]).read().split('\n')
peak = None
for i, l in enumerate(lines):
    if l == 'heap_tree=peak':
        peak = i
        break
total = 0
for l in lines[peak-3:peak]:
    if l.startswith('mem_heap_B='):
        total = int(l.split('=')[1])
nms = 0
for l in lines[peak+1:]:
    if l.startswith('#') or l.startswith('snapshot='):
        break
    m = re.match(r'\s*n\d+: (\d+) 0x[0-9A-Fa-f]+: (nms|nms_orig)\(', l)
    if m:
        nms += int(m.group(1))
print(f"peak_heap={total} nms_bytes={nms}")
