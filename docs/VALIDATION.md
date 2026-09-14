# Validation: reproducible student handoff

Tested on 2026-09-14 against Nanolab source based on commit `9ff9574fa31726fd2cbdaaa0190560ba2519ca00`.

## Toolchain

- Installed Arduino IDE application: **2.3.10**, verified from its application metadata.
- Its bundled Arduino CLI: **1.5.1**, commit `01f3d4f2b`.
- Profile-selected Espressif Arduino core: **3.3.11**. The first profile build installed its isolated resources and used the project-declared local CAN library.
- Target: ESP32-S3 Dev Module, 8 MB flash, no PSRAM, native Hardware CDC/JTAG, CDC on boot, DIO 80 MHz, CPU 240 MHz, default partition layout.

## Passed checks

1. `examples/TestClient/sketch.yaml` profile compilation: 386934 bytes of program storage, 33480 bytes of static RAM.
2. macOS project generator produced a standalone `HandoffTest` sketch containing its local CAN library. The compile script succeeded: 386722 bytes of program storage, 33472 bytes of static RAM.
3. The complete project was zipped without build output, extracted under a different parent directory containing spaces, and compiled successfully using the PowerShell compile launcher. Its source had no dependency on the original Nanolab checkout. The generated project was also committed to a local Git repository, cloned into another directory, and successfully compiled with the macOS launcher.
4. PowerShell project generation produced the same YAML dependency paths as macOS generation. All PowerShell scripts passed parsing. Existing output folders were refused without overwriting them.
5. Both upload launchers passed mocked tests for no USB ports, multiple USB ports, one USB port, an explicit `PORTNO`, an incorrect CLI version and a failed compilation. Ambiguous discovery and wrong versions did not compile/upload; failed compilation never uploaded. Successful uploads passed the selected port and the 1000000-baud override.
6. On the real Mac with both the client and server connected, automatic upload selection correctly stopped and listed the ports.
7. The macOS upload script rebuilt the example from its profile, flashed client MAC `cc:8d:a2:20:8d:d4` through `/dev/cu.usbmodem11401` at **1000000 baud**, and esptool verified the written data.
8. Client serial output reported ID `8dd4`, 250 kbit/s CAN and a server storage acknowledgment. SFTP readback showed a new complete **80-byte** record at `2026-09-14T14:39:00Z`, with client uptime `3.457` seconds and status OK after flashing the 3.3.11 build.
9. Shell syntax, Git whitespace and ZIP portability checks passed.

## Scope

Compilation and hardware testing used **the CLI shipped inside Arduino IDE 2.3.10**. The IDE application's graphical Verify/Upload buttons were not exercised because desktop Accessibility/Screen Recording permission was unavailable. PowerShell launchers were executed with PowerShell 7.6.6 on macOS; actual Windows USB discovery, Windows IDE installation paths and Windows PowerShell 5.1 were not exercised. Scripts intentionally use syntax compatible with PowerShell 5.1.

One physical CAN client was tested. The server firmware and CAN wire protocol were not changed. Runtime library source is unchanged in this handoff update.
