#
# Table structure for table 'server_tbl'
#
CREATE TABLE server_tbl (
  id int NOT NULL auto_increment,
  ip varchar(15),
  hostname varchar(100),
  port int DEFAULT '8000',
  genre varchar(100),
  title varchar(100),
  currentusers int DEFAULT '0',
  maxusers int DEFAULT '256',
  starttime datetime,
  url varchar(100),
  alt int,
  bitrate int DEFAULT '128',
  touched int DEFAULT '0' NOT NULL,
  touchtime datetime DEFAULT '0000-00-00 00:00:00' NOT NULL,
  PRIMARY KEY (id)
);
