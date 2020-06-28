Hacker's Guide
==============

.. role:: raw-html(raw)
    :format: html

This chapter describes how the code is architected. The hope is
that it will enable you to maintain, contribute to, or make small
modifications to the project.

Code Branches and Tags
----------------------

The code is split into two main branches: *master* and *develop*.
Any work on the next release of the project is done in *develop*.
When a release is complete it is merged into *master*. The
*master* branch will always point to the latest stable version of
the code. In addition to these two branches there is a special
*coverity_scan* branch based off *develop* that is used for
static analysis. All other branches correspond to the code name
for a major release (*pixie*, *lilim*, *kodama*, etc). When
working on updates for a service pack or hotfix the change should
go into the branch for that major release. It should then be
merged, cherry-picked or re-implemented in the *develop* branch
for upcoming releases.

Each release of a major version, service pack or hotfix should
have a tag. The tag should consist of the version number, code
name, service pack (if any) and hotfix number (if any). The
version number works as follows: MAJOR.MINOR.PATCH. In this case
the major version is the numerical representation of the code
name (1=pixie, 2=kodama, 3=lilim, etc). The minor version number
is the service pack and the patch is the hotfix number. When making
a pre-release the minor is usually set to 8 and the patch is set to
9. The minor would be incremented for a second release candidate.
For example, lilim service pack 1 hotfix 2 would be *3.1.2*.

Build System
------------

This project uses `CMake <https://www.cmake.org>`_ to generate the
make files (on Linux) or project files (for Visual Studio) needed
to build the server applications and utilities. Currently this is
known working on Linux with make files or Ninja scripts as well as
Visual Studio 2015. The software may be build-able with new
versions of Visual Studio, Eclipse, Xcode or other platforms and
build systems; however, these have not been tested yet and may
require slight modifications. It is known to build with the GCC,
Clang and MSVC compilers.

Dependencies
^^^^^^^^^^^^

Dependencies are handled automatically by the CMake build scripts.
There is four ways you can handle this:

#. Extract pre-built binaries into a *binaries* directory in the
   root folder for the project.

#. Place zip files of the dependency source code in the *deps*
   directory in the root folder for the project.

#. Let the CMake scripts download the source zip files from GitHub.

#. Let the CMake scripts download the source from the GitHub
   repositories. To do this you must set -DGIT_DEPENDENCIES=ON when
   generating the build scripts.

Except in the case of pre-built binaries, the CMake script will
build the external projects and then import the libraries into the
build environment. The imported libraries should include the
interface include directories needed to use in a project that links
to them. For details on how these are built and where to add a new
dependency, see *cmake/external.cmake* as well as
*cmake/binaries.cmake*. When building the Debian package the source
code needs to be included for the build to succeed (especially if
building for a PPA). When adding a dependency be sure to update the
*contrib/sources.sh* script to download the new dependency for these
packages.

Dependencies are built static with the same build configuration as
the main project. For MSVC you may use the static runtime as well
by setting -DUSE_STATIC_RUNTIME=ON when generating the build scripts.

There is two dependencies that are not built external. One is
*sqlite* which is included as a single file in the *deps/sqlite3*
directory. The other is *sqrat* which consists of a series of
template based headers. The *sqrat* dependency can fluctuate quite
a bit so it was included as a submodule in the *deps/sqrat* directory.
Be sure to initialize and update this submodule.

Automated Builds
^^^^^^^^^^^^^^^^

The project is setup to create automated builds for Linux and Windows
using `Travis CI <https://travis-ci.org>`_ and
`AppVeyor <https://www.appveyor.com>`_ respectively. The scripts used
to build the project in these environments can be found in the root
directory of the project as *.travis.yml* and *.appveyor.yml*
respectively. These builds are done in two to four stages: the
external dependencies, objgen (optional), libobjects (optional) and
the main project.

First, there is an `external project repository <https://github.com/comphack/external>`_
that is also built in the same way as the main project. This has a
mirror copy of the *cmake/external.cmake* script. This will build
and zip up the pre-compiled binaries. This is then added as a
release for that `GitHub project <https://github.com/comphack/external/releases>`_.
The Windows build may need to be added manually. The scripts are
then updated by a developer to point to the new release. Then the
automated builds will run using these pre-built binaries to speed
up the build process.

The `objgen utility and library <https://github.com/comphack/objgen>`_
can now be built as a separate project to speed up the automated
builds. This is built and added to the main build in a similar
fashion to the external project. It needs the external project
just like the main build.

