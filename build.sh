#!/bin/bash -ex

mkdir -p builddir
cd builddir

# build shared library
cc -shared -fPIC ${CFLAGS} ../src/mvdir.c -o crew-mvdir.so.1

# build CLI for use in install.sh
cc ${CFLAGS} -L . -l:crew-mvdir.so.1 ../src/main.c -o crew-mvdir

# build ruby binding for use in crew
ruby ../src/ruby_binding/extconf.rb
make -j"$(nproc)"

echo 'Build completed.'
