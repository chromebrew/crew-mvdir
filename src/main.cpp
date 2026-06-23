/*
  Copyright (C) 2013-2026 Chromebrew Authors

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

  Usage: ./crew-mvdir [-c] [-n] [-v] [src] [dst]

  c++ ./main.cpp -O3 -flto -o crew-mvdir
*/

#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <unistd.h>

#define CREW_MVDIR_VERSION "1.0"

using namespace std;

int recreate_and_delete(string src, string dst) {
  try {
    filesystem::copy(src, dst, filesystem::copy_options::copy_symlinks);
    filesystem::remove(src);
  } catch (const filesystem::filesystem_error &err) {
    cerr << src << ": failed to copy file: " << err.code().message() << endl;
    return err.code().value();
  }

  return 0;
}

int move_directory(filesystem::path src, filesystem::path dst, bool force_copying, bool no_clobber, bool verbose) {
  error_code error;

  if (!filesystem::is_directory(src, error)) {
    cerr << src.string() << ": is not a directory" << endl;
    return EXIT_FAILURE;
  } else if (!filesystem::is_directory(dst, error)) {
    cerr << dst.string() << ": is not a directory" << endl;
    return EXIT_FAILURE;
  }

  for (const auto &entry : filesystem::recursive_directory_iterator(src)) {
    auto dst_path = dst / entry.path().lexically_relative(src);

    if (verbose) cerr << entry.path().string() << " -> " << dst_path << endl;

    if (!entry.is_symlink() && entry.is_directory()) {
      if (filesystem::exists(dst_path)) {
        if (!filesystem::is_directory(dst_path, error)) {
          cerr << dst_path << ": cannot overwrite non-directory with directory" << endl;
          return EXIT_FAILURE;
        }
      } else {
        // directory: create an identical directory in destination if not exist (mode will be transferred)
        try {
          if (verbose) cerr << "Creating directory " << dst_path << endl;
          filesystem::create_directory(dst_path, entry.path());
        } catch (const filesystem::filesystem_error &err) {
          cerr << entry.path().string() << ": mkdir() failed: " << err.code().value() << endl;
          return err.code().value();
        }
      }
    } else {
      // file/symlink: move entry to dest, override same-named entries in dest
      if (filesystem::exists(dst_path)) {
        // do not touch existing files if -n specified
        if (no_clobber) continue;

        // remove file with same name in dest (if exist)
        try {
          filesystem::remove(dst_path);
        } catch (const filesystem::filesystem_error &err) {
          cerr << dst_path << ": failed to remove file: " << err.code().message() << endl;
          return err.code().value();
        }
      }

      if (force_copying) {
        int ret = recreate_and_delete(entry.path(), dst_path);
        if (ret) return ret;
        continue;
      }

      // move (rename) file to dest when source and destination are in the same filesystem
      try {
        filesystem::rename(entry.path(), dst_path);
      } catch (const filesystem::filesystem_error &err) {
        if (err.code().value() == EXDEV) {
          // fallback to copying-deleting if source and destination are not in the same filesystem
          if (verbose) cerr << "Warning: -c specified or destination is not in the same filesystem, will fallback to copying-deleting instead." << endl;

          int ret = recreate_and_delete(entry.path(), dst_path);
          if (ret) return ret;
        } else {
          cerr << entry.path().string() << ": rename() failed: " << err.code().message() << endl;
          return err.code().value();
        }
      }
    }
  }

  return 0;
}

int main(int argc, char **argv) {
  bool force_copying = false,
       no_clobber    = false,
       verbose       = false;

  if (argc == 2 && strcmp(argv[1], "--version") == 0) {
    cerr << CREW_MVDIR_VERSION << endl;
    return 0;
  }

  int opt;

  while ((opt = getopt(argc, argv, "vnc")) != -1)
  {
    switch (opt)
    {
    case 'c':
      // force copying-deleting instead of renaming (moving) the file
      force_copying = true;
      break;
    case 'n':
      // do not overwrite an existing file
      no_clobber = true;
      break;
    case 'v':
      // verbose mode
      verbose = true;
      break;
    default:
      cerr << "Usage: " << argv[0] << " [-c] [-n] [-v] [src] [dst]" << endl;
      return EXIT_FAILURE;
    }
  }

  if (argc - optind != 2) {
    cerr << "Usage: " << argv[0] << " [-c] [-n] [-v] [src] [dst]" << endl;
    return EXIT_FAILURE;
  }

  return move_directory(argv[optind], argv[optind + 1], force_copying, no_clobber, verbose);
}
