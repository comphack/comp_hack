/**
 * @file libcomp/src/PushIgnore.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Include file to ignore warnings.
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

#ifndef GCC_VERSION
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + \
    __GNUC_PATCHLEVEL__)
#endif // GCC_VERSION

// You can only push and pop diagnostic pragmas with new versions of GCC.
#if !defined(_MSC_VER) && GCC_VERSION >= 40600
#pragma GCC diagnostic push
#endif // !defined(_MSC_VER) && GCC_VERSION >= 40600

// When using GCC, push a diagnostic pragma to ignore conversion warnings.
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-variable"

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wunused-local-typedef"
#endif // __clang__

#if GCC_VERSION > 40201
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#endif // GCC_VERSION > 40201
#endif // !defined(_MSC_VER)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4244) // possible loss of data
#pragma warning(disable: 4251) // class A needs to have dll-interface to be used by clients of class B
#pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' (performance warning)
#endif // defined(_MSC_VER)
