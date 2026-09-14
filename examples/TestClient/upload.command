#!/bin/bash
exec /bin/bash "$(dirname -- "$0")/nanolab.command" upload "$@"
