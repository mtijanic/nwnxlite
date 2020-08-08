# nwnxlite
Windows MySQL connector for NWN:EE

# Supported versions

nwnxlite supports only NWN:EE versions up to 8186 (the last 32bit build). [Build 8193.14] added support for sqlite to the base game. As such, I recommend migrating away from mysql+nwnxlite to just the base game. It should cover all usecases nwnxlite covers.

# Quick start guide:

- Copy everything from bin/dist/ into the folder where your nwserver.exe resides
- Copy nwnxlite.ini into that folder, and edit the settings
- Add nwnxlite.nss to your module. The API is compatible with the aps_include.nss available for NWN 1.69

## Updating

When a new version of NWN:EE is released, the bin/dist/offests.txt file is updated. Simply overwrite your old version with the new one when you update nwserver.exe

## VC redistributable

The MSVC redistributable is required to run nwnxlite. If not already installed on your machine, you can download it here:  https://support.microsoft.com/en-us/help/2977003/the-latest-supported-visual-c-downloads

Get the x86 version, even if your machine is 64bit.

# Support

Best way to get help is to ask on the NWNX discord: https://discord.gg/hxTt8Fr

For bug reports, file an issue on this project. There will be no further feature development, just bugfixing.

[Build 8193.14]: https://steamcommunity.com/games/704450/announcements/detail/2724067792533337541
