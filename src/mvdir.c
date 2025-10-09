/*
  Copyright (C) 2013-2025 Chromebrew Authors

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

void copy_and_delete_symlink(const char *src_path, const char *dst_path) {
  // copy_and_delete_symlink(): re-create a symlink in destination and delete the original one afterwards, used as a fallback when rename() does not work
  //                            (i.e source and destination are not in the same filesystem)
  char target[PATH_MAX];
  target[readlink(src_path, target, PATH_MAX - 1)] = 0;

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

int move_path(const char *src_path, const struct stat *src_info, int flag, struct FTW *) {
  bool dst_exist = false;
  char dst_path[PATH_MAX];
  struct stat dst_info;

  snprintf(dst_path, PATH_MAX, "%s%s", opts->dst, &src_path[strlen(opts->src)]);

  if (strcmp(src_path, ".") == 0) return 0;
  if (opts->verbose) fprintf(stderr, "%s -> %s\n", src_path, dst_path);

  if (faccessat(AT_FDCWD, dst_path, F_OK, AT_SYMLINK_NOFOLLOW) == 0) {
    // read destination path info if already exists
    dst_exist = true;

    if (lstat(dst_path, &dst_info) == -1) {
      fprintf(stderr, "%s: lstat() failed: %s\n", dst_path, strerror(errno));
      exit(errno);
    }
  }

  switch (flag) {
    case FTW_F:
    case FTW_SL:
    case FTW_SLN:
      if (dst_exist && opts->no_clobber) {
        // do not touch existing files if -n specified
        return 0;
      }

      if (dst_exist && S_ISDIR(dst_info.st_mode)) {
        fprintf(stderr, "%s: cannot overwrite directory with non-directory\n", dst_path);
        exit(EXIT_FAILURE);
      }

      if (dst_exist && remove(dst_path) == -1) {
        // remove file with same name in dest (if exist)
        fprintf(stderr, "%s: failed to remove file: %s\n", dst_path, strerror(errno));
        exit(errno);
      }

      // move (rename) file to dest when source and destination are in the same filesystem
      // file/symlink: move file to dest, override files with same filename in dest (mode/owner remain unchanged)
      if (opts->force_copying || rename(src_path, dst_path) == -1) {
        if (opts->force_copying || errno == EXDEV) {
          // fallback to copying-deleting if source and destination are not in the same filesystem
          if (opts->verbose) fprintf(stderr, "Warning: -c specified or destination is not in the same filesystem, will fallback to copying-deleting instead.\n");

          if (flag == FTW_F) {
            // file: copy source file to destination and delete it (mode will be transferred)
            copy_and_delete_file(src_info, src_path, dst_path);
          } else {
            // symlink: create an identical symlink and delete the source
            copy_and_delete_symlink(src_path, dst_path);
          }
        } else {
          fprintf(stderr, "%s: rename() failed: %s\n", src_path, strerror(errno));
          exit(errno);
        }
      }

      break;
    case FTW_D:
      if (dst_exist && !S_ISDIR(dst_info.st_mode)) {
        fprintf(stderr, "%s: cannot overwrite non-directory with directory\n", dst_path);
        exit(EXIT_FAILURE);
      }

      if (!dst_exist) {
        // directory: create an identical directory in destination if not exist (mode will be transferred)
        if (opts->verbose) fprintf(stderr, "Creating directory %s\n", dst_path);

        if (mkdir(dst_path, src_info->st_mode) == -1) {
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

  // trailing slashes in path are required in order to make move_path() works
  int src_len = strlen(opts->src),
      dst_len = strlen(opts->dst);

  if (opts->src[src_len - 1] != '/') opts->src[src_len] = '/';
  if (opts->dst[dst_len - 1] != '/') opts->dst[dst_len] = '/';

  // call move_path() with files in src recursively
  int ret = nftw(opts->src, move_path, 100, FTW_PHYS | FTW_MOUNT);

  if (ret == -1) {
    fprintf(stderr, "%s: nftw() failed: %s\n", opts->src, strerror(errno));
    exit(errno);
  }

  return ret;
}