The `objects library <https://github.com/comphack/libobjects>`_
can now be built as a separate project to speed up the automated
builds. This contains a copy of the object schema in libcomp.
Both projects should be updated when this schema changes. This is
built and added to the main build in a similar fashion to the
external project. It needs the external project just like the
main build. It also needs the objgen utility and library build
mentioned above.

The automated builds may use Cotire for pre-compiled headers
(PCH) and combine multiple small source files into a single file
to speed up the build process. Usually the Linux GCC build on
Travis CI is the most picky when it comes to warnings followed
by Clang. The Visual Studio build emits warnings but does not
error out when they are generated.

The Windows build on AppVeyor will generate the zip and msi
releases as artifacts that you may download and test with. These
are used during the release process as the clean build of the
project.

Finally, if you merge code from *develop* into the
*coverity_scan* branch it will run a static analysis of the code
provided by `Coverity <https://scan.coverity.com/>`_.

Build Options
^^^^^^^^^^^^^

Here is a list of build options you may set when running CMake:

BUILD_DREAM
"""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** ON

This will build the dream and sandman sub-projects if they are
cloned into the source tree as the directories *dream* and
*tools/sandman* respectively.

GIT_DEPENDENCIES
""""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will download the external dependencies from the Git
repositories instead of downloading a zip release. The branch for
each external is specified in the *cmake/external.cmake* file.

SINGLE_SOURCE_PACKETS
"""""""""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will combine all the packet parsing source files into a
single source file for fast building. Enabling this option could
cause issues depending on how the source was written; however,
these issues are usually caught during the automated build.

SINGLE_OBJGEN
"""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will combine all the objgen generated source files into a
single source file for fast building. This is currently not used
because of circular dependencies on the template functions used
to bind the objects into the scripting language (squirrel).

USE_COTIRE
""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will use Cotire to generate and build using a pre-compiled
header (PCH). This is used by the automated builds to speed up the
build process.

NO_WARNINGS
"""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will disable the generation of errors when a warning is
omitted. The warnings will still be shown but will not halt the
build. This is handy when building on a new Linux system or for
the Debian package (for the PPA). This should not be used during
development or on the automated build system.

DISABLE_TESTING
"""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This disables the build for the unit test applications.

USE_STATIC_RUNTIME
""""""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This disables the build for the unit test applications.

USE_PREBUILT_LIBCOMP
""""""""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will use the pre-built objgen and libcomp libraries and
tools to speed up the build process. This was added to prevent
timeout of the automated builds.

GENERATE_DOCUMENTATION
""""""""""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** ON for Linux; OFF for Windows

This will enable or disable generation of the Doxygen API
documentation and the Docbook guide you are reading now. If
enabled, these only generate if the tools needed are found on
the system.

WINDOWS_SERVICE
"""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will build the server applications as Windows services
instead of a standalone application. This option is turned on for
the AppVeyor build for official releases but may not be useful
when developing.

COVERALLS
"""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** OFF

This will generate code coverage to be uploaded to Coveralls.
This is meant to be used on a Linux host.

BUILD_OPTIMIZED
"""""""""""""""

**Type:** boolean
:raw-html:`<br />`
**Default:** ON

This will built in release mode with aggressive optimizations.
This should improve server performance but may not work for very
very old systems. It may be useful to disable this when building
for development for better debugging information.

Architecture Overview
---------------------

The project is split into three main servers: lobby, world and
channel. Accounts are tied to a single lobby. This lobby can then
connect to multiple worlds. Each world has characters bound to it
that share the same depository and demon compendium. Each world
has several channels connected to it that can manage different
zones and dungeon instances in the world. There is one main
database for accounts used by the lobby and one world database
for each configured world. These may point to the same or a
different database server. They should never re-use the same
database data as this could violate the assumption that the lobby
or each world has exclusive access to the database. This could
cause odd bugs and data corruption.

In addition to the main server applications there is dream,
libobjgen, libcomp, libtester, a test client, unit test suites
and several utilities. The main library libcomp is linked to all
the servers, dream, the test client and most of the tools. It
provides any common code that is used by at least two of these
applications.

The objgen utility (and the libobjgen library it uses for metadata)
are used to generate C++ classes that wrap data structures. These
are generated for libcomp and all 3 server applications. The
generated classes can load and save to binary, XML and a SQL
database. This works for loading both the binary data format used
by the client and custom data files. The C++ classes can also bind
to the scripting language (squirrel).

For an in-depth discussion of different aspects of the code it's
recommended you look at the `API documentation <https://comphack.github.io>`_.
Most of what you need to know should be in there (or in this guide).

Object Generator (objgen)
-------------------------

The objgen utility takes XML files as input. These XML files
describe the data structures that should be generated. What follows
is the format of this XML and examples of how to use the generated
code. All XML elements are enclosed in the following:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <objgen>
        <!-- Describe your objects here. -->
    </objgen>

