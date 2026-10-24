# TPLAY current Windows XT correction

The local TWXT1 candidate now supports current XR1 HPXY with the required
video-memory guard. It bundles the exact first-party RESERVE.COM and source;
users run one DOS launcher. Stock CGA remains the unreserved path. Existing
releases remain unchanged.

[Install](../experimental/WINAMP30/LAUNCH/INSTALL.TXT),
[policy/source/build/tests](../experimental/WINAMP30/LAUNCH/POLICY.MD),
[exact results](../experimental/WINAMP30/LAUNCH/TESTS.JSON).

MSC6/MASM5.10/LINK4 rebuilt the actual launcher. 45 host policy checks,
28 native preflight/refusal cases and both HPXY/CGA playback with graphics
activity, stop, sound release and normal Windows/bridge cleanup passed.
Occupied overlap, unsupported hardware/DOS and a failed Windows start were
also exercised. No published release or installed user profile was changed.

The integration follow-up explicitly accepts XRSYNC.DRV from reviewed PR155
commit e84a17d1562a3f938b5268b8a92fc5e47b176deb through the same helper.
Current 49 host checks and 31 native preflight cases pass. The actual 60,800-byte
candidate loads as XRSYNC.DRV and passes playback during graphics activity,
stop, sound release and normal exit/bridge cleanup. Its driver SHA and loaded
module filename are recorded in LAUNCH/EVIDENCE/XRSYNC.JSON. Other names remain
refused; no display worktree was modified or copied.
