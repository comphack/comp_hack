#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Script to generate data used for conversion between Unicode and Windows code
# pages 932 and 1252.
#
# This file is a contribution to COMP_hack.
#
# Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

import urllib2
import re

# Whether to use normal Unicode mapping or "best fit".
USE_BESTFIT = True

# Generate the URLs to download the Unicode mapping files from.
URL_BASE = "http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/"
URL_CP932 = URL_BASE + ("WindowsBestFit/bestfit932.txt" \
	if USE_BESTFIT else "WINDOWS/CP932.TXT")
URL_CP1252 = URL_BASE + ("WindowsBestFit/bestfit1252.txt" \
	if USE_BESTFIT else "WINDOWS/CP1252.TXT")

# This should appear at the beginning of the file. All instances of %CP% will
# be replaced with the code page number.
FILE_HEADER = """/**
 * @file libcomp/src/LookupTableCP%CP%.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Data used for conversion between Unicode and Windows code page %CP%.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBCOMP_SRC_DECRYPT_LOOKUPTABLECP%CP%_H
#define LIBCOMP_SRC_DECRYPT_LOOKUPTABLECP%CP%_H

/// Table that contains data needed to convert between Unicode
/// and Windows code page %CP%.
static unsigned char LookupTableCP%CP%[] = {
"""

# This should appear at the end of the file. All instances of %CP% will be
# replaced with the code page number.
FILE_FOOTER = """\n};

#endif // LIBCOMP_SRC_DECRYPT_LOOKUPTABLECP%CP%_H\n"""


