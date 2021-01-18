# -*- coding: us-ascii-unix -*-

NAME       = harbour-qopencpn
VERSION    =

DESTDIR    =
PREFIX     = /usr
DATADIR    = $(DESTDIR)$(PREFIX)/share/$(NAME)
DESKTOPDIR = $(DESTDIR)$(PREFIX)/share/applications
ICONDIR    = $(DESTDIR)$(PREFIX)/share/icons/hicolor
BINDIR     = $(DESTDIR)$(PREFIX)/bin

ICONS = ../data/qopencpn-86x86.png \
        ../data/qopencpn-108x108.png \
        ../data/qopencpn-128x128.png \
        ../data/qopencpn-256x256.png \
        qml/icons/menu-100.png \
        qml/icons/menu-125.png \
        qml/icons/menu-150.png \
        qml/icons/menu-175.png \
        qml/icons/menu-200.png \


install:
	@echo "Installing QML files..."
	mkdir -p $(DATADIR)/qml
	cp qml/qopencpn.qml $(DATADIR)/qml/$(NAME).qml
	cp qml/[A-Z]*.qml $(DATADIR)/qml
	mkdir -p $(DATADIR)/qml/icons
	cp qml/icons/*.png $(DATADIR)/qml/icons
	@echo "Installing desktop file..."
	mkdir -p $(DESKTOPDIR)
	cp ../data/qopencpn.desktop $(DESKTOPDIR)/$(NAME).desktop
	@echo "Installing icons..."
	mkdir -p $(ICONDIR)/86x86/apps
	mkdir -p $(ICONDIR)/108x108/apps
	mkdir -p $(ICONDIR)/128x128/apps
	mkdir -p $(ICONDIR)/256x256/apps
	cp ../data/qopencpn-86x86.png  $(ICONDIR)/86x86/apps/$(NAME).png
	cp ../data/qopencpn-108x108.png $(ICONDIR)/108x108/apps/$(NAME).png
	cp ../data/qopencpn-128x128.png $(ICONDIR)/128x128/apps/$(NAME).png
	cp ../data/qopencpn-256x256.png $(ICONDIR)/256x256/apps/$(NAME).png
	@echo "Installing binary..."
	mkdir -p $(BINDIR)
	cp ../$(BUILDDIR)/qopencpn $(BINDIR)/$(NAME)
	chmod 755 $(BINDIR)/$(NAME)
	@echo "Installing wavefront object files..."
	mkdir -p $(DATADIR)/GSHHS/c
	cp ../data/globe_l?.obj $(DATADIR)/GSHHS/c
	@echo "Installing S57 data files..."
	mkdir -p $(DATADIR)/s57data
	cp ../data/attdecode.csv $(DATADIR)/s57data
	cp ../data/chartsymbols.xml $(DATADIR)/s57data
	cp ../data/rastersymbols-dark.png $(DATADIR)/s57data
	cp ../data/rastersymbols-day.png $(DATADIR)/s57data
	cp ../data/rastersymbols-dusk.png $(DATADIR)/s57data
	cp ../data/s57attributes.csv $(DATADIR)/s57data
	cp ../data/s57objectclasses.csv $(DATADIR)/s57data


%.png:
	svg/svg2png $@

icons: $(ICONS)


.PHONY: install icons

