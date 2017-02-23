# phpMyAdmin MySQL-Dump
# version 2.2.3
# http://phpwizard.net/phpMyAdmin/
# http://phpmyadmin.sourceforge.net/ (download page)
#
# Host: localhost
# Generation Time: Jul 14, 2002 at 01:13 AM
# Server version: 4.00.01
# PHP Version: 4.2.0
# Database : `ftplist`
# --------------------------------------------------------

#
# Table structure for table `files`
#

CREATE TABLE `files` (
  `id` int(11) NOT NULL auto_increment,
  `ftp` tinyint(4) NOT NULL default '0',
  `filename` text NOT NULL,
  `path` text NOT NULL,
  `type` varchar(4) NOT NULL default '',
  `size` mediumint(9) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `ftp` (`ftp`)
) TYPE=MyISAM;
# --------------------------------------------------------

#
# Table structure for table `ftp`
#

CREATE TABLE `ftp` (
  `id` int(9) NOT NULL auto_increment,
  `ip` text NOT NULL,
  `port` text NOT NULL,
  `uname` text NOT NULL,
  `pass` text NOT NULL,
  `cat` text NOT NULL,
  `locked` int(1) NOT NULL default '0',
  `status` int(1) NOT NULL default '0',
  PRIMARY KEY  (`id`)
) TYPE=MyISAM;
# --------------------------------------------------------

#
# Table structure for table `status`
#

CREATE TABLE `status` (
  `crawltime` int(11) NOT NULL default '90',
  `checktime` int(11) NOT NULL default '10',
  `admin` text NOT NULL,
  `motd` text NOT NULL,
  `lastcrawl` int(10) NOT NULL default '0',
  `version` varchar(4) NOT NULL default '',
  `searches` int(5) NOT NULL default '0'
) TYPE=MyISAM;

