# See the file LICENSE for redistribution information.
#
# Copyright (c) 1996, 1997, 1998, 1999, 2000
#	Sleepycat Software.  All rights reserved.
#
#	$Id: test018.tcl,v 1.1.1.2 2002-02-11 16:25:15 ghudson Exp $
#
# DB Test 18 {access method}
# Run duplicates with small page size so that we test off page duplicates.
proc test018 { method {nentries 10000} args} {
	puts "Test018: Off page duplicate tests"
	eval {test011 $method $nentries 19 18 -pagesize 512} $args
}
