OPTIONS = {
    'ida_export': 'funcs_dump_m12206.tsv',
    'found_bindings': 'MacOS-2.206-Arm.txt',
    'output': 'funcs2206-m1.csv',
    'base': 0x100000000
}

class Function:
    """
    - name: Function name
    - offset: Offset of the function in the binary
    - size: Size of the function in bytes
    """

    def __init__(self, name, offset, size):
        self.name = name
        self.offset = offset
        self.size = size

    def __str__(self):
        return f'{self.name} at 0x{self.offset:08X} ({self.size} bytes)'

    def __repr__(self):
        return str(self)

    def toCSV(self):
        return f'{self.name},{self.offset},{self.size}'

with open(OPTIONS['ida_export'], 'r') as f:
    lines = f.readlines()

funcs = []
for line in lines:
    parts = line.split("\t")
    name = parts[0]
    # Check if has ".text" in the second column
    if ".text" in parts[1] or "__text" in parts[1]:
        offset = int(parts[2], 16) - OPTIONS['base']
        size = int(parts[3], 16)
        funcs.append(Function(name, offset, size))

with open(OPTIONS['found_bindings'], 'r') as f:
    lines = f.readlines()

newFuncs = []
for line in lines:
    parts = line.split(" - ")
    if len(parts) == 2:
        name = parts[0]
        offset = int(parts[1], 16)
        for func in funcs:
            # Check if the offset matches
            if func.offset == offset:
                func.name = name
                newFuncs.append(func)
                break

# Sort by offset
newFuncs.sort(key=lambda x: x.offset)

with open(OPTIONS['output'], 'w') as f:
    f.write("Name,Offset,Size\n")
    for func in newFuncs:
        f.write(func.toCSV() + "\n")
