CC=gcc
EXE=th
MAN=$(EXE).1

all: $(EXE)

$(EXE): main.o
	$(CC) -o $(EXE) $< -lpopt

main.o: main.c
	$(CC) -o $@ -Wall -O3 -c -DNDEBUG $<

.PHONY: clean
clean:
	rm -f $(EXE) main.o

.PHONY: install
install: all
	#Install executable
	install -D $(EXE) $(DESTDIR)/usr/bin/$(EXE)
	#Compress manual
	gzip -fk9 $(MAN)
	#Install manual
	install -D -m 644 $(MAN).gz $(DESTDIR)/usr/share/man/man1/$(MAN).gz
	
.PHONY: unistall
unistall:
	#Remove executable
	rm -f $(DESTDIR)/usr/bin/$(EXE)
	#Remove manual
	rm -f $(DESTDIR)/usr/share/man/man1/$(MAN).gz