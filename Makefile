ncparse:
	$(CC) main.c parse.c -o ncparse -Wl,-framework,CoreFoundation -lsqlite3 $(CFLAGS) $(LDFLAGS)

all: ncparse
