include config.mk

SRC = FlyObject.cpp FlyTrackingFilter.cpp FrameInfo.cpp FlyTrackingMain.cpp
OBJ = ${SRC:.c=.o}

all: options FilterFlyMask FlyTracking

options:
	@echo fly-tools build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"
	@echo "SRC 			= ${SRC}"
.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.mk

svte: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ svte.o ${LDFLAGS}

FlyTracking: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ FlyTracking.o ${LDFLAGS}

clean:
	@echo cleaning
	@rm -f FlyTracking FilterFlyMask ${OBJ} fly-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p fly-${VERSION}
	@cp -R Makefile config.mk ${SRC} svte-${VERSION}
	@tar -cf svte-${VERSION}.tar mt-${VERSION}
	@gzip svte-${VERSION}.tar
	@rm -rf svte-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f svte ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/svte

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/svte


.PHONY: all options clean dist install uninstall
