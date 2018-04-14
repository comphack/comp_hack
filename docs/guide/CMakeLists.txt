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

SET(XMLTO_FOUND false)

FIND_PROGRAM(XMLTO_EXECUTABLE NAMES xmlto
	DOC "path to the xmlto docbook xslt frontend")

IF(XMLTO_EXECUTABLE)
	SET(XMLTO_FOUND true)
ENDIF(XMLTO_EXECUTABLE)

IF(NOT XMLTO_FIND_QUIETLY AND NOT XMLTO_FOUND)
	IF(XMLTO_FIND_REQUIRED)
		FATAL_ERROR( "xmlto not found")
	ELSE(XMLTO_FIND_REQUIRED)
		MESSAGE( "xmlto not found")
	ENDIF(XMLTO_FIND_REQUIRED)
ENDIF(NOT XMLTO_FIND_QUIETLY AND NOT XMLTO_FOUND)

MACRO(XMLTO_XHTML imgdir fragment out mode in)
	IF(XMLTO_FOUND)
		FILE(GLOB image_files ${imgdir}/*)
		FOREACH(it ${image_files})
			GET_FILENAME_COMPONENT(iname ${it} NAME)
			SET(ipath static/${iname})
			ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/${out}/${ipath}
				COMMAND ${CMAKE_COMMAND} -E copy ${it}
				${CMAKE_BINARY_DIR}/${out}/${ipath} DEPENDS ${it}
			COMMENT "Copying docbook image ${iname}")
			SET(imgdeps ${CMAKE_BINARY_DIR}/${out}/${ipath} ${imgdeps})
		ENDFOREACH(it ${image_files})

		GET_FILENAME_COMPONENT(inpath ${in} PATH)

		ADD_CUSTOM_TARGET(guide-images DEPENDS ${imgdeps} ${imgdir})
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/${out}/index.html
			COMMAND ${XMLTO_EXECUTABLE}
			-m ${CMAKE_CURRENT_SOURCE_DIR}/${fragment}
			-o ${CMAKE_BINARY_DIR}/${out} ${mode}
			${CMAKE_CURRENT_SOURCE_DIR}/${in} DEPENDS
			${CMAKE_CURRENT_SOURCE_DIR}/${fragment}
			${CMAKE_CURRENT_SOURCE_DIR}/${in} ${ARGN} guide-images
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${inpath}
			COMMENT "Generating docbook ${mode} documentation" VERBATIM)
		ADD_CUSTOM_TARGET(guide ALL DEPENDS ${CMAKE_BINARY_DIR}/${out}/index.html)

		FILE(MAKE_DIRECTORY ${out})
	ENDIF(XMLTO_FOUND)
ENDMACRO(XMLTO_XHTML fragment out mode in)

IF(XMLTO_FOUND)
    FILE(GLOB_RECURSE GUIDE_XML_SRCS *.xml)

    XMLTO_XHTML(
        static config.xsl guide xhtml book.xml
        ${GUIDE_XML_SRCS}
    )

IF(XMLTO_FOUND)
    INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/guide
        DESTINATION share/doc/comp_hack)
ENDIF(XMLTO_FOUND)
