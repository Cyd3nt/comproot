MAIN = src/comproot
HANDLERS.C = $(wildcard src/handlers/handle_*.c)
HANDLERS.H = src/handlers/handlers.h
DECL_HANDLERS.H = src/handlers/decl_handlers.h
SRC.C = $(wildcard src/*.c)
OBJS = \
	$(SRC.C:.c=.o) \
	$(HANDLERS.C:.c=.o)

CFLAGS_ALL = -D_GNU_SOURCE -Wall -Wextra -Wpedantic $(CFLAGS)
LIBS = -lseccomp

all: $(MAIN)

%.o: %.c
	$(CC) $(CFLAGS_ALL) -o $@ -c $<

$(HANDLERS.H): $(HANDLERS.C)
	sed -n '/^DECL_HANDLER(/{s//X(/;s/ {$$//;p}' $(HANDLERS.C) \
		> $@

$(DECL_HANDLERS.H): $(HANDLERS.C)
	sed -n '/^DECL_HANDLER(/s/ {$$/;/p' $(HANDLERS.C) \
		> $@

$(MAIN).o: $(MAIN).c $(HANDLERS.H) $(DECL_HANDLERS.H)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(MAIN) $(OBJS) $(HANDLERS.H) $(DECL_HANDLERS.H)

#
# Maintainer targets:
#

.PHONY: list-todo
list-todo:
	@awk '/^[\t][^\t]/{print $$1}' docs/TODO

.PHONY: list-incomplete
list-incomplete:
	@awk '/^[\t][\t][^\t]/{print $$1}' docs/TODO

.PHONY: list-done
list-done:
	@awk '/^[\t][\t][\t][^\t]/{print $$1}' docs/TODO
