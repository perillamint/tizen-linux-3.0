Name: linux
Summary: The Linux Kernel
Version: 3.0.15
Release: 1
License: GPL
Group: System Environment/Kernel
Vendor: The Linux Community
URL: http://www.kernel.org
Source: linux-3.0.15.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{PACKAGE_VERSION}-root
Provides: linux-3.0.15
%define __spec_install_post /usr/lib/rpm/brp-compress || :
%define debug_package %{nil}

BuildRequires:  kernel-headers
BuildRequires:  lzop
BuildRequires:  u-boot-tools
BuildRequires:  binutils-devel
BuildRequires:  module-init-tools elfutils-devel

%description
The Linux Kernel, the operating system core itself

%package -n %{name}-headers
License:        TO_BE_FILLED  
Summary:        Linux support headers for userspace development
Group:          TO_BE_FILLED/TO_BE_FILLED 
  
%description -n %{name}-headers
This package provides userspaces headers from the Linux kernel.  These
headers are used by the installed headers for GNU glibc and other system
 libraries.

%prep
%setup -q

%build
make trats_defconfig
make uImage %{?jobs:-j%jobs}
make modules

%install
mkdir -p %{buildroot}/usr
mkdir -p $RPM_BUILD_ROOT/boot $RPM_BUILD_ROOT/lib/modules
mkdir -p $RPM_BUILD_ROOT/lib/firmware
mkdir -p %{buildroot}/boot
mkdir -p %{buildroot}/lib/modules
make headers_install INSTALL_HDR_PATH=%{buildroot}/usr
make modules_install INSTALL_MOD_PATH=%{buildroot} 
install -m 755 arch/arm/boot/uImage %{buildroot}/boot/

find  %{buildroot}/usr/include -name ".install" | xargs rm -f
find  %{buildroot}/usr/include -name "..install.cmd" | xargs rm -f
rm -rf %{buildroot}/usr/include/scsi
rm -f %{buildroot}/usr/include/asm*/atomic.h
rm -f %{buildroot}/usr/include/asm*/io.h

#INSTALL_MOD_PATH=$RPM_BUILD_ROOT make KBUILD_SRC= modules_install
#install -m 755 arch/arm/boot/uImage %{buildroot}/boot/
#cp System.map $RPM_BUILD_ROOT/boot/System.map-3.0.15
#cp .config $RPM_BUILD_ROOT/boot/config-3.0.15

%clean
rm -rf $RPM_BUILD_ROOT

%files -n %{name}-headers
/usr/include/*

%files
%defattr (-, root, root)
/lib/modules/*
/lib/firmware
/boot/*

