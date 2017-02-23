<?PHP

function my_var_dump($var) {
  print "<PRE>";var_dump($var);print "</PRE>";
}

function db_connect() {
  static $link=FALSE;
  if($link) { return $link; }
  $link = mysql_pconnect("172.20.48.29", "root", "relisys");
  mysql_select_db("remedyftp");
  return $link;
}

function db_get_ftps() {
  $SQL = "SELECT * FROM ftp";
  return db_get_rows_by_field(&$SQL,"ftp_id");
}

function db_get_rows_by_field($SQL,$field) {
  $rows = array();
  $result = db_query(&$SQL);
  while($row = mysql_fetch_assoc($result)) {
    $rows[$field] = $row;
  }
  return $rows;
}

function db_get_rows($SQL) {
  $rows = array();
  $result = db_query(&$SQL);
  while($row = mysql_fetch_assoc($result)) {
    $rows[] = $row;
  }
  return $rows;
}

function db_query($SQL) {
  db_connect();
  $result = mysql_query(&$SQL);
  if(!$result) trigger_error("Error in SQL query '$SQL':<br>".mysql_error(),E_USER_ERROR);
  return $result;
}

function db_list_files() {
  db_connect();
  $SQL="SELECT ftp_files.*,ftp.*\n"
    ."FROM ftp_files\n"
    ."LEFT JOIN ftp ON ftp_files.ftp_id=ftp.ftp_id";
  return db_get_rows(&$SQL);
}

function db_num_files() {
  db_connect();
  $SQL="SELECT COUNT(*) FROM ftp_files";
  return current(db_get_row(&$SQL));
}

function db_get_row($SQL) {
  db_connect();
  $result = db_query(&$SQL);
  $return = mysql_fetch_assoc($result);
  mysql_free_result($result);
  return $return;
}
?>
<HTML>
<HEAD>
   <meta http-equiv="Content-Language" content="sv">
   <title>(Remedy 2001) - Choose your destination:</title>
</HEAD>
<body bgcolor="#4E59B3" text="#FAF83F" link="#FAF83F">

<CENTER>
<br>
<br>
<img src="remedy.jpg" alt="Remedy 2001" border="0" width="420" height="62"><br>
<br>
<font size=+5>

<b>Remedy FTP list</b></font></center>
<p align="left">
<font size=4>
<?php
$fn = "counter.txt";
$fp = fopen($fn, "r");
$cnt = fread($fp, filesize($fn));
fclose($fp);
++$cnt;
$fp = fopen($fn, "w");
fwrite($fp, $cnt);
fclose($fp);

$db = mysql_connect("172.20.48.29", "root", "relisys");
mysql_select_db("remedyftp", $db);

