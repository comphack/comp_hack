/**
 * @file libcomp/src/PopIgnore.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Include file to restore warnings that were ignored.
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

// You can only push and pop diagnostic pragmas with new versions of GCC.
#if !defined(_MSC_VER) && GCC_VERSION >= 40600
#pragma GCC diagnostic pop
#endif // !defined(_MSC_VER) && GCC_VERSION >= 40600

#if defined(_MSC_VER)
#pragma warning(pop)
#endif // defined(_MSC_VER)
