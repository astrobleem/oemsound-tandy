/* Deterministic host-only API/fuzz test. Build with sanitizers, see test_fuzz.sh.
 * Reuses no parser logic. Random mutation rejection itself is not the oracle;
 * memory safety, bounded termination and consistent validated replay are.
 */
#include <stdio.h>
#include <string.h>
#include "SMF.H"

static unsigned char data[SMF_MAX_FILE + 1U];
static unsigned char seed[SMF_MAX_FILE + 1U];
static SMF_ULONG state = (SMF_ULONG)0x87658088UL;

static SMF_ULONG random32(void)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static int test_one(unsigned size)
{
    SMF_EVENT event;
    SMF_ULONG hash;
    SMF_ULONG saved_hash;
    SMF_ULONG last_ms;
    SMF_ULONG last_tick;
    unsigned count;
    unsigned pass;
    int rc;

    if (!smf_open(data, size))
        return smf_next(&event) == -1;
    saved_hash = 0;
    for (pass = 0; pass < 2U; ++pass) {
        hash = 0;
        count = 0;
        last_ms = 0;
        last_tick = 0;
        while ((rc = smf_next(&event)) > 0) {
            if (event.ms < last_ms || event.tick < last_tick ||
                event.ms > smf_duration_ms() || count >= SMF_MAX_EVENTS)
                return 0;
            last_ms = event.ms;
            last_tick = event.tick;
            hash = (hash << 1) ^ event.ms ^ event.tick;
            hash ^= ((SMF_ULONG)event.status << 24) |
                    ((SMF_ULONG)event.data1 << 16) |
                    ((SMF_ULONG)event.data2 << 8) |
                    (SMF_ULONG)event.track;
            hash ^= (SMF_ULONG)event.length;
            ++count;
        }
        if (rc != 0 || count != smf_event_count() || smf_next(&event) != 0)
            return 0;
        if (pass == 0)
            saved_hash = hash;
        else if (hash != saved_hash)
            return 0;
        if (!smf_rewind())
            return 0;
    }
    if (smf_next(0) != -1 || !smf_rewind())
        return 0;
    smf_close();
    return smf_next(&event) == -1;
}

int main(int argc, char **argv)
{
    FILE *file;
    unsigned size;
    unsigned seed_size;
    unsigned i;
    unsigned trial;
    unsigned mutations;
    unsigned position;

    if (argc != 2) {
        fprintf(stderr, "Usage: fuzztest seed.mid\n");
        return 2;
    }
    file = fopen(argv[1], "rb");
    if (file == 0)
        return 2;
    seed_size = (unsigned)fread(seed, 1, sizeof(seed), file);
    fclose(file);
    if (seed_size == 0 || seed_size > SMF_MAX_FILE)
        return 2;
    if (smf_open(0, 0) != 0 || smf_rewind() != 0)
        return 3;
    memcpy(data, seed, seed_size);
    if (!test_one(seed_size))
        return 3;
    /* Random full buffers, truncated mutated seeds, and bytewise mutations.
     * The full-file-size cases exercise limits independently of valid chunks.
     */
    for (trial = 0; trial < 100000U; ++trial) {
        if (trial % 4U == 0) {
            size = (unsigned)(random32() % (SMF_ULONG)(SMF_MAX_FILE + 2U));
            for (i = 0; i < size; ++i)
                data[i] = (unsigned char)random32();
        } else {
            memcpy(data, seed, seed_size);
            mutations = 1U + (unsigned)(random32() % 12U);
            for (i = 0; i < mutations; ++i) {
                position = (unsigned)(random32() % (SMF_ULONG)seed_size);
                data[position] = (unsigned char)random32();
            }
            size = seed_size;
            if (trial % 4U == 1U)
                size = (unsigned)(random32() % (SMF_ULONG)(seed_size + 1U));
        }
        if (!test_one(size)) {
            fprintf(stderr, "FAIL trial %u size %u\n", trial, size);
            return 1;
        }
    }
    printf("PASS: 100000 fuzz cases and API/replay invariants\n");
    return 0;
}
