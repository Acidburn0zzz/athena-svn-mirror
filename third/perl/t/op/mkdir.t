#!./perl

# $Header: /afs/dev.mit.edu/source/repository/third/perl/t/op/mkdir.t,v 1.1.1.1 1996-10-02 06:40:15 ghudson Exp $

print "1..7\n";

`rm -rf blurfl`;

print (mkdir('blurfl',0777) ? "ok 1\n" : "not ok 1\n");
print (mkdir('blurfl',0777) ? "not ok 2\n" : "ok 2\n");
print ($! =~ /exist/ ? "ok 3\n" : "not ok 3\n");
print (-d 'blurfl' ? "ok 4\n" : "not ok 4\n");
print (rmdir('blurfl') ? "ok 5\n" : "not ok 5\n");
print (rmdir('blurfl') ? "not ok 6\n" : "ok 6\n");
print ($! =~ /such|exist/ ? "ok 7\n" : "not ok 7\n");
