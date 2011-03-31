SHELL = /bin/sh

BINDIR = bin
TESTDIR = test
DEPS = deps
INCLUDES = include ../
SRC = src
vpath %.c $(SRC) $(DEPS)/talloc
vpath %.o $(BINDIR)
vpath %.h $(INCLUDES)

CC=gcc
LD=gcc
GREP=egrep
ETAGS=etags

SOURCES=\
	core.log.c \
	core.string.c \
\
	data.hash.c \
	data.list.c \
	data.list.mixin.c \
	data.map.c \
	data.ringbuf.c \
	data.vector.c \
\
	ev.axis.c \
	ev.button.c \
	ev.channel.c \
	ev.core.c \
	ev.cursor.c \
	ev.dpad.c \
	ev.focus.c \
	ev.keyboard.c \
	ev.quit.c \
	ev.window.c \
\
	gl.array.c \
	gl.attrib.c \
	gl.buf.c \
	gl.context.c \
	gl.display.c \
	gl.index.c \
	gl.shader.c \
\
	in.joystick.c \
\
	job.channel.c \
	job.control.c \
	job.core.c \
	job.histogram.c \
	job.queue.c \
\
	math.matrix.c \
	math.vec.c \
\
	mm.heap.c \
	mm.region.c \
	mm.stack.c \
	mm.tls.c \
\
	parse.core.c \
\
	phys.clock.c \
\
	r.drawable.c \
	r.draw.c \
	r.md5.c \
\
	res.core.c \
	res.md5.c \
	res.spec.c \
\
	r.xform.c \
\
	talloc.c

LIBS=GL GLU GLEW 
TARGETS=flo
TESTS=$(SOURCES:%.c=$(TESTDIR)/%)

# Variants - release, debug, ...
RELEASE_CFLAGS=-O3 -DNDEBUG
DEBUG_CFLAGS=-ggdb -DDEBUG
TRACE_CFLAGS=-DTRACE='".*"'
ifndef VARIANT
	VARIANT:=DEBUG
endif
VARIANT_CFLAGS:=$($(VARIANT:%=%_CFLAGS))

DEFS:=-D_GNU_SOURCE $(DEFS)
WARNINGS:=-Wall $(WARNINGS)
CFLAGS:=-std=c99 \
	$(DEFS) \
	$(WARNINGS) \
	$(VARIANT_CFLAGS) \
	`curl-config --cflags` \
	`$(HOME)/devl/prefix/bin/sdl-config --cflags` \
	$(CFLAGS)
LDFLAGS:=-rdynamic \
	`curl-config --libs` \
	`$(HOME)/devl/prefix/bin/sdl-config --libs` \
	$(LDFLAGS)

DEPS=-Wp,-MD,.deps/$(*F).P
TAGS=TAGS

.PHONY : all tests clean

all: $(BINDIR) $(TARGETS) $(TAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TAGS): $(SOURCES) $(TARGETS:%=%.c)
	@echo '[ETAGS]\tworshipping EMACS, the one true god'; \
	$(ETAGS) $(SRC)/*.c $(INCLUDES)/*.h

-include $(SOURCES:%.c=.deps/%.P) $(TARGETS:%=.deps/%.P)

%.o: %.c
	@echo '[CC]\t$<'; \
	$(CC) $(DEFS) $(INCLUDES:%=-I%) $(DEPS) $(CPPFLAGS) $(CFLAGS) -o $(BINDIR)/$@ -c $<

$(TARGETS): $(SOURCES:%.c=%.o) $(TARGETS:%=%.o)
	@echo '[LD]\t$@'; \
	$(LD) $(LIBS:%=-l%) $(LDFLAGS) -o $@ $(foreach o, $(^F), $(BINDIR)/$(o))

tests: $(TESTDIR) $(TESTS)

$(TESTDIR):
	mkdir -p $(TESTDIR)

# This is a mess, but the basic strategy is this:
#  1. Check if the target's source file has a main function
#  2. If it does we compile the source with the test macro defined linking in all the other objects
$(TESTS): $(SOURCES:%.c=%.o)
	@if test -f $(SRC)/$(@F).c && $(GREP) -q '^[[:space:]]*(int|void)[[:space:]]*main[[:space:]]*\(.*\)' $(SRC)/$(@F).c ;\
	then \
	  echo "[CC/LD]\t$@" ;\
		$(CC) $(DEFS) $(INCLUDES:%=-I%) $(DEPS) $(CPPFLAGS) $(CFLAGS) $(LIBS:%=-l%) $(LDFLAGS) -D__$(subst .,_,$(@F))_TEST__ $(SRC)/$(@F).c  -o $@ $(foreach o, $(subst $(@F).o,, $(^F)), $(BINDIR)/$(o)) ;\
	fi

clean:
	rm -f $(BINDIR)/*.o .deps/*.P $(TARGETS)

###############################################################################
.PHONY: dist

dist:
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	cp $(flo_SOURCES) $(ede_FILES) $(DISTDIR)
	tar -cvzf $(DISTDIR).tar.gz $(DISTDIR)
	rm -rf $(DISTDIR)
