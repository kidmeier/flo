SHELL = /bin/sh

BINDIR = bin
TESTDIR = test
INCLUDES = include
SRC = src
vpath %.c $(SRC)
vpath %.o $(BINDIR)
vpath %.h $(INCLUDES)

CC=gcc
LD=gcc
GREP=egrep
ETAGS=etags

SOURCES=\
	control.predicate.c \
\
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
	g.aabb.c \
\
	gl.array.c \
	gl.attrib.c \
	gl.buf.c \
	gl.context.c \
	gl.context.headless.c \
	gl.display.c \
	gl.index.c \
	gl.shader.c \
	gl.types.c \
	gl.util.c \
\
	in.joystick.c \
	in.mouse.c \
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
\
	parse.core.c \
\
	phys.clock.c \
\
	r.drawable.c \
	r.draw.c \
	r.frame.c \
	r.mesh.c \
	r.scene.c \
	r.skel.c \
	r.state.c \
	r.target.c \
	r.view.c \
	r.xform.c \
\
	res.core.c \
	res.md5.c \
	res.obj.c \
	res.spec.c \
\
	sys.fs.c

ifneq ($(strip $(TERM)),)

	LIBS=dl m pthread rt GL GLU GLEW
	LDFLAGS:=-rdynamic \
		$(LDFLAGS)

else 

	LIBS=pthreadGC2 opengl32 glu32 glew32

endif

PKG_LIBS=\
	`sdl2-config --libs`
PKG_CFLAGS=\
	`sdl2-config --cflags`
TARGETS=flo res.import
TESTS=$(SOURCES:%.c=$(TESTDIR)/%)

# Variants - release, debug, ...
RELEASE_CFLAGS=-O3 -DNDEBUG
DEBUG_CFLAGS=-ggdb -DDEBUG
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
	$(CFLAGS)

DEPS=-Wp,-MD,.deps/$(*F).P
TAGS=TAGS

.PHONY : all tests clean

all: $(BINDIR) $(TARGETS) $(TAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TAGS): $(SOURCES) $(TARGETS:%=%.c)
	@echo '[ETAGS]  worshipping EMACS, the one true god'; \
	$(ETAGS) $(SRC:%=%/*.c) $(INCLUDES:%=%/*.h)

-include $(SOURCES:%.c=.deps/%.P) $(TARGETS:%=.deps/%.P)

%.o: %.c
	@echo '[CC]  $<'; \
	$(CC) $(DEFS) $(INCLUDES:%=-I%) $(DEPS) $(CPPFLAGS) $(CFLAGS) $(PKG_CFLAGS) -o $(BINDIR)/$@ -c $<

$(TARGETS): $(SOURCES:%.c=%.o) $(TARGETS:%=%.o)
	@echo '[LD]  $@'; \
	$(LD) $(LDFLAGS) -o $@ $(filter-out $(foreach tgt, $(subst $@,, $(TARGETS)), $(BINDIR)/$(tgt).o), $(foreach o, $(^F), $(BINDIR)/$(o))) $(PKG_LIBS) $(LIBS:%=-l%)

tests: $(TESTDIR) $(TESTS)

$(TESTDIR):
	mkdir -p $(TESTDIR)

# This is a mess, but the basic strategy is this:
#  1. Check if the target's source file has a main function
#  2. If it does we compile the source with the test macro defined linking in all the other objects
$(TESTS): $(SOURCES:%.c=%.o)
	@if test -f $(SRC)/$(@F).c && $(GREP) -q '^[[:space:]]*(int|void)[[:space:]]*main[[:space:]]*\(.*\)' $(SRC)/$(@F).c ;\
	then \
	  echo "[CC/LD]  $@" ;\
		$(CC) $(DEFS) $(INCLUDES:%=-I%) $(DEPS) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -D__$(subst .,_,$(@F))_TEST__ $(SRC)/$(@F).c  -o $@ $(foreach o, $(subst $(@F).o,, $(^F)), $(BINDIR)/$(o)) $(PKG_LIBS) $(LIBS:%=-l%) ;\
	fi

clean:
	rm -f $(BINDIR)/*.o .deps/*.P $(TARGETS)
