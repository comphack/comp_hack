#!/bin/bash

set -e

cd deps

curl -L --url https://github.com/Microsoft/GSL/archive/5905d2d77467d9daa18fe711b55e2db7a30fe3e3.zip --output GSL.zip
curl -L --url https://github.com/comphack/zlib/archive/comp_hack-20161214.zip --output zlib.zip
curl -L --url https://github.com/comphack/openssl/archive/comp_hack-20170520.zip --output openssl.zip
curl -L --url https://github.com/comphack/mariadb/archive/comp_hack-20170530.zip --output mariadb.zip
curl -L --url https://github.com/comphack/ttvfs/archive/comp_hack-20161214.zip --output ttvfs.zip
curl -L --url https://github.com/comphack/physfs/archive/comp_hack-20170117.zip --output physfs.zip
curl -L --url https://github.com/comphack/sqrat/archive/comp_hack-20170905.zip --output sqrat.zip
curl -L --url https://github.com/comphack/civetweb/archive/comp_hack-20170530.zip --output civetweb.zip
curl -L --url https://github.com/comphack/squirrel3/archive/comp_hack-20161214.zip --output squirrel3.zip
curl -L --url https://github.com/comphack/asio/archive/comp_hack-20161214.zip --output asio.zip
curl -L --url https://github.com/comphack/tinyxml2/archive/comp_hack-20161214.zip --output tinyxml2.zip
curl -L --url https://github.com/comphack/googletest/archive/comp_hack-20161214.zip --output googletest.zip
curl -L --url https://github.com/comphack/JsonBox/archive/comp_hack-20170610.zip --output JsonBox.zip

cd deps

unzip GSL.zip
unzip zlib.zip
unzip openssl.zip
unzip mariadb.zip
unzip ttvfs.zip
unzip physfs.zip
unzip sqrat.zip
unzip civetweb.zip
unzip squirrel3.zip
unzip asio.zip
unzip tinyxml2.zip
unzip googletest.zip
unzip JsonBox.zip

mv GSL-* GSL
mv zlib-* zlib
mv openssl-* openssl
mv mariadb-* mariadb
mv ttvfs-* ttvfs
mv physfs-* physfs
mv sqrat-* sqrat
mv civetweb-* civetweb
mv squirrel3-* squirrel3
mv asio-* asio
mv tinyxml2-* tinyxml2
mv googletest-* googletest
mv JsonBox-* JsonBox
