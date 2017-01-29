%define name fs
%define version 0.09
%define release b1

Name:		%{name}
Summary:	Automatic Filesystem Setup
Version:	%{version}
Release:	%{release}
Copyright:	GPL
Group:		System/Tools
Source:		%{name}-%{version}.tar.bz2
Buildroot:	%{_tmppath}/%{name}-%{version}

BuildArchitectures: i586


%description
Automatic filesystem setup for Berry Linux.


##
## Setup Section
##

%prep
%setup -q


##
## Build Section
##

%build
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}
mkdir -p $RPM_BUILD_ROOT

make PREFIX=/opt/berry


##
## Install Section
##

%install
mkdir -p %{buildroot}/opt/berry
strip fs
install -m 755 fs %{buildroot}/opt/berry


##
## Clean Section
##

%clean
[ -n "%{buildroot}" -a "%{buildroot}" != / ] && rm -rf %{buildroot}
rm -rf $RPM_BUILD_DIR/%{name}-%{version}


##
## Files Section
##

%files
%defattr (-,root,root)
/opt/berry/fs


##
## change log
##

%changelog
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
