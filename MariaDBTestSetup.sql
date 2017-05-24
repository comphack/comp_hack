USE mysql;
# See: https://stackoverflow.com/questions/598190/mysql-check-if-the-user-exists-and-drop-it
GRANT USAGE ON *.* TO 'testuser'@'localhost';
DROP USER 'testuser'@'localhost';
CREATE USER 'testuser'@'localhost' IDENTIFIED BY 'un1tt3st';
GRANT ALL PRIVILEGES ON *.* TO 'testuser'@'localhost' WITH GRANT OPTION;
FLUSH PRIVILEGES;