Each object described is converted to a C++ class. You may describe
one as follows:

.. code-block:: xml

    <object name="MyObject">
        <!-- Describe your member variables here. -->
    </object>

The object has several properties that you can change:

- *name* - Name of the object. This is required.

- *namespace* - Namespace to place the object in (default is
  "objects")

- *scriptenabled* - Enables generation of squirrel script bindings
  (default is false)

- *location* - Specifies the database ("lobby" or "world") the
  object is stored in (default is "lobby")

- *persistent* - Enables generation of database load/save (default
  is true)

- *baseobject* - Name of the base class or object to inherit from
  (default is "" which uses libcomp::Object)

- *inherited-construction* - When set this will generated a method
  function called InheritedConstructon that takes the object name.
  This is used by the XML loading for a list of base objects. For
  example if you have a list of Action objects but you specify list
  elements as ActionSetNPCState or ActionAddRemoveStatus it will
  construct the proper action before storing in the list. (default
  is false)

At a minimum each member variable has a type and a name:

.. code-block:: xml

    <member type="u8" name="MyVariable"/>

Here is the list of valid types:

- *bit, bool, flag* - these are boolean types

- *u8* - unsigned 8-bit integer

- *u16* - unsigned 16-bit integer

- *u32* - unsigned 32-bit integer

- *u64* - unsigned 64-bit integer

- *s8* - signed 8-bit integer

- *s16* - signed 16-bit integer

- *s32* - signed 32-bit integer

- *s64* - signed 64-bit integer

- *f32, float, single* - 32-bit single precision floating point

- *f64, double* - 64-bit double precision floating point

- *enum* - enumeration

- *string* - a text string

- *ref, pref* - reference to an object or persistent object (this
  isn't stored on disk / XML as that doesn't make sense)

In addition to these types you may use the name of an object
followed by * for a ref/pref to that specific object type.

Here is the common properties for all variables:

- *caps* - this will make the name appear in capital letters instead
  of camel case when generating the API. This is only useful if
  you variable names start with a lower-case letter.

- *default* - the default value for the member variable when the
  object is created (default depends on the type - usually blank,
  false or 0)

- *inherited* - This will not re-create the variable in a child
  class but can be used to override settings like a default value
  (default is false).

- *key* - generate a LoadXByKey function for this member variable

- *unique* - generate a LoadXListByKey instead if this is false
  (default is true)

- *padding* - This will read or write the given number of zero
  bytes after the member function when loading and saving to a
  binary format (default is 0)

Integer and floating point types has the following additional
properties:

- *min* - Minimum value allowed (default is the min of the
  numeric type)

- *max* - Maximum value allowed (default is the max of the
  numeric type)

A reference can have the following additional properties:

- *nulldefault* - The default value for the reference will be
  null instead of an empty default object (default is false)

A string can have the following additional properties:

- *empty* - Allow an empty string as a valid value (default is true)

- *regex* - Regular expression the string must match or blank
  for none (default is "")

- *round* - Round the length of the string to 2 or 4 bytes,
  padding it with zeroes (default is 0)

- *lensz* - Size in bytes of the length integer when saving to
  binary for a dynamic sized string. Can be 1, 2, or 4 (default
  is 4).

- *length* - Size in bytes of a fixed length string. Can't be
  used with lensz or round (default is 0).

- *encoding* - Storage encoding of the string (it's always UTF-8
  in memory). Can be utf8, cp932, or cp1252 (default is utf8)

An enum can have the following additional properties:

- *underlying* - integer type that is used to store the enumeration

An array must specify the number of elements allowed with the
"size" property. Here is an example:

.. code-block:: xml

    <member type="array" name="EquippedItems" size="15">
        <element type="Item*"/>
    </member>

Here is an example of a list (dynamic array):

.. code-block:: xml

    <member type="list" name="StatusEffects">
        <element type="StatusEffect*"/>
    </member>

Here is an example of a set (a set is a list that has unique elements):

.. code-block:: xml

    <member type="set" name="LearnedSkills">
        <element type="u32"/>
    </member>

Here is an example of a map (associative array):

.. code-block:: xml

    <member type="map" name="Materials">
    <key type="u32"/>
    <value type="u16"/>
    </member>

Note that for a set or map you must use an integer or string type
for the element or key type.

