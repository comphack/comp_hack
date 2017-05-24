USE mysql;
DROP USER IF EXISTS 'testuser'@'localhost';
CREATE USER 'testuser'@'localhost' IDENTIFIED BY 'un1tt3st';
GRANT ALL PRIVILEGES ON *.* TO 'testuser'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;