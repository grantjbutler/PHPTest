--TEST--
Check cli_process_title support in Windows
--SKIPIF--
<?php
if (strtoupper(substr(PHP_OS, 0, 3)) !== 'WIN')
  die("skip");
?>
--FILE--
<?php

// On Windows 8 and Server 2012, this test does not work the same way. When the PowerShell
// command "get-process" is executed using shell_exec, it overwrites the ConsoleTitle with
// "Windows PowerShell" and this title ONLY clears away when the php.exe process exits
// i.e. the test finishes.
// On older versions like Windows 7 though, running the command appends
// "Windows PowerShell" to the ConsoleTitle temporarily and the title reverts
// back to the original once shell_exec is done.
// Hence on Windows 8, we don't verify that the title is actually set by
// cli_set_process_title(). We're only making the API calls to ensure there are
// no warnings/errors.

$is_windows8 = false;
$ps_output = shell_exec("PowerShell \"(Get-Host).UI.RawUI.WindowTitle\"");
if ($ps_output === null)
{
  echo "Get-Host failed\n";
  die();
}

$ps_output = trim($ps_output);
if (($ps_output == "Windows PowerShell") || ($ps_output == "Administrator: Windows PowerShell"))
  $is_windows8 = true;

echo "*** Testing setting the process title ***\n";

$original_title = uniqid("title", true);
$pid = getmypid();

if (cli_set_process_title($original_title) === true)
  echo "Successfully set title\n";

if ($is_windows8)
{
  $loaded_title = $original_title;
}
else
{
  $loaded_title = shell_exec("PowerShell \"get-process cmd*,powershell* | Select-Object mainWindowTitle | ft -hide\"");

  if ($loaded_title === null)
  {
    echo "Reading title using get-process failed\n";
    die();
  }

  // Kind of convoluted. So when the test is run on Windows 7 or older, the console where
  // the run-tests.php is executed forks a php.exe, which forks a cmd.exe, which then forks
  // a final php.exe to run the actual test. But the console title is set for the original console.
  // I couldn't figure out a good way to navigate this, so we're "grep'ing" all possible
  // console windows for our very unique title. It should occur exactly once in the grep
  // output.
  if (substr_count($loaded_title, $original_title, 0) == 1)
    $loaded_title = $original_title;
}

if ($loaded_title == $original_title)
  echo "Successfully verified title using get-process\n";
else
  echo "Actually loaded from get-process: $loaded_title\n";

$read_title = cli_get_process_title();
if (substr_count($read_title, $original_title, 0) == 1)
  echo "Successfully verified title using get\n";
else
  echo "Actually loaded from get: $read_title\n";

?>
--EXPECTF--
*** Testing setting the process title ***
Successfully set title
Successfully verified title using get-process
Successfully verified title using get