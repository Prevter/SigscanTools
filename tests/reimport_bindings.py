from pybroma import Root, codewriter

# TODO: make this actually bearable to use
"""
CONFIG = {
    'old': 'Cocos2d2206.bro',
    'new': 'Cocos2d.bro',
    'output': 'Cocos2d2.bro',
    'found': 'found22073-m1.csv',
    'platform': 'm1'
}
CONFIG = {
    'old': 'GeometryDash2206in.bro',
    'new': 'GeometryDash.bro',
    'output': 'GeometryDash22073.bro',
    'found': 'found22073-imac.csv',
    'platform': 'imac'
}
CONFIG = {
    'old': 'GeometryDash2206in.bro',
    'new': 'GeometryDash.bro',
    'output': 'GeometryDash22073.bro',
    'found': 'found22073-m1.csv',
    'platform': 'm1'
}
CONFIG = {
    'old': 'GeometryDash2206in.bro',
    'new': 'GeometryDash2206ios.bro',
    'output': 'GeometryDash2206ios-new.bro',
    'found': 'found2206-ios.csv',
    'platform': 'ios',
    'old_plat': 'm1'
}
"""
CONFIG = {
    'old': 'GeometryDash2207.bro',
    'new': 'GeometryDash.bro',
    'output': 'GeometryDash22073.bro',
    'found': 'found22073.csv',
    'platform': 'win'
}

class FoundFunction:
    def __init__(self, origin, name, address):
        self.origin = origin
        self.name = name
        self.address = address
        self.func = None

    def __str__(self):
        return f"FoundFunction(0x{self.origin:x}, {self.name}, 0x{self.address:x})"

    def __repr__(self):
        return str(self)

if CONFIG['old_plat'] is None:
    CONFIG['old_plat'] = CONFIG['platform']

def get_offset(binds, platform):
    if platform == 'imac':
        return binds.imac
    elif platform == 'win':
        return binds.win
    elif platform == 'm1':
        return binds.m1
    elif platform == 'ios':
        return binds.ios

with open(CONFIG['found'], 'r') as f:
    data = f.read().splitlines()

    found_functions = []
    for line in data:
        origin, name, address = line.split(',')
        found_functions.append(FoundFunction(int(origin, 16), name, int(address, 16)))

root = Root(CONFIG['old'])
for c in root.classes:
    for f in c.fields:
        if func := f.getAsFunctionBindField():
            if not any([get_offset(func.binds, CONFIG['old_plat']) == ff.origin for ff in found_functions]):
                continue

            index = [ff.origin for ff in found_functions].index(get_offset(func.binds, CONFIG['old_plat']))
            found_functions[index].func = {
                'class': c.name,
                'function': func.prototype.name,
                'args': [a.name for a in func.prototype.args.values()]
            }

with open(CONFIG['new'], 'r', encoding='utf-8') as f:
    oldData = f.read().split('\n')

stats = {
    'existed': 0,
    'different': 0,
    'no_address': 0
}


def add_binding(line, addr, platform):
    # Several cases:
    # 1. void func(); // No address
    # 2. void func() = win 0x1234; // Already has address
    # 3. void func() = mac 0x1234; // Has address but not windows
    # 4. void func() { } // Inlined function
    # 5. void func() = mac 0x1234, win inline { } // Inlined function with address
    if f"{platform} 0x" in line:
        if f"{platform} 0x{addr:x}" in line or f"{platform} 0x{addr:X}" in line:
            stats['existed'] += 1
        else:
            stats['different'] += 1
        return line  # Already has address

    stats['no_address'] += 1

    if '{' in line:
        if ' = ' in line:
            # Case 5
            test = line.replace(' {', f", {platform} 0x{addr:x} {{")
            return test
        # Case 4
        test = line.replace(' {', f" = {platform} 0x{addr:x} {{")
        return test


    if '=' in line:
        # Case 3
        return line.replace(';', f", {platform} 0x{addr:x};")

    # Case 1
    return line.replace(';', f" = {platform} 0x{addr:x};")


def count_args(func_line):
    start_args = func_line.find('(')
    end_args = func_line.find(')')
    if start_args != -1 and end_args != -1:
        args = func_line[start_args + 1:end_args]
        if len(args) == 0:
            return 0
        # we need to parse it manually because of templates (e.g. std::unordered_map<std::string, int> have commas)
        template_level = 0
        args_count = 0
        for ch in args:
            if ch == '<':
                template_level += 1
            elif ch == '>':
                template_level -= 1
            elif ch == ',' and template_level == 0:
                args_count += 1
        return args_count + 1
    return 0


current_class = None
with open(CONFIG['output'], 'w', encoding='utf-8') as f:
    for line in oldData:
        if line.startswith('class '):
            current_class = line.split(' ')[1]

        if current_class:
            for ff in found_functions:
                if ff.func and ff.func['class'] == current_class:
                    func_name = f" {ff.func['function']}("
                    # if has 1 or more arguments, add it
                    if ff.func['args'] and len(ff.func['args']) > 0:
                        func_name += ff.func['args'][0]
                    else:
                        func_name += ')'

                    if func_name in line:
                        # also check for number of arguments to be the same
                        # find the part between ( and )
                        arg_count = count_args(line)
                        if arg_count != len(ff.func['args']):
                            continue

                        # special fixes for inlined definitions
                        if ' new ' in line:
                            print('warning: tried to break inlined function (new): ', line)
                            continue

                        # if equals sign is before the function name, it's an inlined function
                        if '=' in line and line.index('=') < line.index(func_name):
                            print('warning: tried to break inlined function (equals): ', line)
                            continue

                        # ignore lines with %
                        if ' % ' in line:
                            print('warning: ignored line with %: ', line)
                            continue

                        line = add_binding(line, ff.address, CONFIG['platform'])
                        break

        f.write(f"{line}\n")

print(stats)
