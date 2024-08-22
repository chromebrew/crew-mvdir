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

#define _XOPEN_SOURCE 700 // for nftw()
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "./mvdir.h"

struct mvdir_opts *opts;

void copy_and_delete_file(const struct stat *src_info, const char *src_path, const char *dst_path) {
  // copy_and_delete_file(): copy a file and delete it after copying, used as a fallback when rename() does not work
  //                         (i.e source and destination are not in the same filesystem)
  int src_fd = open(src_path, O_RDONLY),
      dst_fd = open(dst_path, O_CREAT | O_WRONLY, src_info->st_mode);

  if (src_fd == -1) {
    fprintf(stderr, "%s: open() failed: %s\n", src_path, strerror(errno));
    exit(errno);
  } else if (dst_fd == -1) {
    fprintf(stderr, "%s: open() failed: %s\n", dst_path, strerror(errno));
    exit(errno);
  }

  // copy contents
  if (sendfile(dst_fd, src_fd, (off_t*) 0, src_info->st_size) == -1) {
    fprintf(stderr, "%s: sendfile() failed: %s\n", dst_path, strerror(errno));
    exit(errno);
  }

  close(src_fd);
  close(dst_fd);

  // remove source file after copying
  if (remove(src_path) == -1) {
    fprintf(stderr, "%s: failed to remove file: %s\n", src_path, strerror(errno));
    exit(errno);
  }
}

int move_file(const char *src_path, const struct stat *src_info, int flag, struct FTW *ftwbuf) {
  char dst_path[PATH_MAX];

  strcpy(dst_path, opts->dst);
  strcat(dst_path, src_path + strlen(opts->src));

  if (strcmp(src_path, ".") == 0) return 0;
  if (opts->verbose) fprintf(stderr, "%s -> %s\n", src_path, dst_path);

  switch (flag) {
    case FTW_F:
    case FTW_SL:
    case FTW_SLN:
      if (access(dst_path, F_OK) == 0) {
        // do not touch existing files if -n specified
        if (!(opts->no_clobber || remove(dst_path) == 0)) {
          // remove file with same name in dest (if exist)
          fprintf(stderr, "%s: failed to remove file: %s\n", dst_path, strerror(errno));
          exit(errno);
        }
      }

      // move (rename) file to dest
      if (opts->same_fs) {
        // (when source and destination are in the same filesystem)
        // file/symlink: move file to dest, override files with same filename in dest (mode/owner remain unchanged)
        if (rename(src_path, dst_path) == -1) {
          if (errno == EXDEV) {
            // fallback to copying-deleting if source and destination are not in the same filesystem
            opts->same_fs = false;
            if (opts->verbose) fprintf(stderr, "Warning: destination is not in the same filesystem, will fallback to sendfile() instead.\n");
            return move_file(src_path, src_info, flag, ftwbuf);
          }

          fprintf(stderr, "%s: rename() failed: %s\n", src_path, strerror(errno));
          exit(errno);
        }
      } else {
        // (when source and destination are not in the same filesystem)
        if (flag == FTW_F) {
          // file: copy source file to destination and delete it (mode will be transferred)
          copy_and_delete_file(src_info, src_path, dst_path);
        } else {
          // symlink: create an identical symlink and delete the source (mode will be transferred)
          char target[PATH_MAX];

          ssize_t target_len = readlink(src_path, target, PATH_MAX - 1);
          target[target_len] = 0;

          if (symlink(target, dst_path) == -1) {
            fprintf(stderr, "%s: symlink() failed: %s\n", dst_path, strerror(errno));
            exit(errno);
          }

          // remove source after copying
          if (remove(src_path) == -1) {
            fprintf(stderr, "%s: failed to remove symlink: %s\n", src_path, strerror(errno));
            exit(errno);
          }
        }
      }

      break;
    case FTW_D:
      // directory: create an identical directory in destination if not exist (mode will be transferred)
      if (access(dst_path, F_OK) == -1) {
        mode_t dir_mode = src_info->st_mode;

        if (opts->verbose) fprintf(stderr, "Creating directory %s\n", dst_path);
        if (mkdir(dst_path, dir_mode) == -1) {
          fprintf(stderr, "%s: mkdir() failed: %s\n", src_path, strerror(errno));
          exit(errno);
        }
      }

      break;
    case FTW_NS:
      // error
      fprintf(stderr, "%s: stat failed! (%s)\n", src_path, strerror(errno));
      exit(errno);
  }
  return 0;
}

int move_directory(struct mvdir_opts *optPtr) {
  opts = optPtr;

  // trailing slashes in path are required in order to make move_file() works
  int src_len = strlen(opts->src),
      dst_len = strlen(opts->dst);

  if (opts->src[src_len - 1] != '/') opts->src[src_len] = '/';
  if (opts->dst[dst_len - 1] != '/') opts->dst[dst_len] = '/';

  // call move_file() with files in src recursively
  int ret = nftw(opts->src, move_file, 100, FTW_PHYS | FTW_MOUNT);

  if (ret == -1) {
    fprintf(stderr, "%s: nftw() failed: %s\n", opts->src, strerror(errno));
    exit(errno);
  }

  return ret;
}
