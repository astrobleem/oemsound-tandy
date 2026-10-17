"""Generate the five test-only fixtures used by RAWGUI.C.

The embedded bytes are first-party synthetic QA data, pinned to the verified
R4 archive and TESTS/NATIVE/FIXTURE-INPUTS.JSON. This generator writes only
into an explicitly named, already-existing empty directory. It does not touch
RUNTIME or require Windows, an emulator, or third-party media.
"""
import hashlib
import sys
from pathlib import Path

FIXTURES = {
    "STRESS.TND": (
        "544e445902040100011832000000142000ff01e00403ff05542000ff01e00403ff05842000ff01e00403ff05c42000ff01e00403ff05842000ff01e00403ff05542000ff01e00403ff05142000ff01e00403ff05842000ff01e00403ff05ff03149000ff01d0f000ff05549000ff01d0f000ff05849000ff01d0f000ff05c49000ff01d0f000ff05849000ff01d0f000ff05549000ff01d0f000ff05149000ff01d0f000ff05849000ff01d0f000ff02139000ff07139000ff07139000ff07139000ff07139000ff07139000ff07139000ff07139000ff0776afff0676afff0676afff0676afff0676afff0676afff0676afff0676afff06",
        "8fa1dade58376d43f138fb83c4db7a40047d437ba6079a8c133d88ac4ff894a7"),
    "FX.TND": (
        "544e445902040100010432000000141037ff07542102ff07842202ff07143304ff07e00404ff07e00503ff07542a84ff07e00f4bff07ff40ff4076ff075aff079fff2f",
        "1670ac4bb02e5e4a61d2f32eaf7178178739fde36035c754aeafd5b3680e2f92"),
    "TYPE0.MID": (
        "4d546864000000060000000100604d54726b0000000c00903c6430803c0030ff2f00",
        "ea10eb7ea2e1310c55581faef0aa021366949444786bf09896a26fa75e26ea1e"),
    "BAD.TND": (
        "544e445902040100010632000000ff00",
        "07e84fa411a34dfe391a56a996ad75dba20c8a7c54ad508bbd67d058a369f6fc"),
    "FAST.TND": (
        "544e445902040100010679000000142000ff01e00403ff05542000ff01e00403ff05842000ff01e00403ff05c42000ff01e00403ff05842000ff01e00403ff05542000ff01e00403ff05142000ff01e00403ff05842000ff01e00403ff05ff03149000ff01d0f000ff05549000ff01d0f000ff05849000ff01d0f000ff05c49000ff01d0f000ff05849000ff01d0f000ff05549000ff01d0f000ff05149000ff01d0f000ff05849000ff01d0f000ff02139000ff07139000ff07139000ff07139000ff07139000ff07139000ff07139000ff07139000ff0776afff0676afff0676afff0676afff0676afff0676afff0676afff0676afff06",
        "7d1f4dfcb5205708069703103eb0ce20d9ddea659fcd7fc0288152830c2d7eea"),
}


def main(argv):
    if len(argv) != 2:
        raise SystemExit("usage: MAKEGUIQA.PY EMPTY_OUTPUT_DIRECTORY")
    out = Path(argv[1])
    if not out.is_dir():
        raise SystemExit("Refusing: output directory must already exist")
    if any(out.iterdir()):
        raise SystemExit("Refusing: output directory is not empty")

    payloads = {}
    for name, (encoded, expected) in FIXTURES.items():
        try:
            data = bytes.fromhex(encoded)
        except ValueError as exc:
            raise SystemExit("Invalid embedded hex for " + name + ": " + str(exc))
        actual = hashlib.sha256(data).hexdigest()
        if actual != expected:
            raise SystemExit("Embedded fixture hash mismatch: " + name)
        payloads[name] = data

    written = []
    try:
        for name, data in payloads.items():
            target = out / name
            with target.open("xb") as stream:
                stream.write(data)
            written.append(target)
    except OSError as exc:
        for target in written:
            try:
                target.unlink()
            except OSError:
                pass
        raise SystemExit("Could not create QA fixture: " + str(exc))

    for name, data in payloads.items():
        if hashlib.sha256((out / name).read_bytes()).hexdigest() != FIXTURES[name][1]:
            raise SystemExit("Written fixture hash mismatch: " + name)
    print("Wrote five verified test-only fixtures to " + str(out))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
