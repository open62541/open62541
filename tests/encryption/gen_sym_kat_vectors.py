"""Prints the expected values for check_securitypolicy_sym_kat.c.
They come from Python hmac/hashlib and pyca/cryptography, so they are
independent of the open62541 crypto plugins. Usage: python3 gen_sym_kat_vectors.py"""
import hmac, hashlib, sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.ciphers.aead import AESGCM, ChaCha20Poly1305

def fill(n, seed):
    return bytes((seed + 7 * i) & 0xFF for i in range(n))

def p_hash(digest, secret, seed, n):
    out, a = b"", seed
    while len(out) < n:
        a = hmac.new(secret, a, digest).digest()
        out += hmac.new(secret, a + seed, digest).digest()
    return out[:n]

def cbc(key, iv, pt):
    e = Cipher(algorithms.AES(key), modes.CBC(iv)).encryptor()
    return e.update(pt) + e.finalize()

def masked_iv(iv, token, seq):
    b = bytearray(iv)
    for i in range(4):
        b[i] ^= (token >> (8 * i)) & 0xFF
        b[4 + i] ^= (seq >> (8 * i)) & 0xFF
    return bytes(b)

def carr(name, data):
    if not data:
        return f"static const UA_Byte {name}[1] = {{0}}; /* empty */\n"
    rows = []
    for i in range(0, len(data), 12):
        rows.append("    " + ", ".join(f"0x{x:02x}" for x in data[i:i+12]))
    return f"static const UA_Byte {name}[{len(data)}] = {{\n" + ",\n".join(rows) + "\n};\n"

HMAC_LENGTHS = [0, 1, 64, 300]
CIPHER_LENGTHS = [16, 64, 528]
HMAC_SEED, CIPHER_SEED = 9, 11
SECRET_SEED, PSEED_SEED, PHASH_LENGTH = 12, 13, 80

CBC = [  # name, digest, signLen, signSeed, encLen, encSeed, ivSeed, pHash
    ("b256sha256", hashlib.sha256, 32, 1, 32, 2, 3, True),
    ("b128rsa15", hashlib.sha1, 16, 4, 16, 5, 6, True),
    ("p384", hashlib.sha384, 48, 7, 32, 8, 9, False),
]
TOKEN, AAD_SEED, AAD_LEN, PT_SEED, SIGN_MSG_LEN = 7, 30, 8, 33, 40
PT_LENGTHS = [32, 100, 0]
AEAD = [  # name, gcm?, signLen, signSeed, encLen, encSeed, ivSeed
    ("gcm", True, 0, 0, 16, 20, 21),
    ("chachapoly", False, 0, 0, 32, 22, 23),
    ("curve25519", False, 32, 24, 32, 25, 26),
    ("curve448", False, 32, 27, 32, 28, 29),
]

out = []
for name, dg, sl, ss, el, es, ivs, ph in CBC:
    sk, ek, iv = fill(sl, ss), fill(el, es), fill(16, ivs)
    for n in HMAC_LENGTHS:
        out.append(carr(f"{name}_hmac_{n}", hmac.new(sk, fill(n, HMAC_SEED), dg).digest()))
    for n in CIPHER_LENGTHS:
        out.append(carr(f"{name}_cbc_{n}", cbc(ek, iv, fill(n, CIPHER_SEED))))
    if ph:
        out.append(carr(f"{name}_phash", p_hash(dg, fill(32, SECRET_SEED), fill(32, PSEED_SEED), PHASH_LENGTH)))
for name, gcm, sl, ss, el, es, ivs in AEAD:
    ek, iv = fill(el, es), fill(12, ivs)
    signkey = fill(sl, ss) if sl else ek
    mk = (lambda k: AESGCM(k)) if gcm else (lambda k: ChaCha20Poly1305(k))
    for seq in (1, 2, 3):
        nonce = masked_iv(iv, TOKEN, seq)
        pt = fill(PT_LENGTHS[seq - 1], PT_SEED)
        out.append(carr(f"{name}_sealed_{seq}", mk(ek).encrypt(nonce, pt, fill(AAD_LEN, AAD_SEED))))
        out.append(carr(f"{name}_tag_{seq}", mk(signkey).encrypt(nonce, b"", fill(SIGN_MSG_LEN, 31 + seq))))
sys.stdout.write("".join(out))
