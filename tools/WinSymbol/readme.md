# Windows Symbol Server Tools

A variety of command line tools related to storing executables and their debug databases on a symbol server. Microsoft's symstore program mostly serves the same purpose; but to my knowledge it can't upload files via scp.

## GetExeSymbolDir

Calculates and prints the name of the directory to place an executable in on a symbol server. So for example given a file `program.exe`, the call would be `GetExeSymbolDir program.exe`, which would result in an output like `123ABC` - on the symbol server the file would thus be saved as `program.exe/123ABC/program.exe`.

## GetPdbSymbolDir

Calculates and prints the name of the directory to place an pdb file in on a symbol server. So for example given a file `program.exe`, the call would be `GetExeSymbolDir program.exe`, which would result in an output like `123ABC` - on the symbol server the pdb file would thus be saved as `program.pdb/123ABC/program.pdb`. (Both .exe and .pdb files work as arguments.)

## License

Unlike OpenJK these tools use the MIT license as given in LICENSE.txt.

## Credits

Inspired by and partially based on [Bruce Dawson's blog post](https://randomascii.wordpress.com/2013/03/09/symbols-the-microsoft-way/) so major thanks him collecting this information.