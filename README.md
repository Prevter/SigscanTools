## Epic Signature Scanning Tools
Some tools I made to pattern scan Geometry Dash bindings.  
Supports x86/x86_64 and ARMv8 architectures (basically everything GD runs on).  

Not sure if anyone will actually use this else than me, but I'm putting it here just in case :D

> Note: I really want to rewrite everything from scratch, but I'm too lazy to do it right now.
> I plan to make it more automated and user-friendly, but for now it's just a bunch of scripts.

### Step-by-step guide
Before doing anything, clone the repo and build the project.  
You need both `BindingsImporter` and `BindingsMapper` targets.

Also, you will need to install [PyBroma](https://github.com/CallocGD/PyBroma), which might be tricky.
> TODO: Rewrite python scripts to use another library or just do everything in C++.

### Step 1: Preparing the bindings
1. Using IDA Pro, open the previous version of the game (the one you will base your patterns on).
2. In the "Functions" tab, right click and click "Copy All"
3. Paste the functions in a text file, remove the first line and save it in `tests/funcs_dump.tsv`
4. Go to the [bindings cache page](https://prevter.github.io/bindings-meta) and grab a list of all bindings for that version.
Save it as `tests/Win64-2.206.txt` for example.
5. Edit the `tests/combine_bindings.py` script to point to the correct files,
and also select the correct base address (0x140000000 for windows and 0x100000000 for mac/ios).
6. Run the script, this will result in a `funcs2206.csv` file.
This file contains important metadata about the functions, such as the address, name and size.

### Step 2: Generating the patterns
1. Run the `BindingsMapper` target, passing these arguments:
```
BindingsMapper.exe funcs2206.csv output2206.csv -0xC00
```
> Note: The last offset is a "magic" number that tells the start of the .text section in the binary.  
> For Windows, this is usually `-0xC00`, for iOS it's `0x0` and for imac it's `-0x4000`.  
> On m1 macs it's different for each game version (see. ["a deep dive on macos universal binaries"](https://www.jviotti.com/2021/07/23/a-deep-dive-on-macos-universal-binaries.html)).

Also, if you're generating patterns for armv8 or x32, you have to add another argument:
```
BindingsMapper.exe funcs2206.csv output2206.csv -0xC00 arm64
```
or
```
BindingsMapper.exe funcs2206.csv output2206.csv -0xC00 x32
```

In the end, you will have a `output2206.csv` file with patterns for each function.

### Step 3: Scanning the newer version
Now run the `BindingsImporter` target, passing the following arguments:
```
BindingsImporter.exe output2207.csv found22073.csv 0xC00
```
> Last argument is the same as before, but positive this time.  
> (sorry, i'm lazy :P)

This will generate a `found22073.csv` file with the results of the scan:  
First column is the old function address (used for comparison),  
Second column is the function name  
Third column is the new function address.

### Step 4: Merging broma files
1. You will need an empty broma file that will be filled in, as well as broma file for the previous version.
2. Edit the `tests/reimport_bindings.py` script to point to the correct files.
3. Run the script, this will merge the broma files and generate a new one.

### Step 5: Profit
GG, you now have a broma file with all the new bindings.