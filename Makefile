MAIN = comproot
HANDLERS.C = $(wildcard handlers/handle_*.c)
HANDLERS.H = handlers/decl_handlers.h handlers/handlers.h
OBJS = \
	$(MAIN).o \
	$(HANDLERS.C:%.c=%.o) \
	file.o \
	util.o \


CFLAGS_ALL = -D_GNU_SOURCE -Wall -Wextra -Wpedantic $(CFLAGS)
LIBS = -lseccomp

all: $(MAIN)

%.o: %.c
	$(CC) $(CFLAGS_ALL) -o $@ -c $<

handlers/decl_handlers.h: $(HANDLERS.C)
	sed -n '/^DECL_HANDLER(/s/ {$$/;/p' $(HANDLERS.C) \
		> $@

handlers/handlers.h: handlers/*.c
	sed -n '/^DECL_HANDLER(/{s//X(/;s/ {$$//;p}' $(HANDLERS.C) \
		> $@

$(MAIN).o: $(MAIN).c $(HANDLERS.H)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS_ALL) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	rm -f $(OBJS) $(MAIN) $(HANDLERS.H)
