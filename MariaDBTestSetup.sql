USE mysql;
CREATE USER 'testuser'@'localhost' IDENTIFIED BY 'un1tt3st';
GRANT ALL PRIVILEGES ON *.* TO 'testuser'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;
