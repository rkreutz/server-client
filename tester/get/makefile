SRCDIR = ../../src
CLIENTLIB = libstorage.a
SERVEREXEC = server
CONFFILE = default.conf

CFLAGS = -g -Wall
LDFLAGS = -g -Wall

default: build

init: $(SERVEREXEC)

$(SERVEREXEC): $(SRCDIR)/$(SERVEREXEC)
	ln -sf $(SRCDIR)/$(SERVEREXEC)

$(SRCDIR)/$(SERVEREXEC):
	cd $(dir $@) && $(MAKE) $(SERVEREXEC)

$(SRCDIR)/$(CLIENTLIB):
	cd $(dir $@) && $(MAKE) $(CLIENTLIB)

.PHONY: default init build

build: main

main: main.c $(SRCDIR)/$(CLIENTLIB) -lcheck -lcrypt
	$(CC) $(CFLAGS) -I $(SRCDIR) $^ -o $@

run: init storage.h main
	./$(SERVEREXEC) $(CONFFILE) &
	./main
	killall ./$(SERVEREXEC)

clean:
	-rm -rf main *.log ./$(SERVEREXEC)  

.PHONY: run
