#!/usr/bin/perl

use strict;
use Mysql;

my $dbh;

$dbh = Mysql->connect("localhost", "radio", "nobody", "");

if ($dbh) {
	$dbh->query("delete from server_tbl where UNIX_TIMESTAMP(NOW()) - UNIX_TIMESTAMP(touchtime) > 600");
}
