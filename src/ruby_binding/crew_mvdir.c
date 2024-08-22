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
  ruby_binding/crew_mvdir.c: Ruby binding for use in `crew`

  Usage: crew_mvdir(src, dst, verbose: false)
*/

#include <string.h>
#include <ruby.h>
#include "../mvdir.h"

VALUE crew_mvdir(int argc, VALUE *argv, VALUE self) {
  struct mvdir_opts *opts = calloc(1, sizeof(opts));
  VALUE src, dst, rb_opts, verbose;

  // initialize mvdir_opts struct
  opts->src = calloc(sizeof(char), PATH_MAX);
  opts->dst = calloc(sizeof(char), PATH_MAX);

  // parse argument passed in
  rb_scan_args(argc, argv, "2:", &src, &dst, &rb_opts);

  if (NIL_P(rb_opts)) rb_opts = rb_hash_new();

  // read extra options
  verbose = rb_hash_aref(rb_opts, rb_intern("verbose"));
  if (NIL_P(verbose)) verbose = Qfalse;

  // convert Ruby values to C style data types
  strcpy(opts->src, StringValueCStr(src));
  strcpy(opts->dst, StringValueCStr(dst));
  opts->verbose = RTEST(verbose);

  move_directory(opts);
  return Qnil;
}

void Init_crew_mvdir() {
  rb_define_global_function("crew_mvdir", crew_mvdir, -1);
}
