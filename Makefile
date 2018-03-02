CC := gcc
CFLAGS := -fPIC -O3 -s -DNDEBUG -Wall -Werror
LDFLAGS := -ldl

MAJOR := 0
MINOR := 1
VERSION := $(MAJOR).$(MINOR)
LIB_NAME = mytrig
LIB := lib$(LIB_NAME).so
LIB_OBJS := trigger.o plugin_manager.o

.PHONY: clean all

all: $(LIB)

$(LIB): $(LIB).$(VERSION)
	ldconfig -v -n .
	ln -sf $(LIB).$(MAJOR) $@

$(LIB).$(VERSION): $(LIB_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS) -shared -Wl,-soname,$(LIB).$(MAJOR)

rebuild: clean all

clean:
	rm -f *~ $(LIB_OBJS) $(LIB)*
