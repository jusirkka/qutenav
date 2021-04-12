# -*- coding: us-ascii-unix -*-

NAME       = harbour-qopencpn
VERSION    =

DESTDIR    =
PREFIX     = /usr
DATADIR    = $(DESTDIR)$(PREFIX)/share/$(NAME)
DESKTOPDIR = $(DESTDIR)$(PREFIX)/share/applications
ICONDIR    = $(DESTDIR)$(PREFIX)/share/icons/hicolor
BINDIR     = $(DESTDIR)$(PREFIX)/bin
LIBDIR     = $(DESTDIR)$(PREFIX)/lib

ICONS = ../data/qopencpn-86x86.png \
        ../data/qopencpn-108x108.png \
        ../data/qopencpn-128x128.png \
        ../data/qopencpn-172x172.png \
        qml/icons/menu-100.png \
        qml/icons/menu-125.png \
        qml/icons/menu-150.png \
        qml/icons/menu-175.png \
        qml/icons/menu-200.png \
        qml/icons/ship-100.png \
        qml/icons/ship-125.png \
        qml/icons/ship-150.png \
        qml/icons/ship-175.png \
        qml/icons/ship-200.png \
        qml/icons/center-100.png \
        qml/icons/center-125.png \
        qml/icons/center-150.png \
        qml/icons/center-175.png \
        qml/icons/center-200.png \
        qml/icons/record-100.png \
        qml/icons/record-125.png \
        qml/icons/record-150.png \
        qml/icons/record-175.png \
        qml/icons/record-200.png \

install:
	@echo "Installing QML files..."
	mkdir -p $(DATADIR)/qml
	cp qml/qopencpn.qml $(DATADIR)/qml/$(NAME).qml
	cp qml/[A-Z]*.qml $(DATADIR)/qml
	@echo "Installing JS files..."
	cp qml/*.js $(DATADIR)/qml
	cp qml/nmea.log $(DATADIR)/qml
	mkdir -p $(DATADIR)/qml/icons
	cp qml/icons/*.png $(DATADIR)/qml/icons
	@echo "Installing desktop file..."
	mkdir -p $(DESKTOPDIR)
	cp ../data/qopencpn.desktop $(DESKTOPDIR)/$(NAME).desktop
	@echo "Installing icons..."
	mkdir -p $(ICONDIR)/86x86/apps
	mkdir -p $(ICONDIR)/108x108/apps
	mkdir -p $(ICONDIR)/128x128/apps
	mkdir -p $(ICONDIR)/172x172/apps
	cp ../data/qopencpn-86x86.png  $(ICONDIR)/86x86/apps/$(NAME).png
	cp ../data/qopencpn-108x108.png $(ICONDIR)/108x108/apps/$(NAME).png
	cp ../data/qopencpn-128x128.png $(ICONDIR)/128x128/apps/$(NAME).png
	cp ../data/qopencpn-172x172.png $(ICONDIR)/172x172/apps/$(NAME).png
	@echo "Installing binaries..."
	mkdir -p $(BINDIR)
	cp ../$(BUILDDIR)/qopencpn $(BINDIR)/$(NAME)
	cp ../$(BUILDDIR)/qopencpn_dbupdater $(BINDIR)/$(NAME)_dbupdater
	chmod 755 $(BINDIR)/$(NAME)
	chmod 755 $(BINDIR)/$(NAME)_dbupdater
	@echo "Installing oeserverd binaries..."
	mkdir -p $(LIBDIR)
	cp ../data-$(ARCH)/oeserverd $(BINDIR)
	cp ../data-$(ARCH)/libsgl*.so $(LIBDIR)
	@echo "Installing wavefront object files..."
	mkdir -p $(DATADIR)/globe
	cp ../data/sphere.obj $(DATADIR)/globe
	@echo "Installing world map files..."
	cp ../data/negx.jpg $(DATADIR)/globe
	cp ../data/negy.jpg $(DATADIR)/globe
	cp ../data/negz.jpg $(DATADIR)/globe
	cp ../data/posx.jpg $(DATADIR)/globe
	cp ../data/posy.jpg $(DATADIR)/globe
	cp ../data/posz.jpg $(DATADIR)/globe
	@echo "Installing S57 data files..."
	mkdir -p $(DATADIR)/s57data
	cp ../data/s57expectedinput.csv $(DATADIR)/s57data
	cp ../data/chartsymbols.xml $(DATADIR)/s57data
	cp ../data/rastersymbols-dark.png $(DATADIR)/s57data
	cp ../data/rastersymbols-day.png $(DATADIR)/s57data
	cp ../data/rastersymbols-dusk.png $(DATADIR)/s57data
	cp ../data/s57attributes.csv $(DATADIR)/s57data
	cp ../data/s57objectclasses.csv $(DATADIR)/s57data
	@echo "Installing systemd service file..."
	mkdir -p $(LIBDIR)/systemd/user
	cp qopencpn.service $(LIBDIR)/systemd/user/$(NAME).service


%.png:
	svg/svg2png $@

icons: $(ICONS)


.PHONY: install icons

