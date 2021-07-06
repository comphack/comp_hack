# COMP\_hack #

[![AGPL License](http://img.shields.io/badge/license-AGPL-brightgreen.svg)](https://opensource.org/licenses/AGPL-3.0)
[![Latest Release](https://img.shields.io/github/downloads/comphack/comp_hack/v4.12.2-wyrd-hotfix2/total.svg)](https://github.com/comphack/comp_hack/releases/tag/v4.12.2-wyrd-hotfix2)
[![Discord Chat](https://img.shields.io/discord/322024695266541579.svg?label=&logo=discord&logoColor=ffffff&color=7389D8&labelColor=6A7EC2)](http://discord.gg/9jXeKcJ)

[![Build Status](https://travis-ci.com/comphack/comp_hack.svg?branch=develop)](https://travis-ci.com/comphack/comp_hack)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/9671/badge.svg)](https://scan.coverity.com/projects/comphack-comp_hack)
[![Coverage Status](https://coveralls.io/repos/github/comphack/comp_hack/badge.svg?branch=develop)](https://coveralls.io/github/comphack/comp_hack?branch=develop)
[![Documentation Status](https://readthedocs.org/projects/comp-hack/badge/?version=latest)](https://comp-hack.readthedocs.io/en/latest/?badge=latest)

## Shin Megami Tensei IMAGINE (真・女神転生IMAGINE) Private Server ##

The is server software to revive an MMO that has been shutdown (SMT: IMAGINE). It's a complete re-implementation of the server from scratch and fully open source. The best place for documentation is the [Definitive Guide](https://comp-hack.readthedocs.io/en/latest/) so be sure to check it out. If you need additional support or have a question, stop by the [Discord](http://discord.gg/9jXeKcJ) server (be sure to read the rules channel to get to the community discussion) or start a [GitHub discussion](https://github.com/comphack/comp_hack/discussions).

### Building on Linux ###

You only need to build the project if you are on a Linux system that doesn't have a package (there is a [PPA](https://launchpad.net/~compomega/+archive/ubuntu/comphack) for Ubuntu-based systems) or you want to contribute. That being said, if you wish to contribute, Linux is the preferred build and run environment for the server. Of course you can build and develop with Visual Studio 2015 on Windows if that's your thing.

#### Dependencies ####

First thing you want to do is download some dependencies. Make sure you have GCC 5+ or Clang with C++14 support. Here is a command for Debian/Ubuntu based distros to pull in packages you may need:
```
sudo apt-get install build-essential cmake docbook-xsl doxygen texlive-font-utils xmlto libqt5webkit5-dev
```

#### Building ####

> Make sure to initialize and update the submodules before trying to build!

That should be all you need. Just build the project now:
```
mkdir build
cd build
cmake -DNO_WARNINGS=ON ..
make
```

See the [Definitive Guide](https://comp-hack.readthedocs.io/en/latest/chapters/hacking.html#build-system) for more information on the build system options and how to setup the server.

### Building on Windows ###

If you do not wish to contribute to the project, download from the [Releases](https://github.com/comphack/comp_hack/releases) section or download the nightly artifact off the [AppVeyor](https://ci.appveyor.com/project/compomega/comp-hack/history) page.

#### Required Dependencies ####

* [Visual Studio 2015](https://visualstudio.microsoft.com/vs/older-downloads/)
* [CMake](https://cmake.org)
* [Qt 5.7+](https://www.qt.io)

#### Optional Dependencies ####

* [Doxygen](http://www.doxygen.nl)
* [WiX Toolset](http://wixtoolset.org)

#### Building ####

> Make sure to initialize and update the submodules before trying to build!

Edit the _vsbuild_x86.bat_ and _vsbuild_x64.bat_ batch files to point to your install of Qt. Run the desired script and you should see a _build32_ or _build64_ folder. Inside the folder should be a comp_hack.sln solution file. Open the solution and build as normal.

See the [Definitive Guide](https://comp-hack.readthedocs.io/en/latest/chapters/hacking.html#build-system) for more information on the build system options and how to setup the server.
