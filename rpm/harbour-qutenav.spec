# % global __strip /bin/true

Name: harbour-qutenav

Summary: Chart plotter / navigator for Sailfish OS
Version: 0.10
Release: 1
Group: Qt/Qt
URL: https://github.com/jusirkka/qutenav
License: GPLv3
Source: %{name}-%{version}.tar.xz

Requires: sailfishsilica-qt5
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.3
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5OpenGL)
BuildRequires:  pkgconfig(glm) >= 0.9.9
BuildRequires:  bison >= 3.1
BuildRequires:  flex >= 2.6.1
BuildRequires:  harfbuzz
BuildRequires:  freetype
BuildRequires:  fontconfig


# % global __os_install_post % {nil}


%description
Marine chart plotter / navigator for Sailfish OS. Supports cm93v2, S57 and OpenCPN/SENC vector charts.

%prep
%setup -n %{name}-%{version}


%build
mkdir -p rpmbuilddir-%{_arch}
cd rpmbuilddir-%{_arch} && cmake -DPLATFORM=silica -DCMAKE_BUILD_TYPE=Debug ..
#cd rpmbuilddir-%{_arch} && cmake -DPLATFORM=silica -DCMAKE_BUILD_TYPE=Release ..
cd ..
make %{?_smp_mflags} -C rpmbuilddir-%{_arch} VERBOSE=1

%install
make -f sailfishos.make \
  DESTDIR=%{buildroot} VERSION=%{version} \
  PREFIX=/usr BUILDDIR=rpmbuilddir-%{_arch} ARCH=%{_arch} install

%files
%defattr(-,root,root,-)
%{_libdir}
%{_bindir}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

