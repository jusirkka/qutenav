# QuteNav

Marine chart plotter / navigator for Sailfish OS and Linux which draws it's inspiration
from [OpenCPN](https://opencpn.org) and [Navionics Boating](https://www.navionics.com/apps/navionics-boating ). QuteNav is not feature complete yet, see TODO file for missing but planned features.
Only offline vector charts are supported, namely
- CM93v2 charts
- S57 charts
- OpenCPN/SENC charts
- OpenCPN/SENC encrypted charts (oesenc)

For sources of these charts, see the [OpenCPN chart source page](https://opencpn.org/OpenCPN/info/chartsource.html)

QuteNav is released under the GNU General Public
License (GPL) version 3. QuteNav also contains source code from other projects
- Geodesic line code from [GeographicLib](https://sourceforge.net/projects/geographiclib/) (C)
Charles Karney and licensed under the MIT/X11 License.
- 2D Line string code from [GEOS](https://trac.osgeo.org/geos) (C) GEOS contributors and licenced under GPL.
- [Earcut](https://github.com/mapbox/earcut.hpp) header-only triangulator (C) Mapbox, ISC License.
- CM93v2 and O(E)SENC readers are based on work done in OpenCPN project (C) David S. Register, GPL v2.
- The binaries oeserverd and libsgl*.so from [oesenc_pi OpenCPN plugin](https://github.com/bdbcat/oesenc_pi/)
needed to decrypt oesenc files are closed source, but re-distribution is allowed (C) David S. Register.
- The design to organize platform dependencies of the qml code was copied from [pure-maps](https://github.com/rinigus/pure-maps)
- The QuteNav logo is from [svgrepo.com](https://www.svgrepo.com/svg/233528/buoy), CC0 Licence.

## Build dependencies

- qtcore development files
- qtdeclarative development files
- qtsql development files
- qtopengl development files
- harfbuzz development files
- freetype development files
- fontconfig development files
- glm >= 0.9.9
- bison >= 3.1
- flex >= 2.6.1
- inkscape to convert svg files to png images

Minimum supported Qt version is 5.6

## Runtime dependencies

- qtcore
- qtdeclarative
- qtpositioning
- qtsql with sqlite support
- qtopengl
- harfbuzz
- freetype
- fontconfig
- dbus

## Building and installing SailfishOS app

### Preparations

1. Install the [SDK](https://sailfishos.org/wiki/Application_SDK_Installation).
2. `git clone git@github.com:jusirkka/qutenav.git`
3. `cd qutenav`
4. `make -f sailfishos.make icons` - you need inkscape for this

### Install build dependencies

- `vboxmanage startvm --type headless "Sailfish OS Build Engine"`
- Copy the glm package to the build engine: `scp -P 2222 -i $SDK/vmshare/ssh/private_keys/engine/mersdk data-arm/glm-0.9.9.8-1.armv7hl.rpm mersdk@localhost:`, where $SDK is the SDK installation path.
- Login to the build engine: `ssh -p 2222 -i $SDK/vmshare/ssh/private_keys/engine/mersdk mersdk@localhost`
- `sb2 -t SailfishOS-$VERSION-armv7hl -m sdk-install -R zypper in flex bison harfbuzz-devel`, where
$VERSION is the sfos version.
- `sb2 -t SailfishOS-$VERSION-armv7hl -m sdk-install -R rpm -U glm-0.9.9.8-1.armv7hl.rpm`
- `exit` from the build engine.

### Build

- For convenience, create an alias for the sfdk tool: `alias sfdk=$SDK/bin/sfdk`
- `sfdk config target="SailfishOS-$VERSION-armv7hl"`
- `sfdk build`

### Install

The built rpm package is in RPMS directory. Assuming you have enabled developer mode in your device, you can install the package by issuing the commands
- `scp RPMS/harbour-qutenav-*.rpm  nemo@$HOST:`, where $HOST is the IP address of your device
- `ssh -t nemo@$HOST "devel-su rpm -U harbour-qutenav-*.rpm"`

### Uninstall

Assuming you have enabled developer mode in your device, you can uninstall the package by issuing
- `ssh -t nemo@$HOST "devel-su rpm -e \$(rpm -qa|grep qutenav)"`

## Building and installing Linux Laptop/Desktop app

- `git clone git@github.com:jusirkka/qutenav.git`
- `cd qutenav`
- `make -f sailfishos.make icons` - you need inkscape for this
- `mkdir build`
- `cd build`
- `cmake -DPLATFORM=qtcontrols -DCMAKE_BUILD_TYPE=Release ..`
- `make -j4`
- `sudo make install`
- optional: if you intend to use oesenc charts, install opencpn-plugin-oesenc or just copy the binaries oeserved and libsgllnx64-$(VERSION).so to `/usr/bin/oeserverd` and `/usr/lib64/libsgllnx64-$(VERSION).so`