Objects are generated using the OBJGEN_XML CMake macro. This
macro generates code using objgen. The arguments must start with
the name of the output variable that will be passed to the
ADD_EXECUTABLE command to ensure the generated files are compiled
and linked to the application or library. The 2nd argument must be
the main xml schema file that includes all other schema files and
structures that code will be generated for. The remaining arguments
will change depending on the extension (or lack of one). Files with
the xml extension will be used as dependencies to the master xml
schema. These are xml schema files that have been declared in an
<include> element. Files that end in cpp or h are source files that
will be generated. Only the source files defined will be generated
despite what structures may be included in the xml schema. Finally,
all other arguments are assumed to be a search path for other xml
schema files that have be listed in an <include> element.

You can instead run the command on you own:

.. code-block:: bash

    comp_objgen -I dir1 -I dir2 -o somefile.h -o somefile.cpp schema.xml moreschema.xml

Here -I is an include directory for XML schema and -o specifies an
output file. You can specify multiple include directories, output
files and XML schema files on a single command line.

Release Process
---------------

The following section describes the process to go through for making
a new release.

Clean Project
^^^^^^^^^^^^^

Go through the project and do the following:

- Make sure any issues tagged for this release are resolved or moved
  to another milestone.

- Ensure the latest build passed.

- Run any applicable test suites on the latest build and make sure
  it passes.

- Clean up the `Server Releases <https://github.com/comphack/comp_hack/projects/1>`_
  project on GitHub.

- Ensure the milestone has no open issues and close it.

Patch Notes
^^^^^^^^^^^

Go through the commits and compile a list of what has changed (new
features, bug fixes, etc). This list will need to be added into the
Debian package (*debian/changelog* file, release entry on GitHub and
the Discord announcement).

Update Documentation
^^^^^^^^^^^^^^^^^^^^

The Doxygen generated API documentation should be downloaded from
the AppVeyor release. The guide should be generated local on a
Linux machine. Once all the documentation is generated, clone the
`comphack.github.io <https://github.com/comphack/comphack.github.io>`_
repository. Remove any existing documentation files and add in the
new files for a new commit. This should update the website.

Update Version Information
^^^^^^^^^^^^^^^^^^^^^^^^^^

Go into the top of the main *CMakeLists.txt* and change the version
to match the release about to be created.

Confirm that the *contrib/sources.sh* zip files match those listed
in *cmake/external.cmake* or the Debian build may fail.

Create a new entry in the Debian package (debchange -i). Make sure
to include the change log items. The format of this file is very
picky. It would be good to do a local test build before pushing
this to the PPA with this command:

.. code-block:: bash

    dpkg-buildpackage -us -uc

When making the commit it's a good idea to describe the commit as
the version intended as we are about to tag and branch it.

Test Packages
^^^^^^^^^^^^^

Test both the Debian packages and the MSI produced by AppVeyor.
If the version information is wrong or something is not working
right, go fix it now.

Tag & Release
^^^^^^^^^^^^^

Go onto GitHub and create a new pre-release. This will create a
new tag. Please use the format from previous releases. This should
be "v" followed by the version with a dash and then the release
name. If this is a service pack or hotfix add another dash and the
appropriate information (for example "-sp1" or "-hotfix1"). Title
the release as shown in previous releases and include the change
log. *Make sure the release points to the right branch and commit.*
It's especially important that the tag and branch are correct if
last minute changes are made and the release is re-created multiple
times.

After they pass, download the tagged version of the AppVeyor build.
Rename to match (if wrong) before uploading to the release. Include
the Windows zip and msi builds. The documentation build can be
included too but is not required.

Tag the datastore repository to match.

After the PPA is updated as well, download and test the release
build. Once it looks OK set it to an official release.

Update PPA
^^^^^^^^^^

Update the PPA with the new Debian build. Clone the correct version
of the code, download the source to the dependencies and remove the
git history. Upload to the PPA and ensure the build completes and
the packages are published. Test the published packages and make
fixes as needed. Here is an example flow (you may need to change
some things):

.. code-block:: bash

    git clone -b v4.12.1-wyrd-hotfix1 git@github.com:comphack/comp_hack.git
    cd comp_hack/
    git submodule update --init --recursive
    rm -rf .git
    contrib/sources.sh
    debuild -S -sa
    cd ..
    dput ppa:compomega/comphack *.changes

Create Branch
^^^^^^^^^^^^^

First, merge the *develop* branch into *master*. This should be a
clean fast-forward. If this is a major release, create the
appropriately named branch off of *develop*. Bug fixes and other
changes for a service pack or hot fixes should go into this branch
from now on.

Go to the *develop* branch and change the version back to unstable
with a new commit. This way nobody forgets to change it and makes
test builds as a release!

Announce Release
^^^^^^^^^^^^^^^^

Announce the release on Discord and wherever else is appropriate.
Be sure to include the change log of cool new features to get
everyone excited. Party hard.
