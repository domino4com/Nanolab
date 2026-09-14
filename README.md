# Nanolab: add status reporting and submit your experiment

Keep your existing experiment code. Add the CAN client initialization in `setup()` and publish a short status string whenever your measurements change. The Nanolab control server requests the latest string every **60 seconds**; CAN communication runs in the background.

## 1. Use these exact versions

- **Arduino IDE 2.3.10**.
- **esp32 by Espressif Systems 3.3.11**, selected in Boards Manager.
- **Arduino CLI 1.5.1**, already bundled with Arduino IDE 2.3.10. The supplied scripts locate it in the usual IDE installation folders; no Python, Node.js, or additional libraries are required for the scripts.

Board: **ESP32S3 Dev Module**, 8 MB flash, PSRAM disabled, Hardware CDC and JTAG, USB CDC On Boot enabled, DIO 80 MHz, CPU 240 MHz. The example retains the default partition scheme (about 1.2 MB application space on the 8 MB chip). If your experiment needs different partition or board settings, agree them with Bjarke and record them in the FQBN in `sketch.yaml`.

For Boards Manager, use the official additional URL:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Install IDE 2.3.10 from the [official release](https://github.com/arduino/arduino-ide/releases/tag/2.3.10). Use **3.3.11 exactly**, rather than whichever ESP32 release is newest.

## 2. Create your submission folder

Download this repository as a ZIP and extract it, or clone it. From the Nanolab folder, run one of:

**macOS Terminal:**

```sh
bash ./create-project.command MyExperiment ../
```

**Windows PowerShell:**

```powershell
.\create-project.ps1 -Name MyExperiment -Parent ..
```

Replace `MyExperiment` with your project name (letters, digits and underscores, starting with a letter). This creates a separate, self-contained folder; an existing folder is never overwritten. Open `MyExperiment/MyExperiment.ino` in Arduino IDE 2.3.10 and merge your existing code into it. Copy any other source files your experiment needs into that folder as well. Keep exactly one `setup()` and one `loop()`.

The main `.ino` filename **must match the sketch folder name**. The generator includes the exact CAN library source in `libraries/CanStatusClient` and lists it in `sketch.yaml`. You can compile this project without installing the CAN library globally. If you also use the IDE's Verify button, install the Nanolab repository ZIP through **Sketch → Include Library → Add .ZIP Library**. The supplied CLI scripts are the required final build check and use the project-local source.

To try the existing 80-byte demonstration first, run `compile.command` / `compile.ps1` or `upload.command` / `upload.ps1` in `examples/TestClient`. Keep the full Nanolab repository together: that example's `sketch.yaml` points to the library two directories above it. The **generated project** has no such dependency on the original repository.

## 3. Add only these calls to your existing code

```cpp
#include <CanStatusClient.h>
CanStatusClient canStatus; // Global: do not put this inside setup().

void setup() {
  // Your existing sensor initialization.
  if (!canStatus.begin()) {
    // CAN startup failed; GPIO40 turns red. Handle this for your experiment.
    return;
  }
  canStatus.setData("Starting");
}

void loop() {
  // Your existing measurement code; use actual values in the string.
  char record[81];
  snprintf(record, sizeof(record), "temperature=%.2f status=OK", 23.5);
  canStatus.setData(record);
  delay(1000); // Or keep your existing scheduling.
}
```

`setData()` copies the string immediately and returns `false` if it exceeds **80 bytes** or contains newlines, tabs or other ASCII control characters. The previous valid value is retained on failure. Use the latest complete measurement; there is no CAN processing call to add to `loop()`. Intermediate measurements are not queued. `begin()` shuts down Wi-Fi/BLE; remove any code that subsequently starts either radio. Leave the library's Bluetooth compile guard in place.

Pins are CAN TX **7**, CAN RX **6**, and status NeoPixel **40**. The library owns the CAN/TWAI controller. Do not initialize a second CAN driver. Amber means waiting, cyan means responding, green means the server acknowledged storage, and red means a startup/CAN fault. Each client's ID comes from the final two bytes of its factory MAC; tell Bjarke that ID and check IDs are unique across the experiments.

## 4. List every library in `sketch.yaml`

The generated file pins the board and ESP32 core and initially lists only the bundled CAN library:

```yaml
profiles:
  nanolab:
    notes: "Arduino IDE 2.3.10; Arduino CLI 1.5.1; ESP32 core 3.3.11"
    fqbn: esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,FlashSize=8M,PSRAM=disabled,FlashMode=dio,CPUFreq=240,PartitionScheme=default
    platforms:
      - platform: esp32:esp32 (3.3.11)
        platform_index_url: https://espressif.github.io/arduino-esp32/package_esp32_index.json
    libraries:
      - dir: libraries/CanStatusClient
default_profile: nanolab
```

Under `libraries:`, add **every external library your sketch uses, including dependencies of those libraries**:

- Library Manager libraries: use their exact Library Manager name and tested version, for example `- Adafruit BusIO (1.17.0)`. This is a syntax example; include it only if your project actually uses that version.
- Custom, modified or non-indexed libraries: copy the complete library, with `library.properties`, source and license, into `libraries/LibraryName`; add `- dir: libraries/LibraryName`. List its dependencies too. Include the source URL and version/commit in an `ORIGIN.txt` file. Do not use an absolute path, a folder outside the submission, or an unpinned Git branch.
- ESP32 core headers and built-in libraries such as `Arduino.h`, `Wire`, `SPI`, `esp_wifi.h` and `esp_bt.h` are already pinned by **esp32 3.3.11**. Do not invent Library Manager entries for them.

Use spaces, not tabs, in YAML. Never omit a version from an indexed library or the ESP32 platform. Include all private/custom library files in the ZIP or Git repository; a dependency on a folder elsewhere on your laptop cannot be compiled by the recipient.

`sketch.yaml` is an **Arduino CLI build profile**. It does not install or select an Arduino IDE version. It also does not make IDE Verify automatically synchronize your dependencies: use the supplied scripts as the handoff test. Profile builds exclude globally installed libraries and automatically fetch the declared indexed versions into an isolated cache. The first build needs internet access and may download several gigabytes of ESP32 tools. [Arduino's build-profile specification](https://docs.arduino.cc/arduino-cli/sketch-project-file/)

## 5. Compile and flash using the scripts

Run these **inside your generated project folder**. They also work when launched from another directory because they locate their own sketch folder.

| Task | macOS | Windows PowerShell |
|---|---|---|
| Compile only | `bash ./compile.command` | `.\compile.ps1` |
| Compile, then flash | `bash ./upload.command` | `.\upload.ps1` |
| Choose the client port | `bash ./upload.command /dev/cu.usbmodemXXXX` | `.\upload.ps1 -Port COM7` |
| List detected ports | `bash ./nanolab.command ports` | `.\nanolab.ps1 -Action ports` |

You may also double-click an executable `.command` file in Finder. For ZIPs where executable bits were lost, the `bash` commands above still work. If Windows blocks downloaded PowerShell scripts, review them and use **Unblock-File** on those specific trusted files, or ask your administrator about an organizational execution policy; do not disable the computer's security policy globally.

Instead of passing the port, set `PORTNO` in the same terminal:

```sh
export PORTNO=/dev/cu.usbmodemXXXX  # macOS
bash ./upload.command
```

```powershell
$env:PORTNO = 'COM7'               # Windows
.\upload.ps1
```

Without a port, the scripts accept exactly one detected USB serial port. If there are zero or several, they list the ports and stop. Disconnect unrelated USB serial devices, or specify the client explicitly. A USB serial adapter's identity does not prove which board is attached. **Do not select the Nanolab server's programming port.** Upload always recompiles and stops if compilation fails. It uses **1000000 baud**, not 921600. Close Serial Monitor before upload.

For a nonstandard IDE installation, set `ARDUINO_CLI` to the full path of its bundled `arduino-cli` executable. The scripts check for CLI **1.5.1** and stop on a different version. This avoids accidentally using an older CLI from your PATH. Alternatively, install the official standalone CLI 1.5.1. Direct equivalents are:

```sh
arduino-cli compile --profile nanolab --clean --build-path build .
arduino-cli upload --profile nanolab --port YOUR_PORT --input-dir build --upload-property upload.speed=1000000 .
```

## 6. When your experiment works, send the complete project to Bjarke

1. Test the actual sensors, CAN status and behavior on your ESP32-S3. Record the client MAC/ID, wiring, any calibration/setup steps and what you tested in the generated `README.md`.
2. Confirm IDE **2.3.10**, core **3.3.11**, the exact board options, and every external library/version/path in `sketch.yaml`.
3. Run the supplied **compile** script on the complete submission. A successful IDE Verify alone is insufficient. Copy the project to a different folder and compile that copy too: it must not refer back to your development folders. Do not rename just the sketch folder without also renaming its main `.ino`.
4. Submit the following complete structure, using either ZIP or Git:

```text
MyExperiment/
├── MyExperiment.ino          # Same base name as the folder
├── sketch.yaml
├── README.md                 # Your experiment, wiring and test results
├── compile.command
├── upload.command
├── nanolab.command
├── compile.ps1
├── upload.ps1
├── nanolab.ps1
├── .gitignore
├── libraries/
│   ├── CanStatusClient/      # Included complete local library
│   │   ├── library.properties
│   │   ├── LICENSE
│   │   ├── ORIGIN.txt
│   │   └── src/...
│   └── OtherLocalLibrary/... # If used; also listed in sketch.yaml
└── ...your other source/data files
```

**ZIP:** remove the generated `build/` folder from the copy you are sending, then compress the entire `MyExperiment` folder. The ZIP must contain that one top-level folder, not just loose `.ino` files. Keep custom libraries, configuration, data assets and all source files. Extract the ZIP elsewhere and run its compile script before sending it.

**Git:** commit the contents of `MyExperiment/` as the repository root, including the local libraries and scripts. Exclude `build/`; the generated `.gitignore` handles this. Any Git host is fine: GitHub, GitLab, Codeberg, or another accessible server. Avoid required submodules so an ordinary clone contains all source. Send the repository URL and exact tested commit or tag, and ensure Bjarke can read it. To preserve Arduino's folder/filename rule even if the repository has another name:

```sh
git clone REPOSITORY_URL MyExperiment
cd MyExperiment
git checkout TESTED_COMMIT_OR_TAG
bash ./compile.command              # macOS
# .\compile.ps1                     # Windows PowerShell
```

The recipient can then run the upload script with their own port. No changes to source or dependency paths should be necessary after cloning or extracting.

For optional advanced API details see [Client reference](docs/CLIENT_API.md). The CAN wire protocol is documented separately in [PROTOCOL.md](PROTOCOL.md). [Validation performed for this release](docs/VALIDATION.md).
