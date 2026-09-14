#!/bin/bash
# macOS launcher: only Arduino CLI and tools included with macOS are required.
set -euo pipefail
cd -- "$(dirname -- "$0")"
action=${1:-compile}
case "$action" in compile|upload|ports) ;; *) echo "Usage: $0 [compile|upload|ports] [serial-port]" >&2; exit 2 ;; esac

# Prefer the CLI shipped with the required IDE over an older Homebrew CLI.
if [ -n "${ARDUINO_CLI:-}" ]; then
    cli=$ARDUINO_CLI
elif [ -x '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli' ]; then
    cli='/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
elif [ -x "$HOME/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" ]; then
    cli="$HOME/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli"
else
    cli=$(command -v arduino-cli || true)
fi
if [ -z "$cli" ] || [ ! -x "$cli" ]; then
    echo 'Install Arduino IDE 2.3.10 in Applications, or set ARDUINO_CLI to Arduino CLI 1.5.1.' >&2
    exit 1
fi
version=$("$cli" version)
case "$version" in *'Version: 1.5.1 '*) ;; *) echo "Required Arduino CLI 1.5.1; found: $version" >&2; exit 1 ;; esac
printf '%s\n' "$version"
if [ "$action" = ports ]; then exec "$cli" board list; fi
if [ ! -f sketch.yaml ] || [ ! -f "$(basename "$PWD").ino" ]; then
    echo 'Keep sketch.yaml and the matching FolderName.ino in this sketch folder.' >&2
    exit 1
fi

port=${2:-${PORTNO:-}}
if [ "$action" = upload ] && [ -z "$port" ]; then
    # Read structured discovery output; never parse localized display columns.
    json=$(mktemp -t nanolab-ports)
    trap 'rm -f "$json"' EXIT
    "$cli" board list --json > "$json"
    candidates=()
    i=0
    while address=$(/usr/bin/plutil -extract "detected_ports.$i.port.address" raw -o - "$json" 2>/dev/null); do
        protocol=$(/usr/bin/plutil -extract "detected_ports.$i.port.protocol" raw -o - "$json" 2>/dev/null || true)
        # USB paths exclude macOS Bluetooth and debug-console pseudo ports.
        if [ "$protocol" = serial ]; then
            case "$address" in /dev/cu.usb*) candidates+=("$address") ;; esac
        fi
        i=$((i + 1))
    done
    if [ "${#candidates[@]}" -ne 1 ]; then
        echo 'Connect exactly one USB serial board, or specify the client port explicitly:' >&2
        "$cli" board list
        echo './upload.command /dev/cu.usbmodemXXXX  (or export PORTNO=...)' >&2
        exit 1
    fi
    port=${candidates[0]}
fi

# Upload always rebuilds, so a failed compilation cannot flash an older binary.
"$cli" compile --profile nanolab --clean --build-path "$PWD/build" .
if [ "$action" = upload ]; then
    printf 'Uploading to %s at 1000000 baud\n' "$port"
    "$cli" upload --profile nanolab --port "$port" --input-dir "$PWD/build" \
        --upload-property upload.speed=1000000 .
fi
