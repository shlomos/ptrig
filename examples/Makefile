CC := gcc
CFLAGS := -fPIC -O3 -s -DNDEBUG -Wall -Werror -I../
LDFLAGS := -lpthread -L../ -lmytrig

BINS := main
BIN_OBJS := main.o

.PHONY: clean all

all: $(BINS)

$(BINS): $(BIN_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) -Wl,-R.

%.o: plugins/%.c
	$(CC) -c $< -o $@ $(CFLAGS)

plugin: $(PLUGIN).so
	mv $^ plugins/$^

$(PLUGIN).so: $(PLUGIN).o
	$(CC) -o $@ $^ $(CFLAGS) -shared -Wl,-soname,$(PLUGIN)

rebuild: clean all

clean:
	rm -f *~ $(BINS) plugins/*.so *.o