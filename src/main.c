/*
  Copyright (C) 2013-2024 Chromebrew Authors

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see https://www.gnu.org/licenses/gpl-3.0.html.
*/

/*
  crew-mvdir: Move all files under one directory to another,
              override conflict files in the destination directory (merge two directories)

  Equivalent of `rsync -ahHAXW --remove-source-files dir1/ dir2/`

  crew-mvdir use rename() syscall to move files (instead of copying-deleting), that's why it is faster than `rsync`

  Usage: ./crew-mvdir [-v] [-n] [src] [dst]

  cc ./crew-mvdir.c -O2 -o crew-mvdir
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "./mvdir.h"

int main(int argc, char** argv) {
  struct mvdir_opts *opts = calloc(1, sizeof opts);
  int opt;

  // initialize mvdir_opts struct
  opts->src = calloc(sizeof(char), PATH_MAX);
  opts->dst = calloc(sizeof(char), PATH_MAX);

  while ((opt = getopt(argc, argv, "vn")) != -1) {
    switch (opt) {
      case 'v':
        // verbose mode
        opts->verbose = true;
        break;
      case 'n':
        // do not overwrite an existing file
        opts->no_clobber = true;
        break;
      default:
        fprintf(stderr, "Usage: %s [-v] [-n] [src] [dst]\n", argv[0]);
        exit(EXIT_FAILURE);
        break;
    }
  }

  if (argc - optind != 2) {
    fprintf(stderr, "Usage: %s [-v] [-n] [src] [dst]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  strcpy(opts->src, argv[optind]);
  strcpy(opts->dst, argv[optind + 1]);

  return move_directory(opts);
}
