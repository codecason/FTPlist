<?php
  echo "<form method=\"POST\">\nSearch for: <input name=\"search\" value=\"\"><p>\n<input type=\"hidden\" name=\"go\" value=\"1\">\n<input type=\"submit\" value=\"Search\"><p>Use * as wildcard. I.e to search for all Dune music, try: Dune*.mp3<p>&nbsp;<p>";

if($_POST[go] == "1") {
  db_addone("searches");
  $search = str_replace("*", "%", $_POST[search]);
  
  $query = "SELECT files.filename,files.path,files.type,ftp.ip,ftp.port,ftp.uname,ftp.pass,ftp.status,ftp.cat FROM `files` INNER JOIN ftp ON ftp.id = files.ftp WHERE `filename` LIKE '%$search%'";
  $conn = mysql_query($query, $db);

  $found = 0;

  while ($columnl = mysql_fetch_array($conn)) {
    ++$found;

    if($columnl[1] == "/") $slash = "";
    else $slash = "/";

    $url = "ftp://$columnl[5]:$columnl[6]@$columnl[3]:$columnl[4]$columnl[1]$slash$columnl[0]";
    $status = status($columnl[7]);
    
    if($columnl[2] == "file") {
      if(stristr($columnl[0], ".avi") != NULL) $img = "video.gif";
      else if(stristr($columnl[0], ".mpeg") != NULL) $img = "video.gif";
      else if(stristr($columnl[0], ".mpg") != NULL) $img = "video.gif";

      else if(stristr($columnl[0], ".mp3") != NULL) $img = "sound.gif";
      else if(stristr($columnl[0], ".wav") != NULL) $img = "sound.gif";
      else if(stristr($columnl[0], ".mid") != NULL) $img = "sound.gif";

      else $img = "file.gif";
    } else if($columnl[2] == "dir") $img = "dir.gif";
    
    echo "<a href=\"$url\"><img src=\"pics/$img\" border=\"0\">&nbsp;$columnl[0]</a>\n<br>FTP Status: $status\n<br>FTP Description: $columnl[8]\n<br><a href=\"ftp://$columnl[5]:$columnl[6]@$columnl[3]:$columnl[4]$columnl[1]\" target=\"_new\">Go to parent directory</a> \n\n<p>";
  }
  
  if($found == 0) echo "<p><b>Sorry, no matches for that query.</b>";
  else echo "<p>Number of files found: <b>$found</b>";
} else {

}
?>
