# Dead Rising 2 & Off the Record visual fixes

This project aims to fix the PC-exclusive visual issues in Dead Rising 2 and Off the Record while being as simple to install as possible, without overwriting any vanilla game files. The ultimate goal is to have visual parity with console, and in particular the Xbox 360 version of the game. There also are some small QoL additions.

### List of global fixes:
- **Gamma correction:** The games lacked gamma correction which resulted in them looking extremely washed out on PC. They will now match the gamma of the Xbox 360 version.
- **Fixed exposure:** The auto exposure was very broken - the luminance that was being calculated was wrong and the delta ratio being passed into the exposure shader was static which made the exposure speed inconsistent. This resulted in the games not only being overexposed on PC but also extremely jarring to play with the constant exposure changes every step you'd take (as in, the exposure would rapidly jump up and down).
- **Fixed MSAA coverage:** A very jarring difference on PC compared to Xbox 360 was the fact that the foliage, and especially the trees, looked terrible (they were aliased and also clearly "cut off"). The reason for that was the fact that MSAA was applying on the vertex edge and that alpha test had a very aggressive value. This has been fixed for both NVIDIA & AMD GPUs by enabling alpha-to-coverage (ATOC and A2M1 respectively) for alpha-tested objects.
- **Fixed shadows**: The shadows on PC had alpha blend enabled while on Xbox 360 they did not, which resulted in PC appearing as if it wasn't casting shadows for small plants, for example some of the plants seen in DR2's main menu were affected by that.
- **Improved cubemaps**: In the PC versions they made the cubemaps slightly higher resolution than Xbox 360, from 64x64 to 128x128 (in an attempt to improve them maybe?). It's a tiny difference that, however, caused a discrepancy with how they looked compared to Xbox 360, because on PC you would see weird shapes that came from the increased details in the cubemaps. The resolution of the cubemaps has been dropped down to 64x64 to match.

### Non-OTR fixes:
- **Fixed motion blur strength**: The motion blur previously had the same issue as the delta ratio for the exposure which resulted in it not being visible at all at high framerates. This was fixed in OTR, although not in the same way.
- **Fixed blur setting**: While playing the game, you might have thought that the distant blur effect from Xbox 360 was removed or that the motion blur doesn't work but the problem, in reality, is that the game was forcefully de-applying the setting on boot, while it was correctly saving and being read from the game's config.

### QoL:
- **Ultrawide support:** As an additional feature, this projects adds 21:9 and 32:9 support to the game - it will no longer reject those aspect ratios, the 3D aspect ratio has been corrected, the 3D to 2D projection has been corrected to fix healthbars and object prompts, the shadow map coverage has been extended (although you might still sometimes see shadows being weird in ultrawide but the problem is significantly less severe) and the resolution of the shadow maps will increase based on your aspect ratio and the HUD itself is limited to a 16:9 scale/position which makes it retain the correct and size and be properly centered.
- **Fixed font scaling**: The game will now have a correct font scale at above 1200p - if you play at a big resolution such as 4K the font will no longer be tiny. This fix also accounts for the Asian versions of the game so the font scale will be corrected with the needed overrides/additional scaling for them. The positioning itself still needs fixing because it will be very slightly off vertically (not really noticeable). 

### Recommendations:
- If you're playing DR2, the UI on PC has terrible compression artifacts, the [HD Frontend Textures](https://www.nexusmods.com/deadrising2/mods/22) mod fixes that.
- If you have an NVIDIA GPU and you're getting weird seams in the character models when using MSAA, it's a driver bug which you can fully avoid with [DXVK](https://github.com/doitsujin/dxvk).

### Installation:
 - Download the latest release and extract the contents of the archive to your DR2/OTR directory, this works for both games. You do not need to rename the shader archives, the games get patched to read whichever one of the two archives is needed based on the game. This is to simplify installing the mod and to also make sure you do not accidentally get rid of the modded shaders if you verify your files.

### Credits

- Shader Help: [miru97](https://github.com/mlleemiles)
- Silent: [ModUtils](https://github.com/CookiePLMonster/ModUtils)
- cursey: [safetyhook](https://github.com/cursey/safetyhook)
- ThirteenAG: [Hooking.Patterns](https://github.com/ThirteenAG/Hooking.Patterns), [Ultimate-ASI-Loader (bundled with releases)](https://github.com/ThirteenAG/Ultimate-ASI-Loader)
