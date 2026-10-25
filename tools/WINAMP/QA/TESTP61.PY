"""Exercise the corrected emulator readback assertion without I/O."""
from pathlib import Path
import re

source = Path(__file__).with_name('UNIFYQA.C').read_text()
match = re.search(r'^#define QA_P61_STORED (0x[0-9a-f]+)U$', source, re.M)
assert match and int(match[1], 16) == 0x0c
mask = int(match[1], 16)
checks = 0
for stored in range(16):
    for before_status in (0, 0x10, 0x20, 0x30):
        for after_status in (0, 0x10, 0x20, 0x30):
            for gates in (0, 3):
                before = stored | before_status
                after = (stored & 0x0c) | gates | after_status
                assert before & mask == after & mask
                checks += 1
                for unauthorized in (4, 8):
                    assert before & mask != (after ^ unauthorized) & mask
                    checks += 1
assert (0x20 & 0xfc) != (0x10 & 0xfc)
assert (0x20 & mask) == (0x10 & mask)
checks += 2
print(f'PASS {checks} emulated port61 readback comparisons; no I/O')
