"""Keep the app-local launcher's policy equal to qualified canonical TPLAY."""
def validate(here):
    canonical=(here.parent/'LAUNCH/TPLAY.C').read_bytes().replace(b'\r\n',b'\n')
    marker=b'#define PLEN 128\n'
    client=b'test?"RAWGUI.EXE":"MINIPLAY.EXE"'
    if canonical.count(marker)!=1 or canonical.count(client)!=1:
        raise ValueError('Canonical launcher changed: review Effects launcher binding')
    expected=canonical.replace(marker,marker+b'#ifndef TP_PLAYER\n#define TP_PLAYER "MINIPLAY.EXE"\n#endif\n').replace(client,b'test?"RAWGUI.EXE":TP_PLAYER')
    if (here/'TPLAY.C').read_bytes().replace(b'\r\n',b'\n')!=expected:
        raise ValueError('Effects launcher differs beyond the reviewed client selector')
