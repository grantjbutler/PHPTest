--TEST--
mysql connect
--SKIPIF--
<?php
require_once('skipif.inc');
require_once('skipifconnectfailure.inc');
?>
--FILE--
<?php
require_once('connect.inc');
$test = '';

if ($socket)
	$host = sprintf("%s:%s", $host, $socket);
else if ($port)
	$host = sprintf("%s:%s", $host, $port);

/*** test mysql_connect localhost ***/
$db = mysql_connect($host, $user, $passwd);
$test .= ($db) ? '1' : '0';
mysql_close($db);

/*** test mysql_connect localhost:port ***/
$db = mysql_connect($host, $user, $passwd, true);
$test .= ($db) ? '1' : '0';
mysql_close($db);

var_dump($test);
print "done!";
?>
--EXPECTF--
Deprecated: mysql_connect(): The mysql extension is deprecated and will be removed in the future: use mysqli or PDO instead in %s on line %d

Deprecated: mysql_connect(): The mysql extension is deprecated and will be removed in the future: use mysqli or PDO instead in %s on line %d
%unicode|string%(2) "11"
done!
