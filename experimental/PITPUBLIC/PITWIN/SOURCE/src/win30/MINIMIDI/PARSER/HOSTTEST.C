/* Portable command-line test harness. Build with -DSMF_HOST -std=c89. */
#include <stdio.h>
#include <stdlib.h>
#include "SMF.H"

static unsigned char bytes[SMF_MAX_FILE + 1U];

int main(int argc, char **argv)
{
    FILE *input;
    size_t count;
    unsigned emitted;
    int rc;
    SMF_EVENT event;

    if (argc != 2) {
        fprintf(stderr, "Usage: hosttest file.mid\n");
        return 2;
    }
    input = fopen(argv[1], "rb");
    if (input == 0) {
        perror(argv[1]);
        return 2;
    }
    count = fread(bytes, 1, sizeof(bytes), input);
    if (ferror(input)) {
        fclose(input);
        return 2;
    }
    fclose(input);
    if (!smf_open(bytes, (unsigned)count)) {
        printf("ERROR %s\n", smf_error());
        return 1;
    }
    printf("OK tracks=%u ppqn=%u duration_ms=%lu channels=%u\n",
           smf_track_count(), smf_division(),
           (unsigned long)smf_duration_ms(), smf_event_count());
    emitted = 0;
    while ((rc = smf_next(&event)) > 0) {
        printf("%lu %lu %u %02X %u %u %u\n",
               (unsigned long)event.ms, (unsigned long)event.tick,
               (unsigned)event.track, (unsigned)event.status,
               (unsigned)event.data1, (unsigned)event.data2,
               (unsigned)event.length);
        ++emitted;
    }
    if (rc < 0) {
        printf("ERROR %s\n", smf_error());
        return 1;
    }
    if (emitted != smf_event_count() || smf_next(&event) != 0)
        return 3;
    /* Test a complete second replay and repeatable EOF. */
    if (!smf_rewind())
        return 3;
    count = 0;
    while ((rc = smf_next(&event)) > 0)
        ++count;
    if (rc != 0 || count != emitted)
        return 3;
    smf_close();
    if (smf_next(&event) != -1)
        return 3;
    return 0;
}
