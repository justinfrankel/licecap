<?

$fp = fopen("php://stdin","r");
if (!$fp) die("cant open stdin\n");



$menu_symbol="";
$menu_depth=0;
while (($x=fgets($fp)))
{
  $y=trim($x);
  if ($menu_symbol == "")
  {
    $tok = "MENU DISCARDABLE";
    if (substr($y,-strlen($tok)) == $tok)
    {
      $menu_symbol = substr($y,0,-strlen($tok));
      $menu_depth=0;
      echo "SWELL_DEFINE_MENU_RESOURCE_BEGIN($menu_symbol)\n";
    }
  }
  else
  { 
    if ($y == "END") 
    {
      $menu_depth-=1;
      if ($menu_depth == 0)
      {
        echo "SWELL_DEFINE_MENU_RESOURCE_END($menu_symbol)\n\n\n";
      }
      if ($menu_depth < 1) $menu_symbol="";
    }
    if ($menu_depth>0) 
    {
      if (substr($y,-strlen(", HELP")) == ", HELP") 
      {
         $x=substr(rtrim($x),0,-strlen(", HELP")) . "\n";
      }
      echo $x;
    }
    if ($y == "BEGIN") $menu_depth+=1;

  }
}

echo "\n//EOF\n\n";
?>
