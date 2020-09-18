%define name fs
%define version 0.20
%define release b1

Name:		%{name}
Summary:	Automatic Filesystem Setup
Version:	%{version}
Release:	%{release}
License:	GPL
Group:		System/Tools
Source:		%{name}-%{version}.tar.bz2
Buildroot:	%{_tmppath}/%{name}-%{version}

#BuildArchitectures: i586

%description
Automatic filesystem setup for Berry Linux.


%prep
%setup -q

%build
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}
mkdir -p $RPM_BUILD_ROOT

make PREFIX=/opt/berry

%install
mkdir -p %{buildroot}/opt/berry
strip fs
install -m 755 fs %{buildroot}/opt/berry

%clean
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}
rm -rf $RPM_BUILD_DIR/%{name}-%{version}

%files
%defattr (-,root,root)
/opt/berry/fs


## change log
%changelog
* Tue Nov 11 2008 Yuichiro Nakada <berry@po.yui.mine.nu>
- Use /sys/block/[hs]d? instead of /proc/partitions
- Use /sys/block/%s/device/media instead of /proc/ide/%s/media
* Wed Nov 14 2007 Yuichiro Nakada <berry@po.yui.mine.nu>
- Use /mnt/sd? default for SATA
* Fri Apr 20 2007 Yuichiro Nakada <berry@po.yui.mine.nu>
- Added ntfs-3g support
* Fri Jul 14 2006 Yuichiro Nakada <berry@po.yui.mine.nu>
- Added -d option for mount dir
* Wed Nov 17 2004 Yuichiro Nakada <berry@po.yui.mine.nu>
- Added noatime for ext3, reiserfs
* Wed Nov 3 2004 Yuichiro Nakada <berry@po.yui.mine.nu>
- Modified for using UTF-8
- Bug fix for extended partition
- Support for reiserfs
- Add -f option
- Support FAT12/16, Hidden FAT12/16/NTFS etc...
* Wed Oct 20 2004 Yuichiro Nakada <berry@po.yui.mine.nu>
- Support Hidden NTFS
- Changed iocharset to nls for NTFS
* Sun Nov 30 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Support FAT12
* Wed Nov 19 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Changed print fomat using no options.
* Mon Nov 10 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Changed compile options
* Mon Aug 18 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Support Hidden FAT32
* Fri Jul 11 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Added -a option
* Sun Jun 1 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Changed ntfs option to use berry users
* Fri May 9 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Changed ntfs option for using Japanese
* Wed Apr 30 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Renamed hd[0-9] to hd[a-z][0-9]
- Bug fixed for extened partition number
* Wed Apr 23 2003 Yuichiro Nakada <berry@po.yui.mine.nu>
- Create for Berry Linux
