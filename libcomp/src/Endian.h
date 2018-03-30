/**
 * @file libcomp/src/Endian.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Routines to convert integers between big endian and little endian.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_ENDIAN_H
#define LIBCOMP_SRC_ENDIAN_H

#include <stdint.h>

#ifndef _WIN32

// Unix and Linux systems have these functions so just use the system version.
#ifdef __FreeBSD__
#include <sys/endian.h>
#else // Q_OS_UNIX
#include <endian.h>
#endif // Q_OS_UNIX

#else // _WIN32

/// Swap the bytes of a 16-bit integer @em d.
#define swap16 (((d & 0xFF00) >> 8) | \
                ((d & 0x00FF) << 8))

/// Swap the bytes of a 32-bit integer @em d.
#define swap32 (((d & 0xFF000000) >> 24) | \
                ((d & 0x00FF0000) >> 8)  | \
                ((d & 0x0000FF00) << 8)  | \
                ((d & 0x000000FF) << 24))

/// Swap the bytes of a 64-bit integer @em d.
#define swap64 (((d & 0xFF00000000000000ULL) >> 56) | \
                ((d & 0x00FF000000000000ULL) >> 40) | \
                ((d & 0x0000FF0000000000ULL) >> 24) | \
                ((d & 0x000000FF00000000ULL) >> 8)  | \
                ((d & 0x00000000FF000000ULL) << 8)  | \
                ((d & 0x0000000000FF0000ULL) << 24) | \
                ((d & 0x000000000000FF00ULL) << 40) | \
                ((d & 0x00000000000000FFULL) << 56))

/**
 * Convert a 16-bit integer from the host byte order to big endian.
 * @param d Integer in host byte order.
 * @returns Integer in big endian.
 */
static inline uint16_t htobe16(uint16_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap16;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 16-bit integer from the host byte order to little endian.
 * @param d Integer in host byte order.
 * @returns Integer in little endian.
 */
static inline uint16_t htole16(uint16_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap16;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 16-bit integer from big endian to host byte order.
 * @param d Integer in big endian.
 * @returns Integer in host byte order.
 */
static inline uint16_t be16toh(uint16_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap16;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 16-bit integer from little endian to host byte order.
 * @param d Integer in little endian.
 * @returns Integer in host byte order.
 */
static inline uint16_t le16toh(uint16_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap16;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 32-bit integer from the host byte order to big endian.
 * @param d Integer in host byte order.
 * @returns Integer in big endian.
 */
static inline uint32_t htobe32(uint32_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap32;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 32-bit integer from the host byte order to little endian.
 * @param d Integer in host byte order.
 * @returns Integer in little endian.
 */
static inline uint32_t htole32(uint32_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap32;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 32-bit integer from big endian to host byte order.
 * @param d Integer in big endian.
 * @returns Integer in host byte order.
 */
static inline uint32_t be32toh(uint32_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap32;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 32-bit integer from little endian to host byte order.
 * @param d Integer in little endian.
 * @returns Integer in host byte order.
 */
static inline uint32_t le32toh(uint32_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap32;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 64-bit integer from the host byte order to big endian.
 * @param d Integer in host byte order.
 * @returns Integer in big endian.
 */
static inline uint64_t htobe64(uint64_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap64;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 64-bit integer from the host byte order to little endian.
 * @param d Integer in host byte order.
 * @returns Integer in little endian.
 */
static inline uint64_t htole64(uint64_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap64;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 64-bit integer from big endian to host byte order.
 * @param d Integer in big endian.
 * @returns Integer in host byte order.
 */
static inline uint64_t be64toh(uint64_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return swap64;
#else // LIBCOMP_BIGENDIAN
    return d;
#endif // LIBCOMP_LITTLEENDIAN
}

/**
 * Convert a 64-bit integer from little endian to host byte order.
 * @param d Integer in little endian.
 * @returns Integer in host byte order.
 */
static inline uint64_t le64toh(uint64_t d)
{
#ifdef LIBCOMP_LITTLEENDIAN
    return d;
#else // LIBCOMP_BIGENDIAN
    return swap64;
#endif // LIBCOMP_LITTLEENDIAN
}

#endif // _WIN32

#endif // LIBCOMP_SRC_ENDIAN_H
