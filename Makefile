MAIN = comproot
SRC.o = file.o util.o chown.o $(MAIN).o
LIBS = -lseccomp

all: $(MAIN)

%.o: %.c
	$(CC) $(CFLAGS) -D_GNU_SOURCE -Wall -Wextra -Wpedantic $(LDFLAGS) -c -o $@ $<

$(MAIN): $(SRC.o)
	$(CC) $(CFLAGS) -Wall -Wextra -Wpedantic $(LDFLAGS) -o $@ $(SRC.o) $(LIBS)

clean:
	rm -f $(SRC.o) $(MAIN)
