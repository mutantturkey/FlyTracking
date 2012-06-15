VERSION = 0.0.1

# Customize below to fit your system

# paths
PREFIX = /usr
MANPREFIX = ${PREFIX}/share/man


# includes and libs

GTKINC=$(shell pkg-config --cflags gtk+-2.0 vte )
GTKLIB=$(shell pkg-config --libs gtk+-2.0 vte )

INCS = -I. -I/usr/include $(pkg-config --cflags Magick++ gsl)
LIBS = -L/usr/lib -lc $(pkg-config --libs Magick++ gsl)
# flags
CPPFLAGS = -DVERSION=\"${VERSION}\"
CFLAGS = -mtune=native -std=gnu99 -O2 -s ${INCS} ${CPPFLAGS}
LDFLAGS = -s ${LIBS}

# compiler and linker
CC = g++
