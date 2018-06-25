topdir = ../..
CC = gcc
RM = rm -rvf
hide = @

lib = libpdi.a

src :=	client/src \
	  	server/src

inc :=	-I$(topdir)/include \
	    -Iclient/include \
	    -Iserver/include

object := $(patsubst /%.c, /%.o, $(wildcard *.c $(src)/*.c))

.PHONY: $(lib)
$(lib): $(object)
	$(CC) -o $@ $^ $(inc)

.PHONY: clean
clean:
	$(hide)$(RM) $(bin)