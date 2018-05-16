/*
 * This file is part of the TREZOR project, https://trezor.io/
 *
 * Copyright (C) 2014 Pavol Rusnak <stick@satoshilabs.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>

#include "signatures.h"
#include "skycoin_crypto.h"
#include "sha2.h"
#include "bootloader.h"

#define PUBKEYS 5

#if SIGNATURE_PROTECT
static const uint8_t * const pubkey[PUBKEYS] = {
	(const uint8_t *)"\x04\xd5\x71\xb7\xf1\x48\xc5\xe4\x23\x2c\x38\x14\xf7\x77\xd8\xfa\xea\xf1\xa8\x42\x16\xc7\x8d\x56\x9b\x71\x04\x1f\xfc\x76\x8a\x5b\x2d\x81\x0f\xc3\xbb\x13\x4d\xd0\x26\xb5\x7e\x65\x00\x52\x75\xae\xde\xf4\x3e\x15\x5f\x48\xfc\x11\xa3\x2e\xc7\x90\xa9\x33\x12\xbd\x58",
	(const uint8_t *)"\x04\x63\x27\x9c\x0c\x08\x66\xe5\x0c\x05\xc7\x99\xd3\x2b\xd6\xba\xb0\x18\x8b\x6d\xe0\x65\x36\xd1\x10\x9d\x2e\xd9\xce\x76\xcb\x33\x5c\x49\x0e\x55\xae\xe1\x0c\xc9\x01\x21\x51\x32\xe8\x53\x09\x7d\x54\x32\xed\xa0\x6b\x79\x20\x73\xbd\x77\x40\xc9\x4c\xe4\x51\x6c\xb1",
	(const uint8_t *)"\x04\x43\xae\xdb\xb6\xf7\xe7\x1c\x56\x3f\x8e\xd2\xef\x64\xec\x99\x81\x48\x25\x19\xe7\xef\x4f\x4a\xa9\x8b\x27\x85\x4e\x8c\x49\x12\x6d\x49\x56\xd3\x00\xab\x45\xfd\xc3\x4c\xd2\x6b\xc8\x71\x0d\xe0\xa3\x1d\xbd\xf6\xde\x74\x35\xfd\x0b\x49\x2b\xe7\x0a\xc7\x5f\xde\x58",
	(const uint8_t *)"\x04\x87\x7c\x39\xfd\x7c\x62\x23\x7e\x03\x82\x35\xe9\xc0\x75\xda\xb2\x61\x63\x0f\x78\xee\xb8\xed\xb9\x24\x87\x15\x9f\xff\xed\xfd\xf6\x04\x6c\x6f\x8b\x88\x1f\xa4\x07\xc4\xa4\xce\x6c\x28\xde\x0b\x19\xc1\xf4\xe2\x9f\x1f\xcb\xc5\xa5\x8f\xfd\x14\x32\xa3\xe0\x93\x8a",
	(const uint8_t *)"\x04\x73\x84\xc5\x1a\xe8\x1a\xdd\x0a\x52\x3a\xdb\xb1\x86\xc9\x1b\x90\x6f\xfb\x64\xc2\xc7\x65\x80\x2b\xf2\x6d\xbd\x13\xbd\xf1\x2c\x31\x9e\x80\xc2\x21\x3a\x13\x6c\x8e\xe0\x3d\x78\x74\xfd\x22\xb7\x0d\x68\xe7\xde\xe4\x69\xde\xcf\xbb\xb5\x10\xee\x9a\x46\x0c\xda\x45",
};
#endif

#define SIGNATURES 3

int signatures_ok(uint8_t *store_hash)
{
	if (!firmware_present()) return SIG_FAIL; // no firmware present

	const uint32_t codelen = *((const uint32_t *)FLASH_META_CODELEN);
	
	uint8_t hash[32];
	sha256_Raw((const uint8_t *)FLASH_APP_START, codelen, hash);
	if (store_hash) {
		memcpy(store_hash, hash, 32);
	}

#if SIGNATURE_PROTECT

	const uint8_t sigindex1 = *((const uint8_t *)FLASH_META_SIGINDEX1);
	const uint8_t sigindex2 = *((const uint8_t *)FLASH_META_SIGINDEX2);
	const uint8_t sigindex3 = *((const uint8_t *)FLASH_META_SIGINDEX3);

	if (sigindex1 < 1 || sigindex1 > PUBKEYS) return SIG_FAIL; // invalid index
	if (sigindex2 < 1 || sigindex2 > PUBKEYS) return SIG_FAIL; // invalid index
	if (sigindex3 < 1 || sigindex3 > PUBKEYS) return SIG_FAIL; // invalid index

	if (sigindex1 == sigindex2) return SIG_FAIL; // duplicate use
	if (sigindex1 == sigindex3) return SIG_FAIL; // duplicate use
	if (sigindex2 == sigindex3) return SIG_FAIL; // duplicate use

	uint8_t pubkey1[33];
	uint8_t pubkey2[33];
	uint8_t pubkey3[33];
	recover_pubkey_from_signed_message((char*)hash, (const uint8_t *)FLASH_META_SIG1, pubkey1);
	if (0 != memcmp(pubkey1, pubkey[sigindex1 - 1], 33)) // failure
	{
		return SIG_FAIL;
	} 
	recover_pubkey_from_signed_message((char*)hash, (const uint8_t *)FLASH_META_SIG2, pubkey2);
	if (0 != memcmp(pubkey2, pubkey[sigindex2 - 1], 33)) // failure
	{
		return SIG_FAIL;
	} 
	recover_pubkey_from_signed_message((char*)hash, (const uint8_t *)FLASH_META_SIG3, pubkey3);
	if (0 != memcmp(pubkey3, pubkey[sigindex3 - 1], 33)) // failure
	{
		return SIG_FAIL;
	} 
#endif

	return SIG_OK;
}