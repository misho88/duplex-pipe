SRC=duplex-pipe.c
EXE=duplex-pipe

all: $(EXE)

$(EXE): $(SRC)
	$(CC) $< -o $@

clean:
	rm -f $(EXE)

install: $(EXE)
	install -d $(DESTDIR)/usr/local/bin
	install $(EXE) $(DESTDIR)/usr/local/bin

uninstall:
	rm -f $(DESTDIR)/usr/local/bin/$(EXE)
