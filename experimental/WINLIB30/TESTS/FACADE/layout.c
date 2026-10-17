#include <stddef.h>
#include "MDAPI.H"
#include "TIMECLK.H"
#include "MCIAPI.H"
#include "RAWFILE.H"
#define CHECK_SIZE(name, type, value) typedef char name[(sizeof(type)==(value))?1:-1]
#define CHECK_OFFSET(name, type, field, value) typedef char name[(offsetof(type,field)==(value))?1:-1]
CHECK_SIZE(word_is_16_bits,WORD,2);
CHECK_SIZE(dword_is_32_bits,DWORD,4);
CHECK_SIZE(mdinfo_is_40_bytes,MDINFO,40);
CHECK_SIZE(tcframe_is_16_bytes,TCFRAME,16);
CHECK_OFFSET(mdinfo_generation_offset,MDINFO,generation,12);
CHECK_OFFSET(mdinfo_coalesced_offset,MDINFO,coalesced,36);
CHECK_OFFSET(tcframe_period_offset,TCFRAME,p,4);
CHECK_OFFSET(tcframe_volume_offset,TCFRAME,volume,10);
CHECK_OFFSET(tcframe_mask_offset,TCFRAME,mask,14);
CHECK_SIZE(mcopen_is_18_bytes,MCOPEN,18);
CHECK_SIZE(mcgen_is_4_bytes,MCGEN,4);
CHECK_SIZE(mcplay_is_12_bytes,MCPLAY,12);
CHECK_SIZE(mcstatus_is_16_bytes,MCSTATUS,16);
CHECK_SIZE(mcset_is_12_bytes,MCSET,12);
CHECK_OFFSET(mcopen_type_offset,MCOPEN,type,6);
CHECK_OFFSET(mcopen_element_offset,MCOPEN,element,10);
CHECK_OFFSET(mcopen_alias_offset,MCOPEN,alias,14);
int main(void) { return 0; }
