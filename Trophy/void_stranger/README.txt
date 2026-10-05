Void Stranger Vita - PS Vita Trophy Set
=======================================

NP Communication ID: VSTR00001_00
Package version: 01.00
Catalog: 30 entries

OVERVIEW
  Void Stranger Vita v2.0 combines the original game achievement state with
  Vita-specific milestones used by the native runner. The in-game Vita menu
  exposes the same 30-entry catalog and shows local unlock notifications.

EXAMPLES OF VITA-SIDE EVENTS
  - The Long Way      -> watch the long startup movie
  - Cut to the Chase  -> skip the long startup movie
  - First Steps       -> first movement
  - First Descent     -> first room transition
  - Memory Lane       -> open the Memories album
  - Vita settings milestones include palette/tone and Stretch Screen changes

GAME ACHIEVEMENT SYNC
  Steam achievement state such as VS_PENDANT is merged into the local Vita
  trophy state when available.

NOTES
  - This is an unsigned/local homebrew trophy pack intended for NoTrpDrm.
  - Do not synchronize unsigned/local trophy data with PSN.
  - NoTrpDrm compatibility depends on the console firmware/configuration.

BUILD
  python Tools/trophy/build_trp.py Trophy/void_stranger Trophy/void_stranger/TROPHY.TRP

VPK TARGET
  sce_sys/trophy/VSTR00001_00/TROPHY.TRP