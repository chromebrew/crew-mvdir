CC ?= cc
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

crew-mvdir:
	$(CC) $(CFLAGS) src/mvdir.c src/main.c -o crew-mvdir

install: crew-mvdir
	install -Dm755 crew-mvdir $(DESTDIR)$(BINDIR)/crew-mvdir
