# LEGO Games Freeze Fix

Quita las congelaciones y los tirones periódicos de los juegos LEGO de TT Games en PC. Un `dinput8.dll` que se
copia junto al juego: un solo fichero, sin modificar ningún fichero del juego.

[Read in English](README.md)

---

## El problema

- **Juegos antiguos** (LEGO Star Wars: The Complete Saga, Batman, Indiana Jones, Harry Potter...): más o menos
  cada minuto el juego se congela 10-20 segundos. La música sigue sonando y las cinemáticas avanzan, pero el
  juego no responde.
- **Juegos modernos** (LEGO DC Super-Villains, The LEGO Movie 2): tirones cortos en vez de congelaciones largas.
- Pasa en Steam, GOG y Epic, desde cualquier disco, también en PC potentes. Cuantos más mandos conectados, más
  duran las congelaciones. No es tu PC: es la forma en que los juegos buscan los mandos.

## Juegos

La DLL tiene que tener los mismos bits que el `.exe` del juego: **32** o **64 bits**. El zip trae las dos; esta
tabla dice cuál necesita cada juego.

- **.exe 32 bits / .exe 64 bits**: el juego trae ese ejecutable.
- **Steam / GOG / Epic**: DLL que necesita el `.exe` que lanza esa tienda por defecto.
  **–**: no probado en esa tienda (puede que no se venda allí).

### Probados y arreglados

| Juego | Año | .exe 32 bits | .exe 64 bits | Steam | GOG | Epic |
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

Un juego con un solo `.exe` siempre necesita esa DLL, sea cual sea la tienda. Uno con los dos puede arrancar
cualquiera, así que no te guíes por el nombre del fichero (el de 64 suele ser `..._DX11.exe`): mira la columna
de la tienda. Una DLL con los bits equivocados no da ningún error, el juego simplemente la ignora. Si al arrancar
el juego no aparece `dinput8-proxy.log` a su lado, seguramente está corriendo el otro `.exe`: usa la otra DLL
(ver "¿Funciona?").

### Probados, sin congelaciones

| Juego | Año | Notas |
|---|---|---|
| LEGO Harry Potter: Years 1-4 | 2010 | Solo busca mandos al arrancar. Si notas congelaciones, usa la DLL de 32 bits. |
| LEGO Star Wars: The Skywalker Saga | 2022 | No necesita la DLL. |

### Sin probar

LEGO Star Wars: The Video Game (2005), LEGO Star Wars II: The Original Trilogy (2006), LEGO City Undercover,
LEGO Harry Potter Collection, LEGO Batman: Legacy of the Dark Knight.

