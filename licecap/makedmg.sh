#!/usr/bin/php

<?

$sign_by = isset($argv[1]) ? $argv[1] : "";
$notarize_email = $sign_by != "" && isset($argv[2]) ? $argv[2] : "";
$src_build_dir = "Release";

function copy_text_replace_line($infn, $outfn, $srctext, $desttext)
{
  $replcnt=0;
  $in=fopen($infn,"r");
  $out = fopen($outfn,"w");
  if (!$in || !$out) die("error opening file(s) for copy_text_replace\n");
  while (($x = fgets($in)))
  { 
    $x=rtrim($x);
    if ($x == $srctext) { $replcnt++; $x=$desttext; }
    fwrite($out,$x . "\n");
  }
  fclose($in);
  fclose($out);
  if ($replcnt != 1) echo "Warning: replaced $replcnt lines in $inffn -> $outfn\n";
}

function regsearch_file($fn, $pattern)
{
  $fp=fopen($fn,"r");
  if (!$fp) { echo "regsearch_file($fn) could not open file.\n";  return false; }

  while (($x = fgets($fp))) if (preg_match($pattern,$x,$regs)) return $regs; 

  fclose($fp);
  return false;
}


$verstr1 = "";
$verstr1_full = "";

$regs = regsearch_file("licecap_version.h",'/^#define LICECAP_VERSION "v(.*)".*$/');
if ($regs) $verstr1 = str_replace(".","",$verstr1_full=$regs[1]);

echo "APP version $verstr1\n";

if ($verstr1 =="") die("invalid version info found\n");

$ver=$verstr1;

$workdir = "build/BUILD";
system("rm -fr $workdir");
system("mkdir $workdir");
system("mkdir $workdir/LICEcap");

system("cp -R 'build/$src_build_dir/LICEcap.app' $workdir/LICEcap/");

copy_text_replace_line("build/$src_build_dir/LICEcap.app/Contents/Info.plist",
		       "$workdir/LICEcap/LICEcap.app/Contents/Info.plist",
			"\t<string>DEVELOPMENT BUILD -- DEVELOPMENT BUILD</string>",
			"\t<string>$verstr1_full</string>");

$fp = fopen("license.txt","r");
if ($fp) 
{
  $fpo = fopen("license-cleaned.txt","w");
  if ($fpo) { 
    while(($x=fgets($fp,4096)))
    {
      $x=rtrim($x);
      $o = "";
      for ($a=0;$a<strlen($x);$a++)
        if (ord($x[$a])<128) $o .= $x[$a];
      fwrite($fpo,$o ."\n");
    }
    fclose($fpo);
  }

  fclose($fp);
}

system("cp whatsnew.txt $workdir/LICEcap");

if ($sign_by != "") 
{
  echo "signing code as $sign_by\n";
  if ($notarize_email != "")
  {
    system("codesign --force --options runtime --timestamp --deep --sign \"$sign_by\" --entitlements ./LICEcap.entitlements $workdir/LICEcap/LICEcap.app",$rv);
    if ($rv) die("error signing\n");
    system("rm -f licecap.zip");
    system("zip -r licecap.zip $workdir/LICEcap/LICEcap.app");
    system("xcrun altool -t osx -f licecap.zip --primary-bundle-id com.cockos.LICEcap --notarize-app --username $notarize_email --password \"@keychain:Developer-altool\"");
    echo "uploading, now spawning a subshell to wait to continue!!!\n";
    echo "uploading, now spawning a subshell to wait to continue!!!\n";
    echo "uploading, now spawning a subshell to wait to continue!!!\n";
    system("bash");
    system("xcrun stapler staple $workdir/LICEcap/LICEcap.app");
  }
  else
    system("codesign -s \"$sign_by\" $workdir/LICEcap/LICEcap.app");
}

system("perl ./pkg-dmg --format UDBZ --target ./build/licecap$ver.dmg --source $workdir/LICEcap --license ./license-cleaned.txt --copy stage_DS_Store:/.DS_Store --symlink /Applications:/Applications --mkdir .background --copy background.png:.background --volname LICECAP_INSTALL --icon licecap.icns --config cmd_SetFile=/Applications/Xcode.app/Contents/Developer/usr/bin/SetFile --config cmd_Rez=/Applications/Xcode.app/Contents/Developer/usr/bin/Rez");



?>
