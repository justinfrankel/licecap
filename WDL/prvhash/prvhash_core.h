/**
 * prvhash_core.h version 3.3
 *
 * The inclusion file for the "prvhash_core64", "prvhash_core32",
 * "prvhash_core16", "prvhash_core8", "prvhash_core4", "prvhash_core2" PRVHASH
 * core functions for various state variable sizes.
 *
 * Description is available at https://github.com/avaneev/prvhash
 *
 * License
 *
 * Copyright (c) 2020-2021 Aleksey Vaneev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef PRVHASH_CORE_INCLUDED
#define PRVHASH_CORE_INCLUDED

#include <stdint.h>

/**
 * This function runs a single PRVHASH random number generation round. This
 * function can be used both as a hash generator and as a general-purpose
 * random number generator. In the latter case, it is advisable to initially
 * run this function 5 times before using its random output, to neutralize any
 * possible oddities of "Seed"'s and "lcg"'s initial values.
 *
 * To generate hashes, the "lcg" variable should be XORed with entropy input
 * prior to calling this function.
 *
 * @param[in,out] Seed0 The current "Seed" value. Can be initialized to any
 * value.
 * @param[in,out] lcg0 The current "lcg" value. Can be initialized to any
 * value.
 * @param[in,out] Hash0 Current hash word in a hash word array.
 * @return Current random value.
 */

inline uint64_t prvhash_core64( uint64_t* const Seed0, uint64_t* const lcg0,
	uint64_t* const Hash0 )
{
	uint64_t Seed = *Seed0; uint64_t lcg = *lcg0; uint64_t Hash = *Hash0;

	const uint64_t plcg = lcg;
	const uint64_t mx = Seed * ( lcg - ~lcg );
	const uint64_t rs = mx >> 32 | mx << 32;
	lcg += ~mx;
	Hash += rs;
	Seed = Hash ^ plcg;
	const uint64_t out = lcg ^ rs;

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

inline uint32_t prvhash_core32( uint32_t* const Seed0, uint32_t* const lcg0,
	uint32_t* const Hash0 )
{
	uint32_t Seed = *Seed0; uint32_t lcg = *lcg0; uint32_t Hash = *Hash0;

	const uint32_t plcg = lcg;
	const uint32_t mx = Seed * ( lcg - ~lcg );
	const uint32_t rs = mx >> 16 | mx << 16;
	lcg += ~mx;
	Hash += rs;
	Seed = Hash ^ plcg;
	const uint32_t out = lcg ^ rs;

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

inline uint16_t prvhash_core16( uint16_t* const Seed0, uint16_t* const lcg0,
	uint16_t* const Hash0 )
{
	uint16_t Seed = *Seed0; uint16_t lcg = *lcg0; uint16_t Hash = *Hash0;

	const uint16_t plcg = lcg;
	const uint16_t mx = (uint16_t) ( Seed * ( lcg - ~lcg ));
	const uint16_t rs = (uint16_t) ( mx >> 8 | mx << 8 );
	lcg += (uint16_t) ~mx;
	Hash += rs;
	Seed = (uint16_t) ( Hash ^ plcg );
	const uint16_t out = (uint16_t) ( lcg ^ rs );

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

inline uint8_t prvhash_core8( uint8_t* const Seed0, uint8_t* const lcg0,
	uint8_t* const Hash0 )
{
	uint8_t Seed = *Seed0; uint8_t lcg = *lcg0; uint8_t Hash = *Hash0;

	const uint8_t plcg = lcg;
	const uint8_t mx = (uint8_t) ( Seed * ( lcg - ~lcg ));
	const uint8_t rs = (uint8_t) ( mx >> 4 | mx << 4 );
	lcg += (uint8_t) ~mx;
	Hash += rs;
	Seed = (uint8_t) ( Hash ^ plcg );
	const uint8_t out = (uint8_t) ( lcg ^ rs );

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

inline uint8_t prvhash_core4( uint8_t* const Seed0, uint8_t* const lcg0,
	uint8_t* const Hash0 )
{
	uint8_t Seed = *Seed0; uint8_t lcg = *lcg0; uint8_t Hash = *Hash0;

	const uint8_t plcg = lcg;
	const uint8_t mx = (uint8_t) (( Seed * ( lcg - ~lcg )) & 15 );
	const uint8_t rs = (uint8_t) (( mx >> 2 | mx << 2 ) & 15 );
	lcg += (uint8_t) ~mx;
	lcg &= 15;
	Hash += rs;
	Hash &= 15;
	Seed = (uint8_t) ( Hash ^ plcg );
	const uint8_t out = (uint8_t) ( lcg ^ rs );

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

inline uint8_t prvhash_core2( uint8_t* const Seed0, uint8_t* const lcg0,
	uint8_t* const Hash0 )
{
	uint8_t Seed = *Seed0; uint8_t lcg = *lcg0; uint8_t Hash = *Hash0;

	const uint8_t plcg = lcg;
	const uint8_t mx = (uint8_t) (( Seed * ( lcg - ~lcg )) & 3 );
	const uint8_t rs = (uint8_t) (( mx >> 1 | mx << 1 ) & 3 );
	lcg += (uint8_t) ~mx;
	lcg &= 3;
	Hash += rs;
	Hash &= 3;
	Seed = (uint8_t) ( Hash ^ plcg );
	const uint8_t out = (uint8_t) ( lcg ^ rs );

	*Seed0 = Seed; *lcg0 = lcg; *Hash0 = Hash;

	return( out );
}

#endif // PRVHASH_CORE_INCLUDED
