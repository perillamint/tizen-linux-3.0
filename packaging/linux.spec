%define config_name tizen_defconfig
%define abiver 1
%define build_id %{config_name}.%{abiver}
%define defaultDtb exynos4412-trats2.dtb
%define buildarch arm

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

%define fullVersion %{version}-%{build_id}

BuildRequires:  cpio
BuildRequires:  lzma
BuildRequires:  python
BuildRequires:  binutils-devel
BuildRequires:  lzop
BuildRequires:  module-init-tools

%description
The Linux Kernel, the operating system core itself



%package -n kernel-headers
Summary:        Linux support headers for userspace development
Group:          Development/System

%description -n kernel-headers
This package provides userspaces headers from the Linux kernel.  These
headers are used by the installed headers for GNU glibc and other system
 libraries.

%package kernel-devel
Summary:        Prebuild Linux kernel
Group:          Development/System

%description kernel-devel
Prebuild linux kernel

%prep
%setup -q

%build
# 1. Compile sources
make EXTRAVERSION="-%{build_id}" trats2_defconfig
make EXTRAVERSION="-%{build_id}"

# 2. Build modules
make EXTRAVERSION="-%{build_id}" modules %{?_smp_mflags}

# 4. Create tar repo for build directory
tar cpf linux-kernel-build-%{fullVersion}.tar .


%install

QA_SKIP_BUILD_ROOT="DO_NOT_WANT"; export QA_SKIP_BUILD_ROOT

# 1. Destination directories
mkdir -p %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}
mkdir -p %{buildroot}/boot/
mkdir -p %{buildroot}/boot/lib/modules/%{fullVersion}

# 2. Install config, System.map
install -m 644 System.map %{buildroot}/boot/System.map-%{fullVersion}
install -m 644 .config %{buildroot}/boot/config-%{fullVersion}

# 3. Install modules
make -j8 INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=%{buildroot}/boot/ modules_install

# 4. Install kernel headers
make -j8 INSTALL_PATH=%{buildroot} INSTALL_MOD_PATH=%{buildroot} INSTALL_HDR_PATH=%{buildroot}/usr headers_install

# 5. Restore source and build irectory
tar -xf linux-kernel-build-%{fullVersion}.tar -C %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}
ls %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}
ls %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch
ls %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/%{buildarch}
mv %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/%{buildarch} .
mv %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/Kconfig .
rm -rf %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/*
mv %{buildarch} %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/
mv Kconfig      %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/arch/

# 6. Remove files
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name ".tmp_vmlinux*" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name ".gitignore" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name ".*dtb*tmp" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.*tmp" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "vmlinux" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "uImage" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "zImage" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "test-*" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.cmd" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.ko" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.o" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.S" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.s" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -name "*.c" -not -path "%{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/scripts/*" -exec rm -f {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion} -size 0c -exec rm -f {} \;
find %{buildroot}/usr/include -name "\.install"  -exec rm -f {} \;
find %{buildroot}/usr -name "..install.cmd" -exec rm -f {} \;

# 6.1 Clean Documentation directory
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/Documentation -type f ! -name "Makefile" ! -name "*.sh" ! -name "*.pl" -exec rm -f {} \;

rm -rf %{buildroot}/boot/vmlinux*
rm -rf %{buildroot}/System.map*
rm -rf %{buildroot}/vmlinux*

# 7. Update file permisions
%define excluded_files ! -name "*.h" ! -name "*.cocci" ! -name "*.tst" ! -name "*.y" ! -name "*.in" ! -name "*.gperf" ! -name "*.PL" ! -name "lex*" ! -name "check-perf-tracei.pl" ! -name "*.*shipped" ! -name "*asm-generic" ! -name "Makefile*" ! -name "*.lds" ! -name "mkversion" ! -name "zconf.l" ! -name "README" ! -name "*.py" ! -name "gconf.glade" ! -name "*.cc" ! -name "dbus_contexts" ! -name "*.pm" ! -name "*.xs" ! -name "*.l" ! -name "EventClass.py" ! -name "typemap" ! -name "net_dropmonitor.py"

find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/tools/perf/scripts/ -type f %{excluded_files} -exec chmod 755 {} \;
find %{buildroot}/usr/src/linux-kernel-build-%{fullVersion}/scripts/            -type f %{excluded_files} -exec chmod 755 {} \;
find %{buildroot}/usr                                                           -type f ! -name "check-perf-tracei.pl" -name "*.sh" -name "*.pl" -exec chmod 755 {} \;
find %{buildroot}/boot/lib/modules/ -name "*.ko"                                     -type f -exec chmod 755 {} \;

# 8. Create symbolic links
rm -f %{buildroot}/boot/lib/modules/%{fullVersion}/build
rm -f %{buildroot}/boot/lib/modules/%{fullVersion}/source
ln -sf /usr/src/linux-kernel-build-%{fullVersion} %{buildroot}/boot/lib/modules/%{fullVersion}/build


%clean
rm -rf %{buildroot}


%files -n kernel-headers
/usr/include/*

%files kernel-devel
%defattr (-, root, root)
/usr/src/linux-kernel-build-%{fullVersion}
/boot/lib/modules/%{fullVersion}/modules.*
/boot/lib/modules/%{fullVersion}/build

%files
/boot/System.map*
/boot/config*
/boot/lib/modules/%{fullVersion}/kernel
/boot/lib/modules/%{fullVersion}/modules.*
