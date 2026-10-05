# -*- coding: utf-8 -*-
# Packs a folder of trophy files into an unencrypted PS Vita TROPHY.TRP
# (the format read by Rinnegatamante's NoTrpDrm). Format reverse-engineered
# from a real .TRP:
#   Header (0x40): magic 'DC A2 4D 00' | version(BE u32=2) | filesize(BE u64) |
#                  count(BE u32) | entrysize(BE u32=0x40) | devflag(BE u32=0) |
#                  sha1(20) | pad(16)
#   Entry (0x40):  name[32] | offset(BE u64) | size(BE u64) | flag(BE u32=0) | pad(12)
#   Files follow, each aligned to 16 bytes.
#   sha1 = SHA1 of the whole file with the 20-byte hash field zeroed.
import os, sys, struct, hashlib

def build(folder, out_path):
    # canonical order: TROPCONF.SFM, TROP.SFM, ICON0.PNG, then TROP###.PNG
    # The SFM files must keep a 320-byte Sce-Np-Trophy-Signature placeholder.
    names = ["TROPCONF.SFM", "TROP.SFM", "ICON0.PNG"]
    trops = sorted(f for f in os.listdir(folder) if f.upper().startswith("TROP") and f.upper().endswith(".PNG"))
    names += trops
    files = []
    for n in names:
        p = os.path.join(folder, n)
        if not os.path.exists(p):
            print("  MISSING: " + n); continue
        files.append((n, open(p, "rb").read()))

    count = len(files)
    data_start = 0x40 + count * 0x40

    # assign 16-byte aligned offsets
    entries = []
    pos = data_start
    blob = bytearray()
    for n, data in files:
        if (data_start + len(blob)) % 16 != 0:
            pad = 16 - ((data_start + len(blob)) % 16)
            blob += b"\x00" * pad
        offset = data_start + len(blob)
        entries.append((n, offset, len(data)))
        blob += data

    filesize = data_start + len(blob)

    # header (hash zeroed for now)
    hdr = bytearray(0x40)
    hdr[0:4] = b"\xDC\xA2\x4D\x00"
    struct.pack_into(">I", hdr, 0x04, 2)            # version
    struct.pack_into(">Q", hdr, 0x08, filesize)     # file size
    struct.pack_into(">I", hdr, 0x10, count)        # file count
    struct.pack_into(">I", hdr, 0x14, 0x40)         # entry size
    struct.pack_into(">I", hdr, 0x18, 0)            # dev flag
    # 0x1C..0x2F sha1 (zero for now), 0x30..0x3F pad

    ent = bytearray()
    for n, offset, size in entries:
        e = bytearray(0x40)
        nb = n.encode("ascii")
        e[0:len(nb)] = nb
        struct.pack_into(">Q", e, 0x20, offset)
        struct.pack_into(">Q", e, 0x28, size)
        struct.pack_into(">I", e, 0x30, 0)          # flag (0 = plain)
        ent += e

    out = bytes(hdr) + bytes(ent) + bytes(blob)
    assert len(out) == filesize, (len(out), filesize)

    # sha1 of whole file with hash field zeroed (already zero), write it in
    digest = hashlib.sha1(out).digest()
    out = bytearray(out)
    out[0x1C:0x30] = digest
    open(out_path, "wb").write(out)

    print("  Built: %s" % out_path)
    print("  %d files, %d bytes, sha1=%s" % (count, filesize, digest.hex()))
    for n, offset, size in entries:
        print("    %-16s @ 0x%X  %d bytes" % (n, offset, size))

def _trophy_dir():
    # Default trophy source folder: <project_root>/Trophy/void_stranger.
    d = os.path.dirname(os.path.abspath(__file__))
    while d != os.path.dirname(d):
        if os.path.isdir(os.path.join(d, "Trophy", "void_stranger")):
            return os.path.join(d, "Trophy", "void_stranger")
        d = os.path.dirname(d)
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), "void_stranger")

if __name__ == "__main__":
    default_folder = _trophy_dir()
    folder = sys.argv[1] if len(sys.argv) > 1 else default_folder
    out = sys.argv[2] if len(sys.argv) > 2 else os.path.join(default_folder, "TROPHY.TRP")
    build(folder, out)
