Translit plugin for pidgin
==========================

This is a pidgin plugin for
1. De-transliteraion of the russian messages in ISO-9 translit.
2. Providing a keymap inside the piging conversations.


Building and installing
=======================

In order to build a plugin, you have to have:
* Pidgin libraries `libpurple.so`
* GTK libraries and GLib-2.0
* Headers for pango, cairo, atk, gdk-pixbuf

After that run `make` and copy `translit.so` into `~/.purple/plugins`.  On
the next start of pidgin, in plugins section you should see a plugin called
`Translit tools`; enable it, and read help for `/detrans`, `/nodetrans`,
`/rus` and `/norus`.


How does it work?
=================

This plugin is using [ISO-9](http://en.wikipedia.org/wiki/ISO_9) coding
scheme for de-transliteration of the russian messages.  For every
conversation from the user included in the de-transliterarion list, the
message is going to be de-coded using ISO-9 table and a set of custom
exceptions.  The plugin tries to leave as is html-tags, &xxxx; symbols and
urls.

The main feature of the plugin is a set of custom exceptions, which were
retrieved bu analyzing a russian dictionary of hunspell.  Consider russian
word `ушла` in a transliterated fashion -- `ushla` -- it can be decoded by
simple per-letter decoding; so far, so good.  Now let's look at `сходить`
-- `shodit'`; any naiive decoder would decode it as `шодить`, which of
course is not what you want.  In order to overcome this, one has to apply a
set of rules to recognize such a patterns.  These rules can be found in the
file called `ru-special-words.def`.  `INPUT (a, b)` means that while
de-transliteration `a` is going to be replaced with `b`.  The longest-match
principle is used while searching for the replacement candidate.

There are two more files involved in the decoding process:
1. `ru-replacement.def` which is just ISO-9 table
2. `ru-capital-letters.def` which is a table for replacing
    small russian letter with capital.

As this table can be considerably large, we are using trie data structure
for fast matching.  It works considerably well -- 4 Mb can be
detransliterated in 0.2 seconds on core i5.

Hacking
=======

De-transliteration works outside the plugin context, and one can compile
`detrans-input` binary by running `make detrans-input` which read a message
from `stdin` and outputs decoded version on the `stdout`.

`detrans-file` binary which can be built with `make detrans-binary` accepts
a file wher each line is tab-separated correct russian word and
transliterated version of it, and the binary will print out those pairs
where de-transliteration wouldn't match the original.


Todo
====

* It would be nice to come-up with a minimal set of exceptions, to decrease
  a size of the binary.
 
* One can put some effort in making the tool aware of encodings, which may 
  make it work a wee bit faster.

* Testing :)


As always, pathces and suggestions would be highly appreciated.
