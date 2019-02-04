# This file is part of COMP_hack.
#
# Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

IF(NOT UPDATER_ONLY)
	ADD_SUBDIRECTORY(bdpatch)
	ADD_SUBDIRECTORY(bgmtool)
	ADD_SUBDIRECTORY(capgrep)
	ADD_SUBDIRECTORY(cathedral)
	ADD_SUBDIRECTORY(decrypt)
	ADD_SUBDIRECTORY(encrypt)
	ADD_SUBDIRECTORY(logger)
	ADD_SUBDIRECTORY(nifcrypt)

	ADD_SUBDIRECTORY(patcher)
	ADD_SUBDIRECTORY(rehash)

	IF(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sandman" AND BUILD_DREAM)
	    ADD_SUBDIRECTORY(sandman)
	ENDIF(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sandman" AND BUILD_DREAM)

	IF(NOT WIN32)
		ADD_SUBDIRECTORY(manager)
	ENDIF(NOT WIN32)
ENDIF(NOT UPDATER_ONLY)

ADD_SUBDIRECTORY(updater)
