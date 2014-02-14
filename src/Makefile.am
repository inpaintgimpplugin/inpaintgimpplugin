## Process this file with automake to produce Makefile.in

bin_PROGRAMS = inpaint-BCT

bindir = $(BINDIRREP)

inpaint_BCT_SOURCES = \
	plugin-intl.h	\
	interface.c	\
	interface.h	\
	main.c		\
	main.h		\
	render.cpp	\
	render.h    \
	inpainting_func.cpp \
	inpainting_func.h \
	Heap.cpp \
	Heap.h

AM_CPPFLAGS = \
	-DLOCALEDIR=\""$(LOCALEDIR)"\"		\
	-DDATADIR=\""$(DATADIR)"\"

INCLUDES =\
	-I$(top_srcdir)		\
	@GIMP_CFLAGS@		\
	-I$(includedir)

LDADD = $(GIMP_LIBS)

