# Autobuild

How LumaVAULT's nightly build (bigguy, 23:00 to 06:00) builds this repository. The form is in lumavault-server
`docs/AUTOBUILD.md`.

## gfamigakicad

KiCad on the open9 stove in one long run: scripts/nightly.sh runs build-amiga.sh with the night's minutes as its
limit, instead of the 840 s kitchen slices that kill KiCad's compiles in flight. build-amiga.sh keeps its work in its
own cache, so a night that runs out of time carries on the next night. Nothing is published: the .lha goes to the
warehouse's Nightly folder only.

- frequency: nightly
- kind: app
- script: scripts/nightly.sh
- args: 360
- keep: out/*.lha
- minutes: 360
