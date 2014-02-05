--TEST--
IntlCalendar::isWeekend(): bad arguments
--INI--
date.timezone=Atlantic/Azores
--SKIPIF--
<?php
if (!extension_loaded('intl'))
	die('skip intl extension not enabled');
if (version_compare(INTL_ICU_VERSION, '4.4') < 0)
	die('skip for ICU 4.4+');
--FILE--
<?php
ini_set("intl.error_level", E_WARNING);

$c = new IntlGregorianCalendar(NULL, 'pt_PT');

var_dump($c->isWeekend(1, 2));
var_dump($c->isWeekend("jhhk"));

var_dump(intlcal_is_weekend($c, "jj"));
var_dump(intlcal_is_weekend(1));

--EXPECTF--

Warning: IntlCalendar::isWeekend(): intlcal_is_weekend: bad arguments in %s on line %d
bool(false)

Warning: IntlCalendar::isWeekend() expects parameter 1 to be double, string given in %s on line %d

Warning: IntlCalendar::isWeekend(): intlcal_is_weekend: bad arguments in %s on line %d
bool(false)

Warning: intlcal_is_weekend() expects parameter 2 to be double, string given in %s on line %d

Warning: intlcal_is_weekend(): intlcal_is_weekend: bad arguments in %s on line %d
bool(false)

Catchable fatal error: Argument 1 passed to intlcal_is_weekend() must be an instance of IntlCalendar, integer given in %s on line %d
