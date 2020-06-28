Server Setup
============

This chapter describes how to install and configure the server
and related components.

Requirements
------------

Running the Server
^^^^^^^^^^^^^^^^^^

If running the server under Linux, you should install packages for the
following:

- `Qt <http://qt.nokia.com>`__ 4.8.x or higher (QtCore, QtNetwork,
  QtScript, QtSql, QtXml)

- `OpenSSL <http://www.openssl.org>`__ 0.9.8 or higher

- `MySQL <http://www.mysql.com>`_ 5 (recommened if you are running a
  production server or the website)

Compiling the Source Code
^^^^^^^^^^^^^^^^^^^^^^^^^

- Working C++ compiler (like GCC, MinGW, LLVM/Clang,
  Visual Studio 2010)

- `Qt <http://qt.nokia.com>`__ 4.8.x or higher (QtCore, QtGui,
  QtNetwork, QtScript, QtSql, QtWebKit, QtXml)

- `CMake <http://www.cmake.org>`__ 2.8 or higher

- `OpenSSL <http://www.openssl.org>`_ 0.9.8 or higher

- `zlib <http://zlib.net/>`_ (Qt uses this but it is also used
  directly in the code)

- `Doxygen <http://www.stack.nl/~dimitri/doxygen/>`_ (if you wish to build the API
  documentation)

- `Docbook XSL <http://docbook.sourceforge.net>`_ and
  `xmlto <https://fedorahosted.org/xmlto/>`_ (if you wish to build this guide
  and other documention)

- `Git <http://git-scm.com>`_ (if you wish to checkout a copy of
  the source code from the repository instead of using a pre-packaged
  distribution)


Linux Install & Setup
---------------------

Overview
^^^^^^^^