# Generate a header file that contains an array of mappings between Unicode and
# Windows code page codepoints. The array is simply a series of 16-bit values
# in little endian byte order. For example, it can be cast to the uint16_t type
# and the index would be the Unicode code point. The returned value would then
# be the Windows code page codepoint. Depending on the code page, this value
# may be 8 bits (like CP1252), 16 bits, or either depending on the value of the
# code point (lookup how CP932 works - it is a lot like ShiftJIS). Note that
# this is Unicode code points and not encodings so UTF-8, UTF-16, etc. need to
# be parsed and converted first. To access the other mapping, Windows code page
# to Unicode codepoints, offset the array by 128KiB or 2^16 16-bit elements and
# cast the array to the uint16_t type again. The resulting header files will be
# written to the current working directory.
# url   - URL of the Unicode mapping file to download and use when generating
#         the array of mapping entries.
# cpnum - Number corresponding to the Windows code page to be used. This value
#         is only used for cosmetic things like the name of the file, comments,
#         and the name of the array.
def generateLookup(url, cpnum):
	# Initialize the lookup arrays to zero values. The arrays have 2^16
	# elements to account for all possible values of a 16-bit code point.
	mappingToUnicode = [0 for i in range(2 ** 16)]
	mappingFromUnicode = [0 for i in range(2 ** 16)]

	# Download the file from the specified URL and process each line looking
	# for mapping values.
	if USE_BESTFIT:
		# Formats for different lines in the mapping.
		cpinfoRegExp = re.compile(r"^CPINFO\s+([0-9]+)\s+0x([0-9a-fA-F]+)" +
			r"\s+0x([0-9a-fA-F]+).*$")
		mbtableRegExp = re.compile(r"^MBTABLE\s+([0-9]+).*$")
		wctableRegExp = re.compile(r"^WCTABLE\s+([0-9]+).*$")
		dbcsrangeRegExp = re.compile(r"^DBCSRANGE\s+([0-9]+).*$")
		dbcstableRegExp = re.compile(r"^DBCSTABLE\s+([0-9]+).*$")
		recordRegExp = re.compile(r"^0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+).*$")

		# 0 - looking for CODEPAGE
		# 1 - looking for CPINFO
		# 2 - looking for a table
		# 3 - parsing a MBTABLE
		# 4 - parsing a DBCSRANGE
		# 5 - looking for a DBCSTABLE
		# 6 - parsing a DBCSTABLE
		# 7 - parsing a WCTABLE
		parseState = 0

		# How many DBCS lead byte ranges are left to parse.
		rangeCount = 0

		# How many codepoint mappings are left to read from the table.
		recordCount = 0

		# How many lead bytes there are for the DBCS range (each has a
		# matching DBCSTABLE).
		leadByteCount = 0

		# Current lead byte being used.
		leadByte = 0x00

		#
		# For a detailed description of the format, see this page: http://www.
		# unicode.org/Public/MAPPINGS/VENDORS/MICSFT/WindowsBestFit/readme.txt
		#
		for line in urllib2.urlopen(url).read().split('\n'):
			# Debug print the current state and the line being parsed.
			# print str(parseState) + ": " + line

			# If the line is a comment, ignore it.
			if len(line) and line[0] == ';':
				continue

			# Parse the line different depending on the current state.
			if parseState == 0 and line.find("CODEPAGE") == 0:  # Initial state
				# We have found where to start parsing, move to the next state.
				parseState = 1
			elif parseState == 1:  # looking for CPINFO
				# Check if the line matches the CPINFO format.
				match = cpinfoRegExp.match(line)
				if not match:
					continue

				# Default characters during conversion.
				cp = int(match.group(2), 16)
				uni = int(match.group(3), 16)

				# Fill the arrays with default characters.
				mappingToUnicode = [uni for i in range(2 ** 16)]
				mappingFromUnicode = [cp for i in range(2 ** 16)]

				# Move to the next parse state.
				parseState = 2
			elif parseState == 2:  # looking for a table
				match = mbtableRegExp.match(line)
				if match:
					parseState = 3  # parsing a MBTABLE
					recordCount = int(match.group(1))
					continue

				match = dbcsrangeRegExp.match(line)
				if match:
					parseState = 4  # parsing a DBCSRANGE
					rangeCount = int(match.group(1))
					continue

				match = wctableRegExp.match(line)
				if match:
					parseState = 7  # parsing a WCTABLE
					recordCount = int(match.group(1))
					continue

				# If we are at the end stop parsing.
				if line.find("ENDCODEPAGE") == 0:
					break
			elif parseState == 3:  # parsing a MBTABLE record (CP=>Uni)
				match = recordRegExp.match(line)
				if not match:
					continue

				# Get the codepoints for this record.
				cp = int(match.group(1), 16)
				uni = int(match.group(2), 16)

				# Record the conversion.
				mappingToUnicode[cp] = uni

				# Remove the record from the count.
				recordCount -= 1

				# If we have read all records, look for another table.
				if recordCount <= 0:
					parseState = 2
			elif parseState == 4:  # parsing a DBCSRANGE (series of DBCSTABLE)
				match = recordRegExp.match(line)
				if not match:
					continue

				# Determine the first lead byte and the number of lead bytes
				# from the first and last lead byte.
				leadByte = int(match.group(1), 16)
				leadByteCount = int(match.group(2), 16) - leadByte + 1

				# Now that we know how many tables there are, start looking for
				# one.
				parseState = 5
			elif parseState == 5:  # looking for a DBCSTABLE
				match = dbcstableRegExp.match(line)
				if not match:
					continue

				parseState = 6  # parsing a DBCSTABLE
				recordCount = int(match.group(1))
			elif parseState == 6:  # parsing a DBCSTABLE
				match = recordRegExp.match(line)
				if not match:
					continue

				# Get the codepoints for this record.
				# Add the lead byte to the codepoint as the higher 8 bits.
				cp = (leadByte << 8) | int(match.group(1), 16)
				uni = int(match.group(2), 16)

				# Record the conversion.
				mappingToUnicode[cp] = uni

				# Remove the record from the count.
				recordCount -= 1

				# If we have read all records, look for another table.
				if recordCount <= 0:
					# Remove the table from the count.
					leadByteCount -= 1

					# Add to the lead byte.
					leadByte += 1

					# If there is no more lead bytes in the range.
					if leadByteCount <= 0:
						# Remove the range from the count.
						rangeCount -= 1

						# If there is another range to parse, do so; otherwise,
						# look for a new table.
						if rangeCount <= 0:
							parseState = 2  # looking for a table
						else:
							parseState = 4  # parsing a DBCSRANGE
					else:
						parseState = 5  # looking for a DBCSTABLE (next lead)
			elif parseState == 7:  # parsing a WCTABLE record (Uni=>CP)
				match = recordRegExp.match(line)
				if not match:
					continue

				# Get the codepoints for this record.
				cp = int(match.group(2), 16)
				uni = int(match.group(1), 16)

				# Record the conversion.
				mappingFromUnicode[uni] = cp

				# Remove the record from the count.
				recordCount -= 1

				# If we have read all records, look for another table.
				if recordCount <= 0:
					parseState = 2
			else:
				# Should never get here, bail.
				break
	else:
		# This is used to match a line in the mapping file. The format is
		# simpy two hex values with white space in between.
		mappingRegExp = re.compile(r"^0x([0-9a-fA-F]+)\s+0x([0-9a-fA-F]+).*$")

		for line in urllib2.urlopen(url).read().split('\n'):
			if len(line) and line[0] == '#':
				continue

			# Attempt to match the line to the code point mapping format.
			match = mappingRegExp.match(line)

			# If the line is in the mapping format, process and add it to the
			# lookup arrays.
			if match:
				# The first hex string is the Windows code point and the second is
				# the Unicode code point. Convert both hex strings into integers.
				cp = int(match.group(1), 16)
				uni = int(match.group(2), 16)

				# Add the relationship to the lookup tables.
				mappingToUnicode[cp] = uni
				mappingFromUnicode[uni] = cp

	# Open the output file named LookupTableCP{cpnum}.h where {cpnum} is the
	# code page value passed to the method. After opening the file, write the
	# header replacing all instances of %CP% with the same code page value that
	# was passed into the method. Finally, write a tab to start the first line
	# of the Unicode to Windows code page mapping portion of the array.
	lookupFile = open("LookupTableCP%d.h" % cpnum, "w")
	lookupFile.write(FILE_HEADER.replace("%CP%", str(cpnum)))
	lookupFile.write('\t')

	# Loop over the Unicode to Windows code page mapping array for each of the
	# 2^16 codepoints we intend to convert. Write the two bytes of the matching
	# Windows code page codepoint as two hex bytes in little endian format. If
	# this is the 4th mapping value on a line (which means 8 bytes on the line)
	# then create a new line and indent again; otherwise, add a space.
	for uni in range(2 ** 16):
		cp = mappingFromUnicode[uni]

		lookupFile.write("0x%02x, 0x%02x," % (cp & 0xFF, cp >> 8))

		if (uni % 4) == 3:
			lookupFile.write("\n\t")
		else:
			lookupFile.write(" ")

	# Loop over the Windows code page to Unicode mapping array for each of the
	# 2^16 codepoints we intend to convert. This loop is exactly like the
	# Unicode to Windows code page array above. The conversion code will simply
	# offset 128KiB or 2^16 elements into the array to access this mapping
	# instead of the one above.
	for cp in range(2 ** 16):
		uni = mappingToUnicode[cp]

		lookupFile.write("0x%02x, 0x%02x," % (uni & 0xFF, uni >> 8))

		if (cp % 4) == 3:
			lookupFile.write("\n\t")
		else:
			lookupFile.write(" ")

	# There won't be another line in the array and the last byte should not
	# contain a comma after it so rewind the file by three bytes to remove the
	# characters before writing the footer then write the footer replacing all
	# instances of %CP% with the same code page value that was passed into the
	# method.
	lookupFile.seek(-3, 1)
	lookupFile.write(FILE_FOOTER.replace("%CP%", "932"))

generateLookup(URL_CP932, 932)
generateLookup(URL_CP1252, 1252)
