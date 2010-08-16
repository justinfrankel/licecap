<?

$fp = fopen("php://stdin","r");
if (!$fp) die("cant open stdin\n");

echo '#ifndef SWELL_DLG_SCALE_AUTOGEN' . "\n";
echo '#define SWELL_DLG_SCALE_AUTOGEN 1.7' . "\n";
echo '#endif' . "\n";
echo '#ifndef SWELL_DLG_FLAGS_AUTOGEN' . "\n";
echo '#define SWELL_DLG_FLAGS_AUTOGEN SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_NOAUTOSIZE' . "\n";
echo "#endif\n";
echo "\n";


$dlg_state=0; // 1 = before BEGIN, 2=after BEGIN


$dlg_name="";
$dlg_size_w=0;
$dlg_size_h=0;
$dlg_title = "";
$dlg_styles = "SWELL_DLG_FLAGS_AUTOGEN";
$dlg_contents="";

$next_line="";
for (;;) 
{
  if ($next_line != "") { $x=$next_line; $next_line =""; }
  else if (!($x=fgets($fp))) break;

  $y=trim($x);
  if ($dlg_state>=2) 
  {
     $dlg_contents .= $y . "\n";
     if ($y == "END")
     {
       if ($dlg_state==2) $dlg_styles.="|SWELL_DLG_WS_OPAQUE";
       echo "#ifndef SET_$dlg_name" . "_SCALE\n";
       echo "#define SET_$dlg_name" . "_SCALE SWELL_DLG_SCALE_AUTOGEN\n";
       echo "#endif\n";
       echo "#ifndef SET_$dlg_name" . "_STYLE\n";
       echo "#define SET_$dlg_name" . "_STYLE $dlg_styles\n";
       echo "#endif\n";
       echo "SWELL_DEFINE_DIALOG_RESOURCE_BEGIN($dlg_name,SET_$dlg_name" . "_STYLE,\"$dlg_title\",$dlg_size_w,$dlg_size_h,SET_$dlg_name" . "_SCALE)\n";
       $dlg_contents=str_replace("NOT WS_VISIBLE","SWELL_NOT_WS_VISIBLE",$dlg_contents);
       $dlg_contents=str_replace("NOT\nWS_VISIBLE","SWELL_NOT_WS_VISIBLE",$dlg_contents);
       $dlg_contents=str_replace("NOT \nWS_VISIBLE","SWELL_NOT_WS_VISIBLE",$dlg_contents);
       echo $dlg_contents;
       echo "SWELL_DEFINE_DIALOG_RESOURCE_END($dlg_name)\n\n\n";
       $dlg_state=0;
     }
     else if (strlen($y)>1) $dlg_state=3;
  }
  else 
  {
    $parms = explode(" ", $y);
    if (count($parms) > 0)
    {
      if ($dlg_state == 0)
      {
       // if (substr($parms[0],0,8) == "IDD_PREF") 
	if (count($parms)>4 && ($parms[1] == 'DIALOGEX'||$parms[1] == 'DIALOG'))
        {
          $dlg_name=$parms[0];
          $rdidx = 2;
          if ($parms[$rdidx] == 'DISCARDABLE') $rdidx++;
          while ($parms[$rdidx] == "" && $rdidx < count($parms)) $rdidx++;
          $rdidx  += 2;
          $dlg_size_w = (int)$parms[$rdidx++];
          $dlg_size_h = (int)$parms[$rdidx++];
          if (count($parms) >= $rdidx && $dlg_size_w >0 && $dlg_size_h >0)
          {
            $dlg_title=""; 
            $dlg_styles="SWELL_DLG_FLAGS_AUTOGEN"; 
            $dlg_contents="";
            $dlg_state=1;
          }
          else echo "//WARNING: corrupted $dlg_name resource\n";
        }
      }
      else if ($dlg_state == 1)
      {
        if ($parms[0] == "BEGIN")
        {
          $dlg_state=2;
          $dlg_contents = $y ."\n";
        }
	else
        {
          if ($parms[0] == "CAPTION") 
          {
            $dlg_title = str_replace("\"","",trim(substr($y,8)));
          }
          else if ($parms[0] == "STYLE" || $parms[0] == "EXSTYLE")
          { 
             $rep=0;
             for (;;) 
             {
               $next_line = fgets($fp,4096);
               if (!($next_line )) { $next_line=""; break; }
               if (substr($next_line,0,1)==" " || substr($next_line,0,1)=="\t")
               {
                 $y .= " " . trim($next_line);
                 $rep++;
                 $next_line="";
               }
               else break;
             }
	     if ($rep) $parms = explode(" ", $y);
             $opmode=0;
             $rdidx=1;
             while ($rdidx < count($parms))
             {
               if ($parms[$rdidx] == '|') { $opmode=0; }
               else if ($parms[$rdidx] == 'NOT') { $opmode=1; }
               else if ($parms[$rdidx] == 'WS_CHILD') 
               {
                 if (!$opmode) $dlg_styles .= "|SWELL_DLG_WS_CHILD";
               }
               else if ($parms[$rdidx] == 'WS_THICKFRAME') 
               {
                 if (!$opmode) $dlg_styles .= "|SWELL_DLG_WS_RESIZABLE";
               }
               else if ($parms[$rdidx] == 'WS_EX_ACCEPTFILES') 
               {
                 if (!$opmode) $dlg_styles .= "|SWELL_DLG_WS_DROPTARGET";
               }
               else $opmode=0;
               $rdidx++;
             }
          }
        }
      } 
    }
  }
}
if ($dlg_state != 0)
  echo "// WARNING: there may have been a truncated  dialog resource ($dlg_name)\n";

echo "\n//EOF\n\n";

?>
