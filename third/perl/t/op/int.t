#!./perl

# $Header: /afs/dev.mit.edu/source/repository/third/perl/t/op/int.t,v 1.1.1.1 1996-10-02 06:40:15 ghudson Exp $

print "1..4\n";

# compile time evaluation

if (int(1.234) == 1) {print "ok 1\n";} else {print "not ok 1\n";}

if (int(-1.234) == -1) {print "ok 2\n";} else {print "not ok 2\n";}

# run time evaluation

$x = 1.234;
if (int($x) == 1) {print "ok 3\n";} else {print "not ok 3\n";}
if (int(-$x) == -1) {print "ok 4\n";} else {print "not ok 4\n";}
