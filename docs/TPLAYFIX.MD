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
