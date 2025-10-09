# crew-mvdir

A command line tool for moving all files under one directory to another, override conflict files in the destination directory (merge two directories)

Equivalent of `rsync -ahHAXW --remove-source-files dir1/ dir2/`

`crew-mvdir` use `rename()` syscall to move files (instead of copying-deleting), that's why it is faster than `rsync`

## Usage
```
./crew-mvdir [-v] [-n] [-c] [src] [dst]

  -v: enable verbose mode
  -n: do not overwrite an existing file (no clobber)
  -c: force copying-deleting instead of renaming (moving) the file
```

## Compile
```shell
cc ./crew-mvdir.c -O3 -flto -o crew-mvdir
```

## License
Copyright (C) 2013-2025 Chromebrew Authors

This project including all of its source files is released under the terms of [GNU General Public License (version 3 or later)](http://www.gnu.org/licenses/gpl.txt).
