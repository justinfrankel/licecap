#!/usr/bin/php
<?

if (count($argv)<2) die("usage: mac_resgen.php file.rc ...\n");

$lp = dirname(__FILE__);
for ($x = 1; $x < count($argv); $x ++)
{
   $srcfn = $argv[$x];
   if (!stristr($srcfn,".rc")) continue;
   echo "processing " . $srcfn . "\n";
   $ofnmenu = $srcfn . "_mac_menu";
   $ofndlg = $srcfn . "_mac_dlg";
   system("php $lp/rc2cpp_menu.php < \"$srcfn\" > \"$ofnmenu\"");
   system("php $lp/rc2cpp_dlg.php < \"$srcfn\" > \"$ofndlg\"");
}

?>
