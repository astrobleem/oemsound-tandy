# TPLAY Windows XT fix

[Tandy Winamp XT1](https://github.com/astrobleem/oemsound-tandy/releases/tag/tandy-winamp-xt1.20261024)
supports current HPXY and XRSYNC Windows XT profiles through the required16KB
video reservation and includes the exact helper and paired TSOUND driver.

[Install guide](../experimental/WINAMP30/INSTALL.TXT),
[explicit policy and build/tests](../experimental/WINAMP30/LAUNCH/POLICY.MD),
[exact native results](../experimental/WINAMP30/LAUNCH/TESTS.JSON).

The rebuilt launcher passes49 host policy checks and31 native preflight cases.
Actual XRSYNC playback during window movement/repaint, stop, sound release,
normal Windows/bridge cleanup and fresh reservation pass. Earlier HPXY/CGA,
helper reuse and failed-start successes retain their original runtime identities.
Exact package/source/runtime checks run in CI; native receipts record actual
installed DOSBox-X/Windows3.0 real-mode execution. Existing releases stay intact.
