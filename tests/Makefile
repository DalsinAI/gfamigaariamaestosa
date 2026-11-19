# Host tests for the Amiga pieces: make test (ARIA_SRC=<aria checkout> also applies the patches)
CC ?= cc
CFLAGS ?= -std=c99 -Wall -Wextra -Werror -O1 -fsanitize=address,undefined
test: test_camd_smf
	./test_camd_smf
	sh test-part.sh
	sh test-package.sh
	sh test_lock.sh
test_camd_smf: test_camd_smf.c ../amiga/camd_smf.c ../amiga/camd_smf.h
	$(CC) $(CFLAGS) -I../amiga -o $@ test_camd_smf.c ../amiga/camd_smf.c
clean:
	rm -f test_camd_smf
.PHONY: test clean
