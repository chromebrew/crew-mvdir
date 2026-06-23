CXX ?= c++
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

.PHONY: clean test

crew-mvdir:
	$(CXX) $(CFLAGS) src/main.cpp -o crew-mvdir

install: crew-mvdir
	install -Dm755 crew-mvdir $(DESTDIR)$(BINDIR)/crew-mvdir

test: crew-mvdir
	./run_tests.sh

clean:
	rm -rf tests crew-mvdir
