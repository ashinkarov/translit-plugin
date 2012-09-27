# Copyright (c) 2010-2012, Artem Shinkarov <artyom.shinkaroff@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

BINARY  := translit
INCLUDE := -I/usr/include/libpurple \
	   -I/usr/include/glib-2.0 \
	   -I/usr/lib/glib-2.0/include \
	   -I/usr/lib/gtk-2.0 \
	   -I/usr/lib/gtk-2.0/include \
	   -I/usr/include/pidgin \
	   -I/usr/include/gtk-2.0/ \
	   -I/usr/include/pango-1.0 \
	   -I/usr/include/cairo  \
	   -I/usr/include/atk-1.0 \
	   -I/usr/include/gdk-pixbuf-2.0

DETRANS_DEPS :=  ru-replacement.def ru-special-words.def ru-capital-letters.def trie.h
TRIE_DEPS    :=  trie.h

all: trans

detrans-input: detrans.c trie.c $(DETRANS_DEPS) $(TRIE_DEPS)
	$(CC) -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE -D_BSD_SOURCE \
	-D_DETRANS_BINARY -D_CMD_TOOL -o $@ detrans.c trie.c 

detrans-file: detrans.c trie.c $(DETRANS_DEPS) $(TRIE_DEPS)
	$(CC) -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE -D_BSD_SOURCE \
	-D_DETRANS_BINARY -D_READ_FROM_FILE -o $@ detrans.c trie.c


trans: $(BINARY).so
%.so:%.o
	$(CC) -shared -fpic -lglib-2.0 -lpurple -o $@ $^

%.o:%.c
	$(CC) $(INCLUDE) -D_GNU_SOURCE  -D_BSD_SOURCE -fpic -O3 -std=c99 \
	-Wall -Wextra  -pedantic -c -o $@ $<

detrans.o: $(DETRANS_DEPS)
trie.o: $(TRIE_DEPS)
$(BINARY).so: translit.o detrans.o trie.o

clean:
	$(RM) $(BINARY).so *.o  detrans-input  detrans-file


