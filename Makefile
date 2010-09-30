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
	data.list.c \
	data.list.mixin.c \
	data.hash.c \
	data.map.c \
	data.ringbuf.c \
	data.vector.c \
\
	display.core.c \
\
	ev.core.c \
	ev.channel.c \
	ev.axis.c \
	ev.button.c \
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
	gl.index.c \
	gl.shader.c \
\
	in.joystick.c \
\
	job.core.c \
	job.channel.c \
	job.control.c \
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
	r.md5.c \
\
	res.core.c \
	res.md5.c \
	res.spec.c \
\
	r.xform.c \
\
	seq.core.c \
	seq.array.c \
	seq.file.c \
	seq.filter.c \
	seq.map.c \
\
	talloc.c

LIBS=SDL GL GLU GLEW 
TARGETS=flo
TESTS=$(SOURCES:%.c=$(TESTDIR)/%)

# Variants - release, debug, ...
RELEASE_CFLAGS=-O3 -DNDEBUG
DEBUG_CFLAGS=-ggdb -DDEBUG
ifndef VARIANT
	VARIANT:=DEBUG
endif
VARIANT_CFLAGS:= $($(VARIANT:%=%_CFLAGS))

CFLAGS:=-std=c99 -D_GNU_SOURCE -Wall $(VARIANT_CFLAGS) `curl-config --cflags` $(CFLAGS)
LDFLAGS:=-rdynamic `curl-config --libs`
LD_LINK=$(LD) $(LDFLAGS) -L. -o $@

DEPS=-Wp,-MD,.deps/$(*F).P
TAGS=TAGS

.PHONY : all tests clean

all: $(BINDIR) $(TARGETS) $(TAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TAGS): $(SOURCES:%.c=%.o) $(TARGETS:%=%.o)
	@echo '[ETAGS]\tfeed EMACS, the one true god'; \
	$(ETAGS) --recurse

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
