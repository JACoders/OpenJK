# Language String Files

These are handled in code[mp]/qcommon/stringed_ingame.cpp and stringed_interface.cpp

Recursive folder parsing

Header (only version is required, but legacy files from Raven editor include)

```
VERSION       "1"
CONFIG        "W:\bin\stringed.cfg"
FILENOTES     "In-game text for various events"
```

each reference string is denoted by REFERENCE.  Quotes not required.  NOTES line can exist anywhere
if English there will only be LANG_ENGLISH.  If the language's string is the same as the english variant it should be "#same"

```
REFERENCE     PICKUPLINE
NOTES         "Printed before item name when pick is obtained"
LANG_ENGLISH  "Obtained"
LANG_GERMAN   "Aufgenommen:"
```

Optional token during reference parsing (quotes accepted):

```
FLAGS       	FLAG_CAPTION FLAG_TYPEMATIC
```

file ends with

```
ENDMARKER
```
