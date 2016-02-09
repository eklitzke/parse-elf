# Parse ELF

This is some really bad code that can decode ELF symbol tables on x86-64 Linux
tables. Actually there is nothing specific to Linux or x86-64 here you could
probably delete those checks nad be fine.

You should be able to do something like this on a Debian or Ubuntu system (which
statically builds in the Python symbols):

```bash
$ make
$ ./parse_elf $(which python)
```

And then see a bunch of symbols that look like Python symbols, e.g.
`PyObject_Malloc` should be somewhere in the output.

Or choose your other favorite binary.

This code is not well written, but I found it pretty difficult to understand the
offsets as specified in the `elf(5)` man page, so hopefully this will help
anyone else trying to understand the string table offsets used to decode
symbols.

I personally am using this code as the basis of decoding statically compiled
symbols into a binary, but it should be straightforward to extend this code to
work with true dynamic relocations.
