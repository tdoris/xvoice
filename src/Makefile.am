SUBDIRS= expat

bin_PROGRAMS = xvoice wmvoice

COMMON_FILES = keys.h EventStream.cc EventStream.h \
	mouseGrid.cc mouseGrid.h MainWin.h \
	ParseEventStream.cc ParseEventStream.h \
	Error.cc Error.h \
	Target.cc Target.h EventRecord.h \
	Voice.cc Voice.h \
	xvoice.cc xvoice.h\
	App.cc App.h x.h \
	WordInfo.h WordInfo.cc \
	WordInfoQueue.h WordInfoQueue.cc \
	Corrector.h CorrectionUI.h

XVOICE_FILES = gnomeMainWin.cc
xvoice_SOURCES = $(COMMON_FILES) $(XVOICE_FILES)
xvoice_DEPENDENCIES = expat/libexpat.a

WMVOICE_FILES = wmgeneral.cc wmgeneral.h \
	wmMainWin.cc

wmvoice_SOURCES = $(COMMON_FILES) $(WMVOICE_FILES)
wmvoice_DEPENDENCIES = expat/libexpat.a

EXTRA_DIST = wmvoice-master.xpm

AM_CFLAGS          = $(XV_CFLAGS) -DDATADIR=$(datadir) -DUSE_SHELL=\"@USE_SHELL@\"
AM_CXXFLAGS        = $(XV_CXXFLAGS) -DDATADIR=\"$(datadir)\" -DUSE_SHELL=\"@USE_SHELL@\"

INCLUDES        = $(XV_GNOME_CFLAGS) -I$(XV_VV)/include

BASE_LIBS = -lXtst -L./expat -lexpat

if HAVE_SMAPI
xvoice_LDADD    = $(BASE_LIBS) $(XV_VV_LIBS) -lsmapi -lvtbnfc31 \
                    $(XV_GNOME_LIBS)
else
xvoice_LDADD    = $(BASE_LIBS) $(XV_GNOME_LIBS)
endif

wmvoice_LDADD = -L/usr/X11R6/lib -lX11 -lXpm $(BASE_LIBS) $(XV_VV_LIBS) \
        -lsmapi -lvtbnfc31 -lpopt -lglib -lgtk -lgdk -lgnomeui -lgnome 	-lgnomesupport -lart_lgpl

libxvesd_la_SOURCES = xvesd.c
libxvesd_la_LDFLAGS = -lesd -version-info 0:0:0

libxvarts_la_SOURCES = xvarts.c
libxvarts_la_LDFLAGS = -lartsc -version-info 0:0:0

if HAVE_ESD
ESDLIB = libxvesd.la
endif

if HAVE_ARTS
ARTSLIB = libxvarts.la
endif

lib_LTLIBRARIES = $(ESDLIB) $(ARTSLIB)

# IBM's install wildly breaks the file system standard.
# We install to a standard directory, and then symlink it
# here so the voice engine can find it.

install-exec-local:
if HAVE_ESD
	install -d $(DESTDIR)$(XV_VV)/bin
	ln -f -s $(libdir)/libxvesd.so $(DESTDIR)$(XV_VV)/bin
endif
if HAVE_ARTS
	install -d $(DESTDIR)$(XV_VV)/bin
	ln -f -s $(libdir)/libxvarts.so $(DESTDIR)$(XV_VV)/bin
endif
if VVINSTALLED_SYSTEM
	mv $(DESTDIR)$(bindir)/xvoice $(DESTDIR)$(bindir)/xvoice.bin
	install -m 755 xvoice.sh $(DESTDIR)$(bindir)/xvoice
endif

uninstall-local:
if HAVE_ESD
	rm -f $(DESTDIR)$(XV_VV)/bin/libxvesd.so
endif
if HAVE_ARTS
	rm -f $(DESTDIR)$(XV_VV)/bin/libxvarts.so
endif
if VVINSTALLED_SYSTEM
	rm -f $(DESTDIR)$(bindir)/bin/xvoice.bin
endif