Si pruebas alguno, [abre un issue](https://github.com/pooka-v1/lego-games-freeze-fix/issues) y cuenta cómo te
fue, con el fichero `dinput8-proxy.log` (ver "¿Funciona?"). Se puede escribir en español.

## Instalar

1. Descarga `lego-games-freeze-fix-v1.0.zip` de
   [Releases](https://github.com/pooka-v1/lego-games-freeze-fix/releases) y descomprímelo. Contiene:
   ```
   32-bit\dinput8.dll
   64-bit\dinput8.dll
   README.txt
   ```
2. Abre la carpeta del juego, la que tiene el `.exe`:
   - **Steam**: clic derecho en el juego > *Administrar* > *Ver archivos locales*.
   - **GOG Galaxy** (sin español): página del juego > icono *More* > *Manage installation* > *Show folder*.
   - **Epic**: clic derecho en el juego > *Gestionar* > sección *Instalación* > icono de carpeta.
3. Copia el `dinput8.dll` de la carpeta que diga la tabla (`32-bit` o `64-bit`) junto al `.exe` del juego.
4. Arranca el juego. Aparece un fichero `dinput8-proxy.log` en la misma carpeta: la DLL funciona.

No la copies nunca en `C:\Windows`: solo funciona desde la carpeta del juego, y allí sustituiría el
`dinput8.dll` del propio Windows.

## Desinstalar

Borra `dinput8.dll` y `dinput8-proxy.log` de la carpeta del juego.

Al desinstalar un juego, Steam y GOG dejan esos dos ficheros en la carpeta del juego (Epic borra la carpeta
entera): se pueden borrar a mano.

## ¿Funciona?

Al arrancar el juego, la DLL escribe `dinput8-proxy.log` junto al `.exe`.

- **No hay log**: el juego no ha cargado la DLL. Lo más probable es que los bits no coincidan (prueba la otra
  DLL, ver "Juegos") o que la DLL no esté en la misma carpeta que el `.exe` que arranca el juego.
- **Hay log**: busca estas dos líneas (cada línea empieza con la fecha y la hora):
  ```
  WMI shortcut: installed
  WMI shortcut: first answer (CreateInstanceEnum), 3 XInput device(s), the game waited 1234 ms
  ```
  La segunda puede decir `ExecQuery` en vez de `CreateInstanceEnum`: las dos valen. El juego espera 1-2 s una
  sola vez, en la pantalla de carga; después ya no hay congelaciones.
- `NOT installed`, `unexpected class` o `unexpected query`: este juego pregunta por los mandos de una forma que
  la DLL no conoce. Avisa, por favor.

Si algo no funciona, [abre un issue](https://github.com/pooka-v1/lego-games-freeze-fix/issues) con el juego, la
tienda y el fichero `dinput8-proxy.log` adjunto.

## Notas

- **Steam Input puede seguir activado.** El juego sigue viendo el mando virtual de Steam como mando Xbox.
- **Dos mandos Xbox sin Steam Input, solo en los juegos más antiguos**: el juego puede cruzar los nombres de
  los mandos, y el jugador 2 sale "sin asignación". Visto en LEGO Star Wars: The Complete Saga y LEGO Indiana
  Jones: The Original Adventures; desde LEGO Indiana Jones 2, en nuestras pruebas el jugador 2 entró bien. Es un
  fallo del propio juego, pasa igual sin la DLL. Arreglo: configurar a mano los controles del jugador 2 en las
  opciones del juego, o activar Steam Input.
- **Avisos del antivirus.** La DLL no tiene firma digital (los certificados de firma son caros para un proyecto
  personal gratuito), y cambia cómo habla el juego con Windows, que es justo lo que vigilan los antivirus. Puede
  salir un aviso, y sería un falso positivo. Todo el código está aquí, puedes compilarlo tú (ver "Compilar"), y
  cada release publica el SHA-256 de cada DLL. Para comprobar la tuya, en PowerShell:
  ```powershell
  Get-FileHash .\dinput8.dll
  ```

## Cómo funciona

### La causa

Encontrada con volcados de memoria tomados durante las congelaciones (ver "Herramientas de investigación"): el
hilo principal del juego estaba parado siempre en el mismo sitio, una consulta a **WMI**
(`fastprox!CWbemSvcWrapper::XWbemServices::CreateInstanceEnum`).

Los juegos llevan el código de ejemplo de Microsoft `IsXInputDevice`, que muchos juegos de la época copiaron tal
cual:

1. Más o menos cada minuto, el juego busca sus dispositivos con DirectInput (`EnumDevices`): teclados, ratones
   y mandos.
2. **Por cada dispositivo encontrado**, teclado y ratón incluidos, pide a Windows la lista completa de
   dispositivos (`Win32_PnPEntity`) para ver si su identificador contiene `IG_`, es decir, si es un mando Xbox.
3. Un PC normal tiene unos cientos de dispositivos (placa base, Bluetooth, audio, red...). Cada consulta tarda
   unos 1,7 s, y con varios dispositivos y el PC ocupado con el juego, la espera sube a 15 s.

La propia documentación de Microsoft desaconseja hacer esa consulta de forma repetida.

### La solución

En pocas palabras: la DLL hace a Windows esa pregunta lenta una sola vez, en segundo plano, guarda la respuesta
y se la da al juego al instante cada vez que vuelve a preguntar.

En detalle: los juegos cargan `dinput8.dll` desde su propia carpeta antes que desde la de Windows. Esta DLL:

- carga el `dinput8.dll` real de Windows y le pasa todas las llamadas **sin cambiarlas**: el juego ve los mismos
  teclados, ratones y mandos que siempre;
- instala un **atajo de WMI** (`WmiShortcut.cpp`): en la tabla de importaciones del juego cambia
  `CoCreateInstance` por una función propia. Cuando el juego pide WMI, recibe un WMI falso que solo contesta a
  "dame `Win32_PNPEntity`" (preguntado con `CreateInstanceEnum` o `ExecQuery`), y solo con los dispositivos
  cuyo identificador contiene `IG_`. Cualquier otro `CoCreateInstance` pasa a Windows sin tocarlo;
- esa respuesta es **la del WMI real, leída una sola vez** en un hilo en segundo plano al arrancar el juego
  (`RealWmi.cpp`). Tiene que ser la real porque **Steam Input** añade su mando virtual a la respuesta de WMI
  desde dentro del proceso del juego: sin él, el juego no ve ese mando como mando Xbox;
- **guarda las listas de dispositivos** (`EnumCache.h`). Aun sin WMI, cada `EnumDevices` de DirectInput tarda
  unos 160 ms, y el juego pide tres seguidas. La primera búsqueda de cada tipo se hace de verdad; las siguientes
  reciben la lista guardada al instante;
- **actualiza las listas al conectar o desconectar un mando**: un hilo en segundo plano lee las listas de
  DirectInput, después vuelve a leer el WMI real, y solo entonces entrega las dos al juego, sin pararlo.

El fichero opcional `dinput8-proxy.ini` (plantilla en este repositorio) apaga partes para averiguar cuál causa
un problema en un juego concreto: `WmiShortcut=0` devuelve las congelaciones largas, `EnumCache=0` los tirones
cortos.

## Compilar

Hace falta Visual Studio con la carga de trabajo *Desarrollo para el escritorio con C++*.

1. Abre `dinput8.slnx` (o `dinput8\dinput8.vcxproj` si tu Visual Studio no abre ficheros `.slnx`).
2. Compila **Release | x86** → `bin\Win32\Release\dinput8.dll` (juegos de 32 bits).
3. Compila **Release | x64** → `bin\x64\Release\dinput8.dll` (juegos de 64 bits).

El runtime de C++ va dentro de la DLL (`/MT`), así que los jugadores no necesitan el Visual C++ Redistributable.

## Herramientas de investigación

Los scripts de `tools\` encontraron la causa. Sirven para cualquier otro juego que se congele.

| Script | Qué hace |
|---|---|
| `freeze-probe.ps1` | CPU del juego y de los 3 procesos que más gastan, cada ~1 s → `freeze-probe.csv`. |
| `freeze-dump.ps1` | Cada **Bloq Despl** hace 2 volcados de memoria del juego, separados 3 s → `tools\dumps\`. |
| `analyze-dumps.ps1` | Dónde está parado el hilo principal en cada volcado (`-AllThreads`: todos los hilos). |

```powershell
powershell -ExecutionPolicy Bypass -File tools\freeze-dump.ps1 -GameName LEGOStarWarsSaga
powershell -ExecutionPolicy Bypass -File tools\analyze-dumps.ps1
```

`-GameName` es el nombre del `.exe` sin la extensión. Necesitan dos herramientas de Microsoft que no se incluyen
aquí (su licencia no lo permite):

- `freeze-dump.ps1`: [ProcDump](https://learn.microsoft.com/sysinternals/downloads/procdump), descomprimido en
  `tools\procdump\`.
- `analyze-dumps.ps1`: WinDbg (`winget install Microsoft.WinDbg`). La primera vez descarga los símbolos de
  Microsoft en `tools\symbols\`.

Por qué Bloq Despl: no le quita el foco al juego. Cualquier herramienta que obligue a hacer clic fuera del juego
(Process Explorer, el Administrador de tareas) no sirve con pantalla completa exclusiva: el juego se minimiza y
sus hilos se paran, y eso cambia lo que se quiere medir.

## Aviso legal

Proyecto no oficial, hecho por un aficionado. No está afiliado ni respaldado por el Grupo LEGO, TT Games ni
Warner Bros. Games. LEGO es una marca registrada del Grupo LEGO. Los nombres de los juegos solo se usan para
decir con qué juegos funciona este arreglo.

## Licencia

[MIT](LICENSE) © 2026 pooka-v1
