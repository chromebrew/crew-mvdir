require 'mkmf'

$LDFLAGS += " -L#{Dir.pwd} -l:crew-mvdir.so.1"

create_makefile 'crew_mvdir'
