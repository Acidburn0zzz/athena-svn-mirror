#!./perl

# $Header: /afs/dev.mit.edu/source/repository/third/perl/t/op/glob.t,v 1.1.1.1 1996-10-02 06:40:15 ghudson Exp $

print "1..4\n";

@ops = <op/*>;
$list = join(' ',@ops);

chop($otherway = `echo op/*`);

print $list eq $otherway ? "ok 1\n" : "not ok 1\n$list\n$otherway\n";

print $/ eq "\n" ? "ok 2\n" : "not ok 2\n";

while (<jskdfjskdfj* op/* jskdjfjkosvk*>) {
    $not = "not " unless $_ eq shift @ops;
    $not = "not at all " if $/ eq "\0";
}
print "${not}ok 3\n";

print $/ eq "\n" ? "ok 4\n" : "not ok 4\n";
