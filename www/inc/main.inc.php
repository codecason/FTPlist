This is the Remedy FTP List and FTP Search Engine.<br><br>

<font color="<?php echo $green;?>">Green</font> text in the status column 
= up with free slots<br>
<font color="<?php echo $yellow;?>">Yellow</font> text in the status 
column = up without free slots<br>
<font color="<?php echo $red;?>">Red</font> text in the status column = 
down<br><br>

The FTPs are crawled every <b><?php echo $crawltime;?></b> minutes. They 
are checked every <b><?php echo $checktime;?></b> minutes.<br>
Last crawl was at: <b><?php echo date("H:i",$lastcrawl);?></b>.<br>
Current FTP-List admin is: <b><?php echo $admin;?></b>.<br>
MOTD: <b><?php echo $motd;?></b>
<br>&nbsp;<p>
<table border="0" cellpadding="2" cellspacing="0">
 <tr>
  <td><b><a href="index.php?order_by=ip">IP</a>:<a 
href="index.php?order_by=port">Port</a></b></td><td><b><a 
href="index.php?order_by=status">Status</a></b></td><td><b><a 
href="index.php?order_by=uname">Username</a></b></td><td><b><a 
href="index.php?order_by=pass">Password</a></b></td><td><b>Files</b></td><td><b><a 
href="index.php?order_by=cat">Description</a></b></td>
 </tr>

<?php
/* Check order by */
$order_by = $_GET[order_by];

if (!isset($order_by)) $order_by = "status";

$query = "SELECT ip,port,uname,pass,cat,status,id FROM ftp ORDER BY $order_by, id";
$conn = mysql_query($query, $db);

while ($columnl = mysql_fetch_array($conn)) {

/* Get files */
db_query("SELECT COUNT(*) FROM files WHERE ftp = '". $columnl[6] ."'");
$files = $column[0];
/* db_query("SELECT SUM(size) FROM files WHERE ftp = '$columnl[6]'");
$mb = $column[0]/100000; // Megabyte
$files = $mb/1000; // Gigabyte
*/

?>
 <tr>
  <td><a href="ftp://<?php echo $columnl[2];?>:<?php echo 
$columnl[3];?>@<?php echo $columnl[0];?>:<?php echo $columnl[1];?>/"><?php 
echo $columnl[0];?>:<?php echo $columnl[1];?></a>&nbsp;</td><td><?php echo status($columnl[5]);?></td><td><?php echo $columnl[2];?></td><td><?php echo 
$columnl[3];?></td><td><?php echo $files; ?></td><td><?php echo $columnl[4];?></td>
 </tr>
<?php
}
?>
</table>
