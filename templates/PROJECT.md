# Experiment submission

Fill in this README before sending the project to Bjarke.

- Experiment name and programmer:
- Client factory MAC and final-two-byte ID:
- Hardware/wiring and sensor models:
- Calibration, configuration or data files required:
- Meaning and units of the status string:
- Hardware behavior tested and date:
- Tested Git commit/tag (if submitting through Git):

Required development environment: **Arduino IDE 2.3.10**, **esp32 by Espressif Systems 3.3.11**, **Arduino CLI 1.5.1** (bundled with that IDE). Board options and every external library must be pinned in `sketch.yaml`. Built-in ESP32 libraries are covered by the platform version. Custom libraries must be included locally and declared with relative `dir:` entries. Keep the main `.ino` name identical to this folder's name.

From this folder:

| Task | macOS | Windows PowerShell |
|---|---|---|
| Compile | `bash ./compile.command` | `.\compile.ps1` |
| Compile and flash | `bash ./upload.command` | `.\upload.ps1` |
| Specify port | `bash ./upload.command /dev/cu.usbmodemXXXX` | `.\upload.ps1 -Port COM7` |
| List ports | `bash ./nanolab.command ports` | `.\nanolab.ps1 -Action ports` |

Alternatively set `PORTNO` in the terminal. The upload script stops on ambiguous USB serial ports, rebuilds before upload, and flashes at 1000000 baud. Close Serial Monitor first. Set `ARDUINO_CLI` to the full CLI executable path if the IDE is installed in a nonstandard location.

The first profile build downloads declared dependencies; it needs internet. The scripts build from `sketch.yaml`, independently of globally installed Arduino libraries. Use their successful build as the final handoff check even if IDE Verify already passed.

For ZIP delivery, omit `build/` and compress this entire folder with all source, libraries, YAML, scripts, this README and required data assets. Extract and compile the ZIP in another location before sending it.

For Git delivery, commit this folder's contents as the repository root, including local libraries and scripts; omit `build/`. Send the repository URL and tested commit/tag. The recipient should clone into a directory with the same name as the main `.ino`, check out that commit/tag, and run the compile script. Avoid dependencies outside the repository and required submodules.

Full integration and submission instructions: https://github.com/domino4com/Nanolab
