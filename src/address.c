#include "internal.h"
#include "base58.h"
#include "ccan/ccan/build_assert/build_assert.h"
#include <include/wally_address.h>
#include <include/wally_bip32.h>
#include <include/wally_crypto.h>
#include <include/wally_script.h>

int wally_bip32_key_to_address(const struct ext_key *hdkey, uint32_t flags,
                               uint32_t version, char **output)
{
    unsigned char address[HASH160_LEN + 1];
    int ret;

    if (output)
        *output = NULL;

    if (!hdkey || (version & ~0xff) || (flags & ~0xff) || !output)
        return WALLY_EINVAL;

    if (!(flags & (WALLY_ADDRESS_TYPE_P2PKH | WALLY_ADDRESS_TYPE_P2SH_P2WPKH)))
        return WALLY_EINVAL;

    // Catch known incorrect combinations of address type and version:
    if ((flags & WALLY_ADDRESS_TYPE_P2PKH && version == 0x05) ||
        (flags & WALLY_ADDRESS_TYPE_P2SH_P2WPKH && version == 0x00) ||
        (flags & WALLY_ADDRESS_TYPE_P2PKH && version == 0xC4) ||
        (flags & WALLY_ADDRESS_TYPE_P2SH_P2WPKH && version == 0x6F))
        return WALLY_EINVAL;

    if (flags == WALLY_ADDRESS_TYPE_P2PKH) {
        // pub_key_hash = ripemd160(sha256(pubkey)
        address[0] = (unsigned char) version & 0xff;
        if (wally_hash160(hdkey->pub_key, sizeof(hdkey->pub_key), address + 1, HASH160_LEN) != WALLY_OK)
            return WALLY_EINVAL;
    } else {
        // redeem_script = SegWit version 0 + push(keyhash) = OP_0 + 0x20 + [key_hash]
        // where key_hash = ripemd160(sha256(pubkey))
        unsigned char redeem_script[HASH160_LEN + 2];
        redeem_script[0] = OP_0;
        redeem_script[1] = HASH160_LEN;

        if (wally_hash160(hdkey->pub_key, sizeof(hdkey->pub_key), redeem_script + 2, HASH160_LEN) != WALLY_OK)
            return WALLY_EINVAL;

        // P2SH address = version (e.g. 0x05) + ripemd160(sha256(redeem_script))
        address[0] = (unsigned char) version & 0xff;
        if (wally_hash160(redeem_script, sizeof(redeem_script), address + 1, HASH160_LEN) != WALLY_OK)
            return WALLY_EINVAL;
    }

    ret = wally_base58_from_bytes(address, sizeof(address), BASE58_FLAG_CHECKSUM, output);

    wally_clear(address, sizeof(address));
    return ret;
}
