topdir = ..
CC = gcc
RM = rm -rvf
hide = @

bin = debug_tool
src := src
inc := -I$(topdir)/libs/pdi/client/include \
		-I$(topdir)/include \
		-Iinclude 

object := $(patsubst /%.c, /%.o, $(wildcard *.c $(src)/*.c))
.PHONY: $(bin)

$(bin): $(object)
	$(CC) -o $@ $^ $(inc)

.PHONY: clean
clean:
	$(hide)$(RM) $(bin)
