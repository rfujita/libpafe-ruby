%{!?ruby_sitearch: %global ruby_sitearch %(ruby -rrbconfig -e 'puts Config::CONFIG["sitearchdir"]')}

Summary: Ruby bindings for the libpafe
Name: libpafe-ruby
Version: 0.0.7
Release: 0%{?dist}
License: GPLv2
Group: System Environment/Libraries
Source0: http://homepage3.nifty.com/slokar/pasori/libpafe-ruby-0.0.7.tar.gz
URL: http://homepage3.nifty.com/slokar/pasori/libpafe-ruby.html
Requires: ruby
Requires: libpafe
Requires: libusb

BuildRequires: gcc
BuildRequires: make
BuildRequires: ruby
BuildRequires: ruby-devel
BuildRequires: libusb-devel
BuildRequires: libpafe-devel
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Patch0: libpafe-ruby-0.0.7-fix-each-to-foreach.patch

%description
Ruby bindings for the libpafe library

%prep
%setup -q
%patch0 -p1 -b .fix-each-to-foreach

%build
mv ChangeLog ChangeLog.orig
iconv -f euc-jp -t utf-8 ChangeLog.orig > ChangeLog
mv README README.orig
iconv -f euc-jp -t utf-8 README.orig > README

mv sample/suica.rb sample/suica.rb.orig
iconv -f euc-jp -t utf-8 sample/suica.rb.orig > sample/suica.rb

ruby extconf.rb
make 

%install
rm -rf ${RPM_BUILD_ROOT}

%makeinstall
make install DESTDIR=${RPM_BUILD_ROOT} 

%clean
rm -rf ${RPM_BUILD_ROOT}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc README ChangeLog sample/*.rb
%ruby_sitearch/pasori.so

%changelog
* Thu Jul 28 2011 Ryo Fujita <rfujita@redhat.com> - 0.0.7-0
- Initial build for Fedora 15

