# $Id: extconf.rb,v 1.3 2008-01-15 12:23:29 hito Exp $
require 'mkmf'

$CFLAGS = "-Wall -O2"

have_header("libpafe/libpafe.h")
have_library("pafe", "pasori_open")
create_makefile("pasori")
