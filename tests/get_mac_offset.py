import os, sys

"""
struct fat_header {
    uint32_t    magic;      /* FAT_MAGIC or FAT_MAGIC_64 */
    uint32_t    nfat_arch;  /* number of structs that follow */
};

struct fat_arch {
    cpu_type_t  cputype;    /* cpu specifier (int) */
    cpu_subtype_t   cpusubtype; /* machine specifier (int) */
    uint32_t    offset;     /* file offset to this object file */
    uint32_t    size;       /* size of this object file */
    uint32_t    align;      /* alignment as a power of 2 */
};
"""

class FatHeader:
    def __init__(self, magic, nfat_arch):
        self.magic = magic
        self.nfat_arch = nfat_arch

    def __str__(self):
        return f"FatHeader(magic=0x{self.magic:x}, nfat_arch={self.nfat_arch})"

    def __repr__(self):
        return str(self)

CPU_TYPE_X86_64 = 0x01000007
CPU_TYPE_ARM64 = 0x0100000C

class FatArch:
    def __init__(self, cputype, cpusubtype, offset, size, align):
        self.cputype = cputype
        self.cpusubtype = cpusubtype
        self.offset = offset
        self.size = size
        self.align = align

    def cputype_str(self):
        if self.cputype == CPU_TYPE_X86_64:
            return "x86_64"
        elif self.cputype == CPU_TYPE_ARM64:
            return "arm64"
        return f"Unknown ({self.cputype})"

    def __str__(self):
        return f"FatArch(cputype={self.cputype_str()}, cpusubtype={self.cpusubtype}, offset={self.offset:X}, size={self.size}, align={self.align:X})"

    def __repr__(self):
        return str(self)

def main():
    # Usage:
    # python get_mac_offset.py Geometry_Dash22074
    if len(sys.argv) < 2:
        print("Usage: python get_mac_offset.py <binary>")
        sys.exit(1)

    binary = sys.argv[1]
    if not os.path.exists(binary):
        print(f"File {binary} not found")
        sys.exit(1)

    with open(binary, 'rb') as f:
        header = FatHeader(int.from_bytes(f.read(4), 'big'), int.from_bytes(f.read(4), 'big'))
        print(header)

        archs = []
        for i in range(header.nfat_arch):
            archs.append(FatArch(
                int.from_bytes(f.read(4), 'big'),
                int.from_bytes(f.read(4), 'big'),
                int.from_bytes(f.read(4), 'big'),
                int.from_bytes(f.read(4), 'big'),
                int.from_bytes(f.read(4), 'big')
            ))

        for arch in archs:
            print(arch)


if __name__ == '__main__':
    main()