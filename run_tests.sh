#!/bin/bash -eu

RESET='\e[0m'

# Simplify colors and print errors to stderr (2).
echo_error() { echo -e "\e[1;91m${*}${RESET}" >&2; } # Use Light Red for errors.
echo_info() { echo -e "\e[1;33m${*}${RESET}" >&1; } # Use Yellow for informational messages.
echo_info_stderr() { echo -e "\e[1;35m${*}${RESET}" >&2; } # Use Magenta for informational messages to STDERR.
echo_success() { echo -e "\e[1;32m${*}${RESET}" >&1; } # Use Green for success messages.
echo_intra() { echo -e "\e[1;34m${*}${RESET}" >&1; } # Use Blue for intrafunction messages.
echo_out() { echo -e "\e[0;37m${*}${RESET}" >&1; } # Use Gray for program output.

rm -rf test && mkdir test

(
  cd test
  mkdir -p src dst

  echo_info '[+] Test 1: Empty directory move under same filesystem'
  ../crew-mvdir -v src dst && echo_success '[+] PASS' || echo_error '[+] FAIL'

  rm -r src dst && mkdir -p src dst

  echo_info '[+] Test 2: Directory move under same filesystem (destination is empty)'
  mkdir -p src/dir1
  touch src/{file1,file2,file3}
  ln -s notexist src/symlink1
  ln -s /bin src/symlink2
  ../crew-mvdir -v src dst
  if [ -f dst/file1 ] && [ -f dst/file2 ] && [ -f dst/file3 ] && [ -d dst/dir1 ] &&
     [ -L dst/symlink1 ] && [ -L dst/symlink2 ]; then
    echo_success '[+] PASS'
  else
    echo_error '[+] FAIL'
  fi

  rm -r src dst && mkdir -p src dst

  echo_info '[+] Test 3: Directory move under same filesystem (destination is non-empty)'
  mkdir -p src/dir1 dst/dir1
  touch src/{file1,file2,file3} src/dir1/file4 dst/{file1,file2,file3}
  ln -s notexist src/symlink1
  ln -s /bin src/symlink2
  ../crew-mvdir -v src dst
  if [ -f dst/file1 ] && [ -f dst/file2 ] && [ -f dst/file3 ] && [ -f dst/dir1/file4 ] &&
     [ -L dst/symlink1 ] && [ -L dst/symlink2 ]; then
    echo_success '[+] PASS'
  else
    echo_error '[+] FAIL'
  fi

  rm -r src dst && mkdir -p src /tmp/dst

  echo_info '[+] Test 4: Directory move under different filesystem (destination is non-empty)'
  mkdir -p src/dir1 /tmp/dst/dir1
  touch src/{file1,file2,file3} src/dir1/file4 /tmp/dst/{file1,file2,file3}
  ln -s notexist src/symlink1
  ln -s /bin src/symlink2
  ../crew-mvdir -v src /tmp/dst
  if [ -f /tmp/dst/file1 ] && [ -f /tmp/dst/file2 ] && [ -f /tmp/dst/file3 ] && [ -f /tmp/dst/dir1/file4 ] &&
     [ -L /tmp/dst/symlink1 ] && [ -L /tmp/dst/symlink2 ]; then
    echo_success '[+] PASS'
  else
    echo_error '[+] FAIL'
  fi

  rm -r src /tmp/dst && mkdir -p src dst

  echo_info '[+] Test 5: Symlinked directory is preserved as symlink'
  mkdir -p src/real
  touch src/real/file
  ln -s real src/linkdir
  ../crew-mvdir -v src dst
  if [ -L dst/linkdir ] && [ "$(readlink dst/linkdir)" = 'real' ] && [ -f dst/real/file ]; then
    echo_success '[+] PASS'
  else
    echo_error '[+] FAIL'
  fi

  rm -rf src dst
)

rm -r test
