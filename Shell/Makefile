CC = gcc
DFLAGS := -g -DDEBUG
CFLAGS = -Wall -Werror -Wno-unused-variable -Iinclude  $(DFLAGS)
SRCS := $(wildcard src/*.c)
FILES := $(patsubst src/%.c,%,$(SRCS))
OBJS := $(patsubst %,build/%.o,$(FILES))

_LDBUILDS := $(patsubst %,../%,$(OBJS))
LDFLAGS := $(_LDBUILDS)  ../lib/icsutil.o
EFLAGS := $(DFLAGS) -I../include
PRG_SUFFIX := .bin

export LDFLAGS
export EFLAGS
export PRG_SUFFIX

all: setup $(FILES)
	$(MAKE) -C tests
	mv tests/*$(PRG_SUFFIX) bin/

setup:
	@mkdir -p bin build 

$(FILES):
	$(CC) $(CFLAGS) -r -c src/$@.c -o build/$@.o

clean:
	rm -rf bin/ build/ 