This section of the guide will describe installation and setup of the
server on an Ubuntu 16.04 LTS (https://www.ubuntu.com) system.
Installation on other Linux systems should be possible and this
guide could be adapted for such a system; however, installation on
these systems won't be described here. Similarly, DNS setup,
connecting to and maintaining a Linux server, basic Linux commands,
basic SQL and web development are out of the scope of this document.
Please look elsewhere online for an excellent selection of guides,
tutorials and videos on these topics.

Package Install
^^^^^^^^^^^^^^^

A packaged version of the server is available from this
`Ubuntu PPA <https://launchpad.net/~compomega/+archive/ubuntu/comphack>`_.
This guide will be using this PPA so if this does not work for your
system, please see the section in this guide describing how to build
the server from source. Install the package as follows:

.. code-block:: bash

    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:compomega/comphack
    sudo apt-get update
    sudo apt-get install comphack

Server Data
^^^^^^^^^^^

Update a copy of the server data or create your own server data. You
now want to upload these files to /var/lib/comp_hack/data and verify
the permissions of the files. First install some utilities for zip
and 7z files:

.. code-block:: bash

    sudo apt-get install unzip p7zip-full

Ensure the directory is created and owned by your account for
simplicity:

.. code-block:: bash

    sudo mkdir /var/lib/comp_hack/data
    sudo chown `whoami`:`whoami` /var/lib/comp_hack/data

Extract the files and ensure they can be read by the *comphack* user.
Here is one way to do this:

::

    chmod -R g+rX,o+rX /var/lib/comp_hack/data
    ls -lh /var/lib/comp_hack/data

The directory contents should now look like this:

::

    drwxr-xr-x 2 bob bob 4.0K Apr  7 15:00 data
    drwxr-xr-x 2 bob bob  36K Apr  7 15:00 events
    drwxr-xr-x 2 bob bob 4.0K Apr  7 15:00 scripts
    drwxr-xr-x 2 bob bob 4.0K Apr  7 15:00 shops
    drwxr-xr-x 2 bob bob 4.0K Apr  7 15:00 tokusei
    drwxr-xr-x 2 bob bob  44K Apr  7 15:00 zones

This directory also needs the BinaryData directory from the client
(re-encrypted with the COMP_hack key) and the QMP files from
Map/Zone/Model (also from the client). They should follow the same
path structure as the client. Create this directory and then upload
the client files:

.. code-block:: bash

    mkdir -p /var/lib/comp_hack/data/Map/Zone/Model

To verify the contents you can run these two commands:

.. code-block:: bash

    ls -lh /var/lib/comp_hack/data/BinaryData/
    ls -lh /var/lib/comp_hack/data/Map/Zone/Model/ | head -n 3 | tail -n 2

You should see these directories in */var/lib/comp_hack/data/BinaryData*:

::

    drwxr-xr-x 2 bob bob 36K Apr  7 15:15 Client
    drwxr-xr-x 2 bob bob 20K Apr  7 15:15 Shield

You should see QMP files in */var/lib/comp_hack/data/Map/Zone/Model*
like this:

::

    -rw-r--r-- 1 bob bob  2.2K Apr  7 15:16 ClientDebugMap.qmp
    -rw-r--r-- 1 bob bob   463 Apr  7 15:16 MA01_001_01.qmp

Once done verify all files and directories will be readable by the
*comphack* user.

MariaDB Setup (Optional)
^^^^^^^^^^^^^^^^^^^^^^^^

A production server should not use the default SQLite3 database. It's
recommended to install MariaDB (a fork of MySQL) instead. This
section will describe how to install and configure MariaDB. First,
install the packages:

.. code-block:: bash

    sudo apt-get install mariadb-server mariadb-client

You can generate a password for the MariaDB user with pwgen:

.. code-block:: bash

    sudo apt-get install pwgen
    pwgen -s 30 1

Now connect to the MariaDB server with the root account so we can
create the *comphack* account:

.. code-block:: bash

    sudo mysql -u root

Run the following commands at the MariaDB prompt replacing any
password with one generated above:

.. code-block:: sql

    CREATE USER 'comphack'@'localhost' IDENTIFIED BY 'password';
    GRANT ALL PRIVILEGES ON comp_hack.* TO 'comphack'@'localhost';
    GRANT ALL PRIVILEGES ON world.* TO 'comphack'@'localhost';
    exit

To make things easier change the ownership of the server
configuration to your local user account:

.. code-block:: bash

    sudo chown -R `whoami`:`whoami` /etc/comp_hack

Now edit */etc/comp_hack/lobby.xml* and change the database type
to *MARIADB*:

.. code-block:: xml

    <member name="DatabaseType">MARIADB</member>

While the file is open add a new section for the MariaDB
configuration. Make sure to change the password to match the one
generated above. This can go after the SQLite3 configuration:

.. code-block:: xml

    <member name="MariaDBConfig">
        <object>
            <member name="IP">127.0.0.1</member>
            <member name="Username">comphack</member>
            <member name="Password">password</member>
        </object>
    </member>

Open the file */etc/comp_hack/world.xml* and make the same database
configuration changes.

Server Configuration
^^^^^^^^^^^^^^^^^^^^

Only one configuration is required; however, this guide will walk
you through a few more useful changes as well. To make the channel
server connect-able you must change the external IP address in the
channel configuration. You can determine the IP address of the server
like so:

::

    dig +short myip.opendns.com @resolver1.opendns.com

Edit the file */etc/comp_hack/channel.xml* and add the external
IP address setting to match the IP address of your server:

.. code-block:: xml

    <member name="ExternalIP">127.1.2.3</member>

While in this file you may also want to change the name of the channel:

.. code-block:: xml

    <member name="Name">My Epic Channel</member>

It may also be a good idea to change the name of the world in
the */etc/comp_hack/world.xml* file:

.. code-block:: xml

    <member name="Name">World of Awesomeness</member>

Finally it is a good idea to change the initial admin account user
name and/or password (other details could be useful to change as
well) in the */etc/comp_hack/setup.xml* file:

.. code-block:: xml

    <member name="Username">myaccount</member>
    <member name="Password">password</member>

See the configuration section of this guide for a full list of
configuration options and what they mean.

Starting the Server
^^^^^^^^^^^^^^^^^^^

The server components may be started individually:

.. code-block:: bash

    sudo systemctl start comp_lobby
    sudo systemctl start comp_world
    sudo systemctl start comp_channel

Stopping the server components can be done by stopping all of the
components in sequence or simply stopping the lobby component:

.. code-block:: bash

    sudo systemctl stop comp_channel
    sudo systemctl stop comp_world
    sudo systemctl stop comp_lobby

If the channel server component is started it will start the lobby
and world first:

.. code-block:: bash

    sudo systemctl start comp_channel

The status of the servers can be shown as well:

.. code-block:: bash

    sudo systemctl status comp_lobby

Looking at the status of the server components or the log files
in */var/log/comp_hack* is very important if the server components
are not starting or functioning properly.

Connecting to the Server
^^^^^^^^^^^^^^^^^^^^^^^^

To connect to the server with a client you must create two files:
*webaccess.sdat* and *ImagineClient.dat*. To create the
*webaccess.sdat* file, first create a file called *webaccess.dat*
with the contents below. Be sure to change the domain name as well
as the protocol if you are using https instead.

::

    <dbnet = http://www.myimagineserver.online/index/auth?user_id=%s&user_password=%s&character_name=%s&world_id=%d>
    <dcoshop = http://www.myimagineserver.online/index/auth?user_id=%s&user_password=%s&character_name=%s&world_id=%d&ref=shop>
    <slot = http://www.myimagineserver.online/index/auth?ref=ddslot&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s&mid=%d>
    <roulette = http://www.myimagineserver.online/index/auth?ref=jr&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s>
    <videogame = http://www.myimagineserver.online/index/auth?ref=tower&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s&mid=%d>
    <slotvip = http://www.myimagineserver.online/index/auth?ref=ddslotv&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s&mid=%d>
    <roulettevip = http://www.myimagineserver.online/index/auth?ref=jrv&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s>
    <videogamevip = http://www.myimagineserver.online/index/auth?ref=towerv&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s&mid=%d>
    <kino = http://www.myimagineserver.online/index/auth?ref=casino&user_id=%s&user_password=%s&character_name=%s&world_id=%d&session_id=%s>
    <login = http://www.myimagineserver.online:10999/>
    <birthday = http://www.myimagineserver.online/index/auth?user_id=%s&user_password=%s>

Now encrypt the file:

.. code-block:: bash

    comp_encrypt webaccess.dat webaccess.sdat

The other file, *ImagineClient.dat*, contains the lobby server to
connect to. Change this as appropriate and save it as the
*ImagineClient.dat* file:

::

    -ip www.myimagineserver.online
    -port 10666

Place these two files into an install of the patched 1.666 client
and it should now connect to your new server!

Building from Source (Windows)
------------------------------

This section of the guide will walk the user through building the
server from source. The guide assumes the user wants to pull the
source with Git for later modifications. There are many programs
out there to help you use Git; however, GitKraken was selected for
this guide. If you prefer a different choice, please adjust the
instructions accordingly.

Prerequisites
^^^^^^^^^^^^^

- `Visual Studio 2015 <https://visualstudio.microsoft.com/vs/older-downloads/>`_ (newer versions may be possible but have not been tested)

- `GitKraken <https://www.gitkraken.com>`_ (any Git client software will work)

- `CMake <https://cmake.org>`__

- `Qt5 Open Source <https://www.qt.io>`_ (`*Offline Download* <https://www.qt.io/offline-installers>`_)

Visual Studio 2015
^^^^^^^^^^^^^^^^^^

It's very important that the user either install Visual Studio 2015
with the Visual C++ features or modify the install to add in these
features. Without the support for C++ it is not possible to build
the server. See :ref:`vs2015cpp` below for the options needed to
enable C++ support.

.. _vs2015cpp:

Installing Visual Studio 2015 with C++ support.
"""""""""""""""""""""""""""""""""""""""""""""""

.. image:: /_static/vs2015cpp.png

CMake
^^^^^

When installing CMake it's very important to add it to the system
path. The option to do this during install can be seen in
:ref:`cmakewin`. This will be needed later during the build process.

.. _cmakewin:

Installing CMake into the system path.
""""""""""""""""""""""""""""""""""""""

.. image:: /_static/cmake.png

Qt5
^^^

Some of the tools use Qt for the GUI (as it works on Windows,
macOS and Linux). When installing Qt be sure to select the
appropriate MSVC 2015 builds (at least 64-bit but 32-bit can be
handy too). Be sure to check *Qt WebEngine* or the updater will
not build. These options can be seen in :ref:`qtwin`.

.. _qtwin:

Qt5 install components.
"""""""""""""""""""""""

.. image:: /_static/qt.png

Checkout Sources
^^^^^^^^^^^^^^^^

If you plan to contribute to the project, fork the main project.
This will commit all changes to your copy of the repository.
Later these can be merged into the main project repository with
a Pull Request. To fork, click the button seen in :ref:`fork_project`.
If you do not plan to contribute to the project, you may use the
link for the main repository instead.

.. _fork_project:

Forking the project.
""""""""""""""""""""

.. image:: /_static/fork.png

Once the project has been forked, start GitKraken and clone it
from your GitHub account as seen in :ref:`clone1`.

.. _clone1:

Cloning a fork off GitHub.
""""""""""""""""""""""""""

.. image:: /_static/clone1.png

If you wish to clone the main repository instead, find the URL on
the GitHub project page and enter it into GitKraken as shown below
in :ref:`clone2` and :ref:`clone3`.

.. _clone2:

Getting the repository URL off GitHub.
""""""""""""""""""""""""""""""""""""""

.. image:: /_static/clone2.png

.. _clone3:

Cloning the main project off GitHub.
""""""""""""""""""""""""""""""""""""

.. image:: /_static/clone3.png

Once the project is cloned you should see a window similar to
:ref:`clone4`.

.. _clone4:

Project cloned in GitKraken.
""""""""""""""""""""""""""""

.. image:: /_static/clone4.png

.. COMMENT: Checkout Sources

Generate Project
^^^^^^^^^^^^^^^^

In the root of the project directory you should see two batch
files called *vsbuild_x86.bat* and *vsbuild_x64.bat* for 32-bit
and 64-bit respectively. These can be edited in a text editor. In
this example the Qt5 directory had to be changed on the
*CMAKE_PREFIX_PATH* line for 32-bit:

.. code-block:: bash

    set CMAKE_PREFIX_PATH=C:\Qt\5.10.1\msvc2015
    cmake -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -G"Visual Studio 14 2015" ..

This also needed to be updated for 64-bit as well:

.. code-block:: bash

    set CMAKE_PREFIX_PATH=C:\Qt\5.10.1\msvc2015_64
    cmake -DCMAKE_CONFIGURATION_TYPES="Debug;Release" -G"Visual Studio 14 2015 Win64" ..

Note that this path may very depending on which version of Qt5 was
installed and to what location. Please look for the appropriate
directory on your install. Run the appropriate batch file and you
should get a dialog similar to the following:

.. _project_generation:

Project generation with CMake.
""""""""""""""""""""""""""""""

.. image:: /_static/project_generation.png

After the CMake generation the *comp_hack.sln* solution file can
be found in the *build64* directory as shown in :ref:`build64`. The
project can then be opened and built in Visual Studio 2015 as
shown in :ref:`vssolution`.

.. _build64:

Project solution file in the build64 directory.
"""""""""""""""""""""""""""""""""""""""""""""""

.. image:: /_static/build64.png

.. _vssolution:

Project opened in Visual Studio 2015.
"""""""""""""""""""""""""""""""""""""

.. image:: /_static/vssolution.png

Client Updater with Nginx
-------------------------

Overview
^^^^^^^^

This section of the guide will describe installation and setup
of a client update server on an Ubuntu 16.04 LTS
(https://www.ubuntu.com) system with Nginx. Installation on
other systems should be possible and this guide could be adapted
for such a system; however, installation on these systems won't
be described here. Similarly, DNS setup, connecting to and
maintaining a Linux server, basic Linux commands, basic SQL and
web development are out of the scope of this document. Please
look elsewhere online for an excellent selection of guides,
tutorials and videos on these topics.

Installing Nginx
^^^^^^^^^^^^^^^^

You just need to install the package as follows:

.. code-block:: bash

    sudo apt-get update
    sudo apt-get install nginx

Client Files
^^^^^^^^^^^^

Before you start, refer back to the Linux install section and
make sure the tools are installed (the main server package isn't
really required). Make sure you can run this command:

.. code-block:: bash

    comp_rehash

You should see output like this:

::

    SYNTAX: comp_rehash --base BASE --overlay OVERLAY

Now create a directory for the client files updater:

.. code-block:: bash

    sudo mkdir /var/www/updates
    sudo chown `whoami`:`whoami` /var/www/updates
    mkdir -p /var/www/updates/files/{base,overlay}

Now upload the original 1.666 update files (obtained elsewhere)
into */var/www/updates/base*. You should see the *.compressed*
files as well as the *hashlist.dat*. The directory should look
like the following:

::

    Avatar                        ImagineClient.dat             msvcp71.dll
    BinaryData                    ImagineClient.exe             msvcp71.dll.compressed
    casket                        ImagineClient.exe.compressed  msvcr71.dll
    Devil                         ImagineOption.exe             msvcr71.dll.compressed
    effect                        ImagineOption.exe.compressed  NPC
    Event                         ImagineUpdate.exe             redist
    GungHoUpdate.dat              ImagineUpdate.exe.compressed  rtdlist.pfl
    GungHoUpdate.dat.compressed   Interface                     Shaders
    GunHoUpdate.dat               Map                           sound
    GunHoUpdate.dat.compressed    MSNUpdate.dat                 Title
    HangameUpdate.dat             MSNUpdate.dat.compressed      webaccess.sdat
    HangameUpdate.dat.compressed  mss32.dll
    hashlist.dat                  mss32.dll.compressed

Now upload the files you wish to replace or add into
*/var/www/updates/overlay*. This could include:

- Re-encrypted BinaryData files.

- Client executable patch.

- Alternate client updater.

- New or modified content.

The alternate client updater is included as part of the server
software. You can download it in the zip release to avoid an
install of the server software. Once the various files have been
added the directory may look similar to the following:

::

    bearer               libGLESV2.dll       Qt5WebChannel.dll
    BinaryData           libstdc++-6.dll     Qt5WebEngineCore.dll
    comp_client.dll      opengl32sw.dll      Qt5WebEngineWidgets.dll
    comp_client.xml      platforms           Qt5Widgets.dll
    D3Dcompiler_47.dll   position            Qt5Xml.dll
    iconengines          Qt5Core.dll         QtWebEngineProcess.exe
    imageformats         Qt5Gui.dll          resources
    ImagineClient.exe    Qt5Network.dll      translations
    ImagineUpdate.dat    Qt5Positioning.dll  VersionData.txt
    ImagineUpdate.exe    Qt5Qml.dll          webaccess.sdat.local
    Interface            Qt5Quick.dll        webaccess.sdat.myserv
    libEGL.dll           Qt5SerialPort.dll
    libgcc_s_sjlj-1.dll  Qt5Svg.dll

Once all the files are in place, run the *comp_rehash* command to
generate the new *hashlist.dat* file in the overlay. This tool will
also generate *hashlist.ver* which is specific to the alternate
updater and *.compressed* files which are needed by the updater.
This command should be run again if the contents of the overlay are
changed. Here is the command:

.. code-block:: bash

    comp_rehash --base /var/www/updates/files/base --overlay /var/www/updates/files/overlay

Lastly the root directory */var/www/updates* should contain the
site to display in the updater. This could be a static page or a
dynamic site (with Ruby/PHP/Java/etc).

Version Data
^^^^^^^^^^^^

The custom updater called the *VersionData.txt*. It has a *[versions]*
section at the top. This lists a sequence of properties for each
server the client may connect to. Each server should include a
*title*, *server* and *tag*. The *title* is the name shown in the
updater. The *server* is the domain name or IP address as well as
the port that will be written into the *ImagineClient.dat*. Finally,
the *tag* is used to describe a section that follows. Here is an
example:

.. code-block:: ini

    [versions]
    title = My Imagine Server
    server = myimagineserver.online:10666
    tag = myserv
    title = Local Server
    server = 127.0.0.1:10666
    tag = local

    [myserv]
    webaccess.sdat

    [local]
    webaccess.sdat

Each section that is tagged lists files that should be copied. For
example, in the *[myserv]* section it lists *webaccess.sdat*. When
the updater launches the client it will copy the file
*webaccess.dat.myserv* to the file named *webaccess.sdat*. This
copies the files with the tag at the end of the name to the file
listed. This allows you to change files depending on the client
being connected to. This is usually limited to the webaccess.sdat
as this is needed for the login box and the in-game website (shop,
casino, etc).

Nginx Configuration
^^^^^^^^^^^^^^^^^^^

The files needed for the client updater are now in place. The
next step is to configure the web server (Nginx). Create a new
file */etc/nginx/sites-available/updates* with the following contents:

::

    server {
        listen 80;
        listen [::]:80;

        server_name updates.myimagineserver.online;

        root /var/www/updates;

        location / {
            # First attempt to serve request as file, then
            # as directory, then fall back to displaying a 404.
            try_files $uri $uri/ =404;

            add_header X-Robots-Tag "noindex, nofollow, nosnippet, noarchive";
            add_header Cache-Control "must-revalidate, private, no-cache, no-store";
        }

        location ~ \.(php|pl|py|sh|fcgi)$ {
            return 403;
        }

        location ~* ^/files/(.+)$ {
            root /var/www/updates/files;

            try_files /overlay/$1 /base/$1 =404;

            add_header X-Robots-Tag "noindex, nofollow, nosnippet, noarchive";
            add_header Cache-Control "must-revalidate, private, no-cache, no-store";
        }
    }

Change the *server_name* setting to match the domain name of your
server and configure any sub-domain as needed. This configuration
will search for files in */var/www/updates/files/overlay* first. If
the file is not found it will search in */var/www/updates/files/base*.
If the file is still not found it will generate a 404. The
*Cache-Control* header is important if using the original updater as
this updater will use IE to cache the update files. This can cause
update problems when the files are changed. Now enable the
configuration and restart Nginx:

.. code-block:: bash

    sudo ln -sf /etc/nginx/sites-available/updates /etc/nginx/sites-enabled/updates
    sudo service nginx restart

The updater should now be functional. Remember to edit the
*ImagineUpdate.dat* to point to the server:

.. code-block:: ini

    [Setting]
    BaseURL1 = http://updates.myimagineserver.online/files
    Information = http:/updates.myimagineserver.online

Website with PHP & Nginx
------------------------

Overview
^^^^^^^^

This section of the guide will describe installation and setup of
a website server software on an Ubuntu 16.04 LTS
(https://www.ubuntu.com) system with Nginx, PHP and MariaDB.
Installation on other systems should be possible and this guide
could be adapted for such a system; however, installation on these
systems won't be described here. Similarly, DNS setup, connecting
to and maintaining a Linux server, basic Linux commands, basic SQL
and web development are out of the scope of this document. Please
look elsewhere online for an excellent selection of guides,
tutorials and videos on these topics.

Installing Nginx
^^^^^^^^^^^^^^^^

You just need to install the package as follows:

.. code-block:: bash

    sudo apt-get update
    sudo apt-get install nginx

Installing PHP
^^^^^^^^^^^^^^

The server needs a new version of PHP (7.1+) and can be installed
from a PPA. Here is how:

.. code-block:: bash

    sudo add-apt-repository ppa:ondrej/php
    sudo apt-get update
    sudo apt-get install php7.1-fpm php7.1-cli php7.1-mcrypt php7.1-gd php7.1-mysql php7.1-pgsql php7.1-imap php7.1-memcached
    sudo apt-get install php7.1-mbstring php7.1-xml php7.1-curl php7.1-bcmath php7.1-sqlite3 php7.1-xdebug

Next, install the PHP package manager called composer:

.. code-block:: bash

    php -r "readfile('http://getcomposer.org/installer');" | sudo php -- --install-dir=/usr/bin/ --filename=composer

Installing MariaDB
^^^^^^^^^^^^^^^^^^

The website software uses Laravel. Laravel needs a new version of
MariaDB (10.2+) so this must also be installed from a PPA. Here is
how to install the latest version:

.. code-block:: bash

    sudo apt-get install software-properties-common
    sudo apt-key adv --recv-keys --keyserver hkp://keyserver.ubuntu.com:80 0xF1656F24C74CD1D8
    sudo add-apt-repository 'deb [arch=amd64,i386,ppc64el] http://sfo1.mirrors.digitalocean.com/mariadb/repo/10.2/ubuntu xenial main'
    sudo apt-get update
    sudo apt-get install mariadb-server

Next you should enable root access again. To do this edit
*/etc/mysql/mariadb.conf.d/50-server.cnf* and add the following in
the *[mysqld]* section:

::

    plugin-load-add = auth_socket.so

Now restart MariaDB and open the MariaDB console:

.. code-block:: bash

    sudo systemctl restart mariadb.service
    sudo mysql -u root

Lastly create the account and database to be used by the website.
Remember to change the password (and the username and database name
if you want):

.. code-block:: sql

    CREATE USER 'compsite'@'localhost' IDENTIFIED BY 'password';
    GRANT ALL PRIVILEGES ON comp_site.* TO 'compsite'@'localhost';
    CREATE DATABASE comp_site CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

Website Install
^^^^^^^^^^^^^^^

This will install the develop branch of the website. This is
bleeding edge and may break. Once it's stable this will be updated
to describe how to use a released version. This first section of
commands will checkout the website files, install the needed PHP
dependencies, generate an application key, and copy the default
configuration. Do *not* re-run the key generation command or wipe
out your *.env* file as this may break account logins. Run these
commands for a fresh install:

.. code-block:: bash

    sudo chown www-data:www-data /var/www/html
    sudo rm /var/www/html/*
    cd /var/www
    sudo -u www-data -H git clone https://github.com/comphack/website.git html
    cd /var/www/html
    sudo -u www-data -H composer install --optimize-autoloader
    sudo -u www-data -H cp .env.example .env
    sudo -u www-data -H php artisan key:generate
    sudo -u www-data -H nano .env

Now edit the *.env* file for your site configuration. The important
options to change are *APP_URL*, *DB_DATABASE*, *DB_USERNAME*,
*DB_PASSWORD* and *COMP_API*. The *COMP_API* points to the channel
server on the same server. This may need to be changed if the
website is located on a different server.

::

    APP_NAME=COMP_hack
    APP_ENV=production
    APP_KEY=
    APP_DEBUG=false
    APP_URL=https://www.myimagineserver.online
    DB_CONNECTION=mysql
    DB_HOST=127.0.0.1
    DB_PORT=3306
    DB_DATABASE=comp_site
    DB_USERNAME=compsite
    DB_PASSWORD=hackMe
    COMP_API=http://127.0.0.1:10999/api

After saving the configuration you must run the config cache
command. Run this every time the *.env* file has changed. For
example, when you run the second command to setup the database,
if you run into migration issues and the account information has
not changed you may need to re-run the config cache command.

.. code-block:: bash

    sudo -u www-data -H php artisan config:cache
    sudo -u www-data -H php artisan migrate

Nginx Configuration
^^^^^^^^^^^^^^^^^^^

The files needed for the website are now in place. The next step is
to configure the web server (Nginx). Create a new file or edit the
file */etc/nginx/sites-available/default* with the following contents:

::

    server {
        listen 80 default_server;
        listen [::]:80 default_server;

        server_name www.myimagineserver.online;

        root /var/www/html/public;

        charset utf-8;

        add_header X-Frame-Options "SAMEORIGIN";
        add_header X-XSS-Protection "1; mode=block";
        add_header X-Content-Type-Options "nosniff";

        index index.html index.htm index.php;

        location / {
            try_files $uri $uri/ /index.php?$query_string;
        }

        location = /favicon.ico { access_log off; log_not_found off; }
        location = /robots.txt  { access_log off; log_not_found off; }

        error_page 404 /index.php;

        location ~ \.php$ {
            include snippets/fastcgi-php.conf;
            fastcgi_pass unix:/run/php/php7.1-fpm.sock;
        }

        location ~ /\.(?!well-known).* {
            deny all;
        }
    }

Change the *server_name* setting to match the domain name of your
server and configure any sub-domain as needed. It may be a good
idea to configure the site with SSL. This won't be described here.
Now enable the configuration and restart Nginx:

.. code-block:: bash

    sudo ln -sf /etc/nginx/sites-available/default /etc/nginx/sites-enabled/default
    sudo service nginx restart

Wrapping Up
^^^^^^^^^^^

The website should now be functional. It would be a good idea to
configure the firewall to block access to port 10999 as it is no
longer needed by the outside world. Nothing else should be
connecting to this port or it may cause issues with the website.
You can edit the *ImagineUpdate.dat* to point to the server:

::

    [Setting]
    BaseURL1 = http://www.myimagineserver.online/updater
    Information = http:/updates.myimagineserver.online

To login to the site with an existing account (for example, the
default admin account) you must use the claim form. For new
accounts use the register function on the site. Go here to claim
(fixing your domain name): http://www.myimagineserver.online/claim

Finally, you should point the client *webaccess.sdat* to the
website instead (so you may block access to port 10999 and still
use the web-based login). See the updater section for how to edit
this file and push the new version. Here is what the configuration
may look like:

::

    <login = http://www.myimagineserver.online/webauth>