if ($sub == "Submit") {
if ($ip == NULL || $port == NULL || $uname == NULL || $pass == NULL || $cat == NULL || $comments == NULL) {
echo "Some fields are missing, plz go back and correct them<br><br>";
} else {
 mysql_query("INSERT INTO ftp (ip,port,uname,pass,cat,comments) VALUES ('$ip','$port','$uname','$pass','$cat','$comments')", $db); 
?>

The FTP has been added sucessfully!<br>

<?php
}

} else if ($src != "") {
foreach($HTTP_GET_VARS as $key => $val) {
  if(empty($val)) unset($HTTP_GET_VARS[$key]);
}
if(count($HTTP_GET_VARS) > 0) {
  $and = FALSE;
  $SQL="SELECT ftp_files.*,ftp.id as id,ftp.uname as user,ftp.*\n"
    ."FROM ftp_files\n"
    ."LEFT JOIN ftp ON ftp_files.ftp_id=ftp.id\n"
    ."WHERE\n";

  if(!empty($file)) {
    $and=TRUE;
    $file = addslashes(str_replace(" ","%",$file));
    $SQL .= "(file LIKE '%$file%'";
    if(isset($searchdirs)) {
      $SQL .= " OR path LIKE '%$file%'";
    }
    $SQL .= ")\n";
  }

  if(!empty($suffix)) {
    if($and) $SQL .= "AND ";
    $and = TRUE;
    $suffix = addslashes($suffix);
    $SQL .= "file LIKE '%.$suffix'";
  }
#  my_var_dump($SQL);
  $rows = db_get_rows(&$SQL);
?>
<P>Hittade <?=count($rows)?> träffar.</P>
<pre>
<?PHP
foreach($rows as $file):
?>
<A HREF="ftp://<?=$file["user"]?>:<?=urlencode($file["pass"])?>@<?=$file["ip"]?><?=$file["path"]?>/<?=$file["file"]?>">ftp://<?=$file["user"]?>:<?=urlencode($file["pass"])?>@<?=$file["ip"]?><?=$file["path"]?>/<?=$file["file"]?></A>
<?PHP
endforeach;
?>
   </pre>

<?PHP
}
} else {
?>

Please add your ftp to the list below.<br>
<b>MOTH: </b>Thanx for a great LAN party, hop you will come to DH</font></p> 
<form action="index.php" method="get">
 <p align="left">
  &nbsp;</p>
<center>Stats for anonymous FTP @ 172.20.3.11<br><?php include("bwbar-1.1/ubar.txt"); ?><br><img 
src="bwbar-1.1/ubar.png"><br><br> </center><b>Sök i FTPerna:</b><br><br>
Tips: sök på <A HREF="index.php?suffix=avi">avi</A> som filsuffix om 
du vill 
ha film, <A HREF="index.php?suffix=mp3">mp3</A> om du vill ha 
musik och så.
<P>

Det finns <B><?= db_num_files(); ?></B> filer i databasen.


<table border="0"><tr>
<td>Filnamn</td><td><INPUT TYPE="text" NAME="file" width="50%" value="<?=$file?>"></td>
</tr><tr><td colspan="2"><i>Sökning sker i den ordning du skrivit in nyckelord.<br>
Så om du skrivit in "tomb raider" så kommer den att matcha 
*tomb*raider*<br> 
alltså alla filer(&kataloger om du valt det) som inehåller tomb och sedan raider.</i></td></tr><tr>
</tr><tr>
<td colspan="2"><INPUT TYPE="checkbox" NAME="searchdirs"> Inkludera katalognamn i sökning
</tr><tr>
<td>Filtyp</td><td><INPUT TYPE="text" NAME="suffix"></td>
</tr><tr><td colspan="2"><i>tex: mp3, avi, mpg m.m.</i></td></tr><tr>
</tr><tr>
<td colspan="2"><input type="submit" VALUE="Skicka sökning"
name="src"></td></tr> </table>

</form>



<br><br><b>Lägg till en FTP:</b><br>
<form method="post" action="index.php">
<table border="0" cellpadding="0" cellspacing="0" style="border-collapse:
collapse" bordercolor="#111111" width="100%" id="AutoNumber1">   <tr>
    <td width="14%">Hostname or IP:</td>
    <td width="86%"><input type="text" name="ip" size="20"></td>
  </tr>
  <tr>
    <td width="14%">Port:</td>
    <td width="86%"><input type="text" name="port" size="20"></td>
  </tr>
  <tr>
    <td width="14%">Username:</td>
    <td width="86%"><input type="text" name="uname" size="20"></td>
  </tr>
  <tr>
    <td width="14%">Password:</td>
    <td width="86%"><input type="password" name="pass" size="20"></td>
  </tr>
  <tr>
    <td width="14%">Category:</td>
    <td width="86%"><input type="text" name="cat" size="20"></td>
  </tr>
  <tr>
    <td width="14%">Comments:</td>
    <td width="86%"><input type="text" name="comments" size="20"></td>
  </tr>
    <tr>
    <td width="14%">
  <input type="submit" value="Submit" name="sub"><input type="reset" value="Reset" name="res"></td>
    <td width="86%">&nbsp;</td>
  </tr>
</table>
</form>
<p align="left">
<font size=4>

&nbsp;</font><font size=+5><b><br></b>
</font>
</strong>
</p>
<table border="1" cellpadding="0" cellspacing="0" style="border-collapse: collapse" bordercolor="#111111" width="100%" id="AutoNumber2">
  <tr>
    <td width="16%" bgcolor="#C0C0C0"><font color="#000000">Hostname or IP:</font></td>
    <td width="16%" bgcolor="#C0C0C0"><font color="#000000">Port:</font></td>
    <td width="17%" bgcolor="#C0C0C0"><font color="#000000">Username:</font></td>
    <td width="17%" bgcolor="#C0C0C0"><font color="#000000">Password:</font></td>
    <td width="17%" bgcolor="#C0C0C0"><font color="#000000">Category:</font></td>
    <td width="17%" bgcolor="#C0C0C0"><font color="#000000">Comments:</font></td>
  </tr>
  <?php
$nums = 0;
$qri = mysql_query("SELECT * FROM ftp ORDER BY id",$db);
while ($kolumn=mysql_fetch_array($qri)) {
?>  <tr>
    <td width="16%" <?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>>
<a href="ftp://<?php echo $kolumn[3] . ":" . $kolumn[4] . "@" . $kolumn[1] . ":" . 
$kolumn[2]; 
?>">
<?echo $kolumn[1]; ?></a>&nbsp;</td>
    <td width="16%"<?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>><?echo $kolumn[2]; ?>&nbsp;</td>
    <td width="17%"<?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>><?echo $kolumn[3]; ?>&nbsp;</td>
    <td width="17%"<?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>><?echo $kolumn[4]; ?>&nbsp;</td>
    <td width="17%"<?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>><?echo $kolumn[5]; ?>&nbsp;</td>
    <td width="17%" <?php
if ($kolumn['online'] == 1) {
echo "bgcolor=\"red\"";
} ?>><?echo $kolumn[6]; ?>&nbsp;</td>
  </tr>
  <?php
++$nums;
  }
  ?>
</table>
<?php

echo "FTPs in datbase: ". $nums ."<br>\n";
}
mysql_close($db);
?>
<br>Code by: TPS_Sorcer (ICQ: 54915721)<br>MySQL server by: Cryo , HTTP 
server by: TPS_Sorcer<br>FTP search by Mog ICQ: 10389772 mooog @ 
efnet.remedy.nu </BODY>
</HTML>
