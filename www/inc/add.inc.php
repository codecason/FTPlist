<?php
if($_POST[go] == "1") processadd();

else {
?>

<form method="POST">
 
<table width="70%" border="0" cellpadding="0" cellspacing="0">
<tr><td width="50%"><b>IP</b>:</td><td width="50%"><input name="ip" value=""></td></tr>
<tr><td width="50%"><b>Port</b>:</td><td width="50%"><input name="port" value="21"></td></tr>
<tr><td width="50%"><b>Username</b>:</td><td width="50%"><input name="uname" value=""></td></tr>
<tr><td width="50%"><b>Password</b>:</td><td width="50%"><input name="pass" value=""></td></tr>
<tr><td width="50%"><b>Description</b>:</td><td width="50%"><input name="cat" value=""></td></tr>
</table>

<br>
<input type="hidden" name="go" value="1">
<input type="submit" value="Add">
</form>

<?php
}

function processadd() {
  global $db;

  if($_POST[ip] == "" || $_POST[port] == "" || $_POST[uname] == "" || $_POST[pass] == "") {
    echo "Please fill out all fields.";
    return;
  }

  if($_POST[ip] == "127.0.0.1") {
    echo "IP address cannot be localhost (127.0.0.1).";
    return;
  }
  
  if($_POST[port] == 0) {
    echo "Invalid port.";
    return;
  }
  
  mysql_query("INSERT INTO `ftp` (`ip`, `port`, `uname`, `pass`, `cat`) VALUES ('$_POST[ip]', '$_POST[port]', '$_POST[uname]', '$_POST[pass]', '$_POST[cat]')", $db);
  
  echo "<br>Thank you for adding your FTP to the FTP-List.";
}
?>
