#!/bin/bash

set -e

cd deps

curl -L --url https://github.com/Microsoft/GSL/archive/5905d2d77467d9daa18fe711b55e2db7a30fe3e3.zip --output GSL.zip
curl -L --url https://github.com/comphack/zlib/archive/comp_hack-20180425.zip --output zlib.zip
curl -L --url https://github.com/comphack/openssl/archive/comp_hack-20180424.zip --output openssl.zip
curl -L --url https://github.com/comphack/mariadb/archive/comp_hack-20181016.zip --output mariadb.zip
curl -L --url https://github.com/comphack/ttvfs/archive/comp_hack-20180424.zip --output ttvfs.zip
curl -L --url https://github.com/comphack/physfs/archive/comp_hack-20180424.zip --output physfs.zip
curl -L --url https://github.com/comphack/civetweb/archive/comp_hack-20180424.zip --output civetweb.zip
curl -L --url https://github.com/comphack/squirrel3/archive/comp_hack-20180424.zip --output squirrel3.zip
curl -L --url https://github.com/comphack/asio/archive/comp_hack-20161214.zip --output asio.zip
curl -L --url https://github.com/comphack/tinyxml2/archive/comp_hack-20180424.zip --output tinyxml2.zip
curl -L --url https://github.com/comphack/googletest/archive/comp_hack-20180425.zip --output googletest.zip
curl -L --url https://github.com/comphack/JsonBox/archive/comp_hack-20180424.zip --output JsonBox.zip
