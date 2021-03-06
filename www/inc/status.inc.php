<?php
/* Get MySQL version */
db_query('SELECT Version() as version');
$mysql_version=$column[0];

/* Get kernel version */
exec("uname -r > /tmp/ftplist.php.tmp");
$filename = "/tmp/ftplist.php.tmp";
$fd = fopen ($filename, "r");
$kernel_version = fread ($fd, filesize ($filename));
fclose ($fd);

/* Get uptime */
exec("uptime > /tmp/ftplist.php.tmp");
$filename = "/tmp/ftplist.php.tmp";
$fd = fopen ($filename, "r");
$uptime_r = fgets($fd, 4096);
fclose ($fd);
preg_match("/.+ up (.+),.+[0-9]+ user/", $uptime_r, $matches);
$uptime = $matches[1];


/* Check if MOSIX cluster config is present */
if(file_exists("/etc/mosix.map")) $mosix = "Yes";
else $mosix = "No";

/* Count number of files in DB */
db_query("SELECT COUNT(*) FROM files");
$files = $column[0];

db_query("SELECT searches FROM status");
$searches = $column[0];
?>
Info and Status on the FTP-List/FTP-Crawler system at this LAN.<br><p>

<table width="70%" border="0" cellpadding="0" cellspacing="0">
<tr><td width="50%"><b>FTPList/FTPCrawler version</b>:</td><td width="50%"><?php echo $version; ?></td></tr>
<tr><td width="50%"><b>MySQL version</b>:</td><td width="50%"><?php echo $mysql_version; ?></td></tr>
<tr><td width="50%"><b>Web Server</b>:</td><td width="50%"><?php echo $SERVER_SOFTWARE; ?></td></tr>
<tr><td width="50%"><b>Linux kernel</b>:</td><td width="50%"><?php echo $kernel_version; ?></td></tr>
<tr><td width="50%"><b>Clustered</b>:</td><td width="50%"><?php echo $mosix; ?></td></tr>
<tr><td width="50%"><b>Files in database</b>:</td><td width="50%"><?php echo $files; ?></td></tr>
<tr><td width="50%"><b>Number of searches</b>:</td><td width="50%"><?php echo $searches; ?></td></tr>
<tr><td width="50%"><b>Web Server uptime</b>:</td><td width="50%"><?php echo $uptime; ?> Hours</td></tr>
</table>
