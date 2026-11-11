# LEGO Games Freeze Fix

Fixes the periodic freezes and stutters in TT Games' LEGO games on PC. A drop-in `dinput8.dll`:
copy one file next to the game, no game files are modified.

[Leer en español](README.es.md)

---

## The problem

- **Older games** (LEGO Star Wars: The Complete Saga, Batman, Indiana Jones, Harry Potter...): about once a
  minute the game freezes for 10-20 seconds. Music keeps playing and cutscenes go on, but the game does not
  respond.
- **Newer games** (LEGO DC Super-Villains, The LEGO Movie 2): short stutters instead of long freezes.
- It happens on Steam, GOG and Epic, on any drive, on fast PCs too. The more controllers connected, the
  longer the freezes. It is not your PC: it is how the games look for controllers.

[Video](https://youtu.be/3ZfiehEKUzI) (2 min): LEGO Star Wars: The Complete Saga with and without the fix.

## Games

The DLL must match the game's `.exe`: **32-bit** or **64-bit**. The zip has both; this table says which
one each game needs.

- **32-bit .exe / 64-bit .exe**: the game ships that executable.
- **Steam / GOG / Epic**: DLL needed by the `.exe` that store launches by default.
  **–**: not tested on that store (it may not be sold there).

### Tested and fixed

| Game | Year | 32-bit .exe | 64-bit .exe | Steam | GOG | Epic |
|---|---|:-:|:-:|:-:|:-:|:-:|
| LEGO Indiana Jones: The Original Adventures | 2008 | ✓ | | 32 | – | – |
| LEGO Batman: The Videogame | 2008 | ✓ | | 32 | – | 32 |
| LEGO Star Wars: The Complete Saga | 2009 | ✓ | | 32 | 32 | – |
| LEGO Indiana Jones 2: The Adventure Continues | 2009 | ✓ | | 32 | – | – |
| LEGO Star Wars III: The Clone Wars | 2011 | ✓ | | 32 | 32 | – |
| LEGO Pirates of the Caribbean: The Video Game | 2011 | ✓ | | 32 | – | – |
| LEGO Harry Potter: Years 5-7 | 2011 | ✓ | | 32 | – | – |
| LEGO Batman 2: DC Super Heroes | 2012 | ✓ | | 32 | – | 32 |
| LEGO The Lord of the Rings | 2012 | ✓ | | 32 | 32 | – |
| LEGO Marvel Super Heroes | 2013 | ✓ | | 32 | – | – |
| The LEGO Movie Videogame | 2014 | ✓ | | 32 | – | – |
| LEGO The Hobbit | 2014 | ✓ | ✓ | 32 | 32 | – |
| LEGO Batman 3: Beyond Gotham | 2014 | ✓ | ✓ | 64 | – | 64 |
| LEGO Jurassic World | 2015 | ✓ | ✓ | 64 | – | – |
| LEGO Marvel's Avengers | 2016 | ✓ | ✓ | 64 | – | – |
| LEGO Star Wars: The Force Awakens | 2016 | ✓ | ✓ | 64 | – | – |
| LEGO Worlds | 2017 | ✓ | ✓ | 64 | – | – |
| The LEGO NINJAGO Movie Video Game | 2017 | ✓ | ✓ | 64 | – | – |
| LEGO Marvel Super Heroes 2 | 2017 | ✓ | ✓ | 64 | – | – |
| LEGO The Incredibles | 2018 | ✓ | ✓ | 64 | – | – |
| LEGO DC Super-Villains | 2018 | | ✓ | 64 | – | – |
| The LEGO Movie 2 Videogame | 2019 | | ✓ | 64 | – | – |

A game with only one `.exe` always needs that DLL, whatever the store. A game with both may run either one,
so do not guess from the file names (the 64-bit one is often `..._DX11.exe`): use the store column. A DLL with
the wrong bits does not show any error, the game just ignores it. If no `dinput8-proxy.log` appears next to
the game after starting it, it is probably running the other `.exe`: use the other DLL (see "Is it working?").

### Tested, no freezes found

- **LEGO Harry Potter: Years 1-4** (2010): no freezes in our test (it only looked for controllers at startup),
  but they have been seen in this game before. If you get them, use the 32-bit DLL.
- **LEGO Star Wars: The Skywalker Saga** (2022): does not need the DLL.

### Not tested

LEGO Star Wars: The Video Game (2005), LEGO Star Wars II: The Original Trilogy (2006), LEGO City Undercover,
LEGO Harry Potter Collection, LEGO Batman: Legacy of the Dark Knight.

If you try one of these, please [open an issue](https://github.com/pooka-v1/lego-games-freeze-fix/issues)
and tell how it went, with the `dinput8-proxy.log` file (see "Is it working?").

## Install

1. Download `lego-games-freeze-fix-v1.0.zip` from
   [Releases](https://github.com/pooka-v1/lego-games-freeze-fix/releases) (also on
   [Nexus Mods](https://www.nexusmods.com/legostarwarsthecompletesaga/mods/220)) and extract it. It contains:
   ```
   32-bit\dinput8.dll
   64-bit\dinput8.dll
   README.txt
   ```
2. Open the game folder, the one with the game's `.exe`:
   - **Steam**: right-click the game > *Manage* > *Browse local files*.
   - **GOG Galaxy**: game page > *More* icon > *Manage installation* > *Show folder*.
   - **Epic**: right-click the game > *Manage* > *Installation* section > folder icon.
3. Copy the `dinput8.dll` from the folder the table says (`32-bit` or `64-bit`) next to the game's `.exe`.
4. Start the game. A `dinput8-proxy.log` file appears in the same folder: the DLL is working.

Never copy it into `C:\Windows`: it only works from the game folder, and there it would replace Windows' own
`dinput8.dll`.

## Uninstall

Delete `dinput8.dll` and `dinput8-proxy.log` from the game folder.

When you uninstall a game, Steam and GOG leave these two files behind in the game folder (Epic deletes the
whole folder): you can delete them by hand.

## Is it working?

When the game starts, the DLL writes `dinput8-proxy.log` next to the `.exe`.

- **No log file**: the game did not load the DLL. Most likely the bits are wrong (try the other DLL, see
  "Games") or the DLL is not in the same folder as the `.exe` the game runs.
- **Log file**: look for these two lines (each line starts with the date and time):
  ```
  WMI shortcut: installed
  WMI shortcut: first answer (CreateInstanceEnum), 3 XInput device(s), the game waited 1234 ms
  ```
  The second one can say `ExecQuery` instead of `CreateInstanceEnum`: both are fine. The game waits 1-2 s
  once, on the loading screen; after that there are no more freezes.
- `NOT installed`, `unexpected class` or `unexpected query`: this game asks for controllers in a way the DLL
  does not know. Please report it.

If something does not work, [open an issue](https://github.com/pooka-v1/lego-games-freeze-fix/issues) with
the game, the store, and the `dinput8-proxy.log` file attached.

## Notes

- **Steam Input can stay on.** The game still sees Steam's virtual controller as an Xbox controller.
- **Two Xbox controllers outside Steam Input, oldest games only**: the game may mix up the controller names,
  and player 2 shows as "unassigned". Seen in LEGO Star Wars: The Complete Saga and LEGO Indiana Jones: The
  Original Adventures; from LEGO Indiana Jones 2 on, player 2 joined fine in our tests. This is a bug in the
  game itself, it happens without the DLL too. Fix: set up player 2's controls by hand in the game's options,
  or turn Steam Input on.
- **Antivirus warnings.** The DLL is not digitally signed (signing certificates are expensive for a free
  personal project), and it changes how the game talks to Windows, which is the kind of thing antivirus
  programs watch for. A warning is possible and would be a false positive. The source code is all here, you
  can build it yourself (see "Building"), and every release lists the SHA-256 of each DLL. To check yours, in
  PowerShell:
  ```powershell
  Get-FileHash .\dinput8.dll
  ```

## How it works

### The cause

Found with memory dumps taken during the freezes (see "Investigation tools"): the game's main thread was always
stuck in the same place, a **WMI** query (`fastprox!CWbemSvcWrapper::XWbemServices::CreateInstanceEnum`).

The games contain Microsoft's sample code `IsXInputDevice`, which many games of that time copied as is:

1. About once a minute the game lists its input devices with DirectInput (`EnumDevices`): keyboards, mice and
   controllers.
2. **For each device found**, keyboard and mouse included, it asks Windows for the full list of devices
   (`Win32_PnPEntity`) to see if its ID contains `IG_`, which means an Xbox controller.
3. A normal PC has a few hundred devices (motherboard, Bluetooth, audio, network...). Each query takes about
   1.7 s, and with several devices and the PC busy with the game, the wait goes up to 15 s.

Microsoft's own documentation advises against running that query repeatedly.

### The fix

In short: the DLL asks Windows that slow question once, in the background, keeps the answer, and gives it to
the game instantly every time it asks again.

In detail: games load `dinput8.dll` from their own folder before the one in Windows. This DLL:

- loads the real `dinput8.dll` from Windows and passes every call through **unchanged**: the game sees the
  same keyboards, mice and controllers as always;
- installs a **WMI shortcut** (`WmiShortcut.cpp`): in the game's import table it replaces `CoCreateInstance`
  with its own function. When the game asks for WMI, it gets a fake WMI that only answers "give me
  `Win32_PNPEntity`" (asked with `CreateInstanceEnum` or `ExecQuery`), and only with the devices whose ID
  contains `IG_`. Any other `CoCreateInstance` goes to Windows untouched;
- that answer is **the real WMI's, read once** in a background thread when the game starts (`RealWmi.cpp`).
  It has to be the real one because **Steam Input** adds its virtual controller to the WMI answer from inside
  the game's process: without it, the game does not see that controller as an Xbox one;
- **keeps the device lists** (`EnumCache.h`). Even without WMI, each DirectInput `EnumDevices` takes about
  160 ms, and the game asks three in a row. The first search of each type is real; the next ones get the
  saved list instantly;
- **updates the lists when a controller is connected or disconnected**: a background thread reads the
  DirectInput lists, then the real WMI again, and only then hands both to the game, without stopping it.

The optional `dinput8-proxy.ini` (template in this repository) turns parts off to find which one causes a
problem in a given game: `WmiShortcut=0` brings back the long freezes, `EnumCache=0` the short stutters.

## Building

Needs Visual Studio with the *Desktop development with C++* workload.

1. Open `dinput8.slnx` (or `dinput8\dinput8.vcxproj` if your Visual Studio does not open `.slnx` files).
2. Build **Release | x86** → `bin\Win32\Release\dinput8.dll` (32-bit games).
3. Build **Release | x64** → `bin\x64\Release\dinput8.dll` (64-bit games).

The C++ runtime is linked into the DLL (`/MT`), so players do not need the Visual C++ Redistributable.

## Investigation tools

The `tools\` scripts found the cause. They work for any other game that freezes.

| Script | What it does |
|---|---|
| `freeze-probe.ps1` | CPU of the game and of the 3 busiest processes, about every second → `freeze-probe.csv`. |
| `freeze-dump.ps1` | Each **Scroll Lock** press takes 2 memory dumps of the game, 3 s apart → `tools\dumps\`. |
| `analyze-dumps.ps1` | Where the main thread is stuck in each dump (`-AllThreads`: every thread). |

```powershell
powershell -ExecutionPolicy Bypass -File tools\freeze-dump.ps1 -GameName LEGOStarWarsSaga
powershell -ExecutionPolicy Bypass -File tools\analyze-dumps.ps1
```

`-GameName` is the `.exe` name without the extension. They need two Microsoft tools that are not included here
(their license does not allow it):

- `freeze-dump.ps1`: [ProcDump](https://learn.microsoft.com/sysinternals/downloads/procdump), extracted to
  `tools\procdump\`.
- `analyze-dumps.ps1`: WinDbg (`winget install Microsoft.WinDbg`). The first run downloads Microsoft's
  symbols to `tools\symbols\`.

Why Scroll Lock: it does not take the focus away from the game. Any tool that needs a click outside the game
(Process Explorer, Task Manager) is useless with exclusive full screen: the game minimizes and its threads
stop, which changes what you want to measure.

## Disclaimer

This is an unofficial fan project. It is not affiliated with or endorsed by the LEGO Group, TT Games or Warner
Bros. Games. LEGO is a trademark of the LEGO Group. Game names are used only to say which games this fix works
with.

## License

[MIT](LICENSE) © 2026 pooka-v1
