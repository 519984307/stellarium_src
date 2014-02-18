/*
 * Stellarium
 * Copyright (C) 2013 Guillaume Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

// This file should be included everywhere.  It contains a few preprocessor
// macros that make the code more cross platform.

// Needed so that M_PI get defined,
#define _USE_MATH_DEFINES
// And so that windows.h does not redefine min and max.
#define NOMINMAX
// Prevent window.s from including winsock.h
#define _WINSOCKAPI_

#ifdef _MSC_VER
// disable a few msvc specific warnings
#  pragma warning(disable : 4305) // double to float truncation.
#  pragma warning(disable : 4805) // unsafe mix of int and bool
#endif

// some math functions are not always defined.
#ifndef _GNU_SOURCE

#include <cmath>

static inline double pow10(const double x)
{
	return std::exp(x * 2.3025850930);
}

static inline double trunc(const double x)
{
	return (x < 0 ? std::ceil(x) : std::floor(x));  
}
static inline double round(const double x)
{
	return (x < 0 ? std::ceil(x - 0.5) : std::floor(x + 0.5));  
}
#endif

#endif // _CONFIG_H_
