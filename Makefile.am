EXTRA_DIST = CODEGUIDE COMMANDS xvoice.png xvoice.xml xvoice.spec.in vimcmds

SUBDIRS = src

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = xvoice.png

xvoicedir = $(datadir)/xvoice

xvoice_DATA = xvoice.xml vimcmds

modulesdir = $(xvoicedir)/modules

modules_DATA = modules/*.xml

dist-hook:
	cp xvoice.spec $(distdir)
