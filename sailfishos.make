# -*- coding: us-ascii-unix -*-

BASE       = qutenav
NAME       = harbour-$(BASE)
BASEUPD	   = $(BASE)_dbupdater
UPD	   = harbour_$(BASEUPD)
VERSION    =

DESTDIR    =
PREFIX     = /usr
DATADIR    = $(DESTDIR)$(PREFIX)/share/$(NAME)
DESKTOPDIR = $(DESTDIR)$(PREFIX)/share/applications
ICONDIR    = $(DESTDIR)$(PREFIX)/share/icons/hicolor
BINDIR     = $(DESTDIR)$(PREFIX)/bin
LIBDIR     = $(DESTDIR)$(PREFIX)/lib

ICONS = data/$(BASE)-48x48.png \
        data/$(BASE)-64x64.png \
        data/$(BASE)-86x86.png \
        data/$(BASE)-108x108.png \
        data/$(BASE)-128x128.png \
        data/$(BASE)-172x172.png \
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
        qml/icons/route-100.png \
        qml/icons/route-125.png \
        qml/icons/route-150.png \
        qml/icons/route-175.png \
        qml/icons/route-200.png \
        qml/icons/compass-100.png \
        qml/icons/compass-125.png \
        qml/icons/compass-150.png \
        qml/icons/compass-175.png \
        qml/icons/compass-200.png \
        qml/icons/arrow.png \
        qml/icons/finish.png \

install:
	@echo "Installing QML files..."
	mkdir -p $(DATADIR)/qml/platform
	cp qml/$(BASE).qml $(DATADIR)/qml/$(NAME).qml
	cp qml/[A-Z]*.qml $(DATADIR)/qml
	cp qml/platform.silica/[A-Z]*.qml $(DATADIR)/qml/platform
	@echo "Installing JS files..."
	cp qml/*.js $(DATADIR)/qml
	mkdir -p $(DATADIR)/qml/icons
	cp qml/icons/*.png $(DATADIR)/qml/icons
	@echo "Installing desktop file..."
	mkdir -p $(DESKTOPDIR)
	cp data/$(BASE).desktop $(DESKTOPDIR)/$(NAME).desktop
	@echo "Installing icons..."
	mkdir -p $(ICONDIR)/86x86/apps
	mkdir -p $(ICONDIR)/108x108/apps
	mkdir -p $(ICONDIR)/128x128/apps
	mkdir -p $(ICONDIR)/172x172/apps
	cp data/$(BASE)-86x86.png  $(ICONDIR)/86x86/apps/$(NAME).png
	cp data/$(BASE)-108x108.png $(ICONDIR)/108x108/apps/$(NAME).png
	cp data/$(BASE)-128x128.png $(ICONDIR)/128x128/apps/$(NAME).png
	cp data/$(BASE)-172x172.png $(ICONDIR)/172x172/apps/$(NAME).png
	@echo "Installing binaries..."
	mkdir -p $(BINDIR)
	cp $(BUILDDIR)/$(BASE) $(BINDIR)/$(NAME)
	cp $(BUILDDIR)/$(BASEUPD) $(BINDIR)/$(UPD)
	chmod 755 $(BINDIR)/$(NAME)
	chmod 755 $(BINDIR)/$(UPD)
	@echo "Installing oeserverd binaries..."
	mkdir -p $(LIBDIR)
	cp data-$(ARCH)/oeserverd $(BINDIR)
	cp data-$(ARCH)/libsgl*.so $(LIBDIR)
	@echo "Installing wavefront object files..."
	mkdir -p $(DATADIR)/globe
	cp data/sphere.obj $(DATADIR)/globe
	@echo "Installing world map files..."
	cp data/negx.jpg $(DATADIR)/globe
	cp data/negy.jpg $(DATADIR)/globe
	cp data/negz.jpg $(DATADIR)/globe
	cp data/posx.jpg $(DATADIR)/globe
	cp data/posy.jpg $(DATADIR)/globe
	cp data/posz.jpg $(DATADIR)/globe
	@echo "Installing S57 data files..."
	mkdir -p $(DATADIR)/s57data
	cp data/s57expectedinput.csv $(DATADIR)/s57data
	cp data/chartsymbols.xml $(DATADIR)/s57data
	cp data/rastersymbols-dark.png $(DATADIR)/s57data
	cp data/rastersymbols-day.png $(DATADIR)/s57data
	cp data/rastersymbols-dusk.png $(DATADIR)/s57data
	cp data/s57attributes.csv $(DATADIR)/s57data
	cp data/s57objectclasses.csv $(DATADIR)/s57data
	@echo "Installing translations..."
	mkdir -p $(DATADIR)/translations
	cp $(BUILDDIR)/*.qm $(DATADIR)/translations


%.png:
	svg/svg2png $@

icons: $(ICONS)


.PHONY: install icons

