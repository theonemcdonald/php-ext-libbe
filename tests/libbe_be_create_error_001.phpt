--TEST--
Test be_create() with invalid name
--CREDITS--
R. Christian McDonald <cmcdonald@netgate.com>
--EXTENSIONS--
libbe
--SKIPIF--
<?php
$funcs = [
	'libbe_init',
	'be_active_name',
	'be_create',
	'libbe_error_description',
	'be_destroy',
	'libbe_close'
];
require('libbe_check.inc');
?>
--FILE--
<?php
// pretest
$be = libbe_init();
$be_name = be_active_name($be).'libbeexttest&$.';

// test
var_dump(be_create($be, $be_name));
var_dump(libbe_error_description($be));

// posttest
be_destroy($be, $be_name);
libbe_close($be);
?>
===DONE===
--EXPECTF--
int(1)
string(29) "invalid boot environment name"
===DONE===
