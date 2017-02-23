<?php
/*
FTPList/FTPCrawler PHP Web interface
By: Sorcer
License: GPL
*/

/* Get the start time */
$stime = time();
include("inc/conf.inc.php");

/* Create MySQL connection */
$db = mysql_connect($mysq_server, $mysql_user, $mysql_pass);
mysql_select_db($mysql_db, $db);

/* Get some data */
db_query("SELECT crawltime, checktime, lastcrawl, admin, motd, version FROM status");
$crawltime = $column[0];
$checktime = $column[1];
$lastcrawl = $column[2];
$admin     = $column[3];
$motd      = $column[4];
$version   = $column[5];

/* Get next crawl time
function timeofnextcrawl() {
 global $crawltime, $lastcrawl;
 list($hours, $mins) = explode(':', $lastcrawl);
 $seconds += $minutes * 60;
 $scrawltime = $crawltime * 60;
 return date("H:i:s", $mins+$scrawltime);
}
*/

/* Status stuff */
function status($code) {
 global $green, $yellow, $red;

 if ($code == 0) { $status = "Online"; $color = $green; }
 if ($code == 1) { $status = "Full";   $color = $yellow; }
 if ($code == 2) { $status = "Offline";$color = $red; }

 return "<font color=\"". $color ."\">". $status ."</font>";
}

/* MySQL support functions */
function db_query($query) {
 global $db, $column; 
 $conn = mysql_query($query, $db);
 $column = mysql_fetch_array($conn);
 return 0;
}

function db_addone($column) {
  global $db;
  mysql_query("UPDATE `status` SET `$column` = $column+1", $db);
  return 0;
}

/* Action settings and stuff */
$action = $_GET[action];

if (!isset($action)) {
 $page = "Main";
 $action = "main";
} else if ($action == "status") {
 $page = "Status/Info";
} else if ($action == "add") {
 $page = "Add FTP";
} else if ($action == "search") {
 $page = "Search";
}

else {
 $page = "404";
}

?>
<html>
<head>
<title>FTP-List - <?php echo $page;?></title>
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<link rel="stylesheet" href="inc/remedy.css" type="text/css">

</head>
<body bgcolor="#000000" leftmargin="0" marginwidth="0" topmargin="0" marginheight="0">

<table width="100%" border="0" cellpadding="0" cellspacing="0">
   <tr>
      <td width="700" height="47" colspan="3" background="pics/topbg.gif"><img
        src="pics/r01.jpg" width="111" height="47" border="0"><img
        src="pics/e.jpg" width="36" height="47" border="0"><img
        src="pics/m.jpg" width="46" height="47" border="0"><img
        src="pics/e.jpg" width="36" height="47" border="0"><img
        src="pics/d.jpg" width="38" height="47" border="0"><img
        src="pics/y.jpg" width="39" height="47" border="0"></td>
      <td width="100%" height="47" background="pics/topbg.gif"><img src="pics/spacer.gif" width="100%" height="47"></td>
   </tr>
   <tr>
      <td width='700' height='21' background="pics/topbg2.gif" colspan='3' valign=top><img
      src="pics/r02.jpg" width='111' height='21'><img
      src="pics/corner01.jpg" width='34' height='21'><img
      src="pics/spacer.gif" width='152' height='21'></td>
      <td width='100%' height='21' background="pics/topbg2.gif"><img src="pics/spacer.gif" width='100%' height='21'></td>
   </tr>
   <tr>
      <td width="111" height="32"><img src="pics/r03.jpg" width="111" height="32"></td>
      <td width="34" height="32"><img src="pics/corner02.jpg" width="34" height="32"></td>
      <td width="555" height="32"><font size="5">FTP-List - <?php echo $page; ?></font>&nbsp;&nbsp;</td>
      <td width="100%" height="32"><img src="pics/spacer.gif" width="100%" height="32"></td>
   </tr>
   <tr> <!-- left menu goes here --->
      <td width="111" height="20" valign=top>
      <br>
      <a href="index.php">Main page</a><br>
      <a href="index.php?action=search">Search</a><br>
      <a href="index.php?action=add">Add FTP</a><br>
      <a href="index.php?action=status">Status/Info</a><br>
      </td>
      <td width="34" height="400" valign=top><img src="pics/spacer.gif" width="34" height="400"></td>
      <!-- left menu ends here --->

      <td height="400" valign="top" align=left width="100%">
      <!-- content goes here  --->
      <br>
      <table border='0' cellpadding='2' cellspacing='0' width="100%">
         <tr>
           <td colspan='2'>
<?php
if ($action == "main") {
  include("inc/main.inc.php");
} else if ($action == "status") {
  include("inc/status.inc.php");
} else if ($action == "add") {
  include("inc/add.inc.php");
} else if ($action == "search") {
  include("inc/search.inc.php");
}

else {
 echo "Invalid action\n";
}
?>
                         </td>
			</tr>
         <tr></tr>

      </table>
      <!-- content ends here  --->
      </td>
      <td width="100%" height="20">&nbsp;</td>
   </tr>

   <tr><td colspan="2">&nbsp;</td>&nbsp;<p>
   <td align="center">The <a href="http://www.sourceforge.net/projects/ftplist/">FTPList/FTPCrawler project</a> (GPL) &copy;2002 <a href="mailto:sorcer@linux.se">Sorcer</a><br>Site design: &copy;2002 Remedy<br><small>Page was generated in: <b><?php echo time() - $stime;?></b>s.</small>

   </td>

</table>
<br><br><br>
</body>
</html>
<?php
mysql_close($db);
?>
