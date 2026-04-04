#!/usr/bin/env bash
set -euo pipefail

if [ -d /workspace ]; then
    git config --global --add safe.directory /workspace >/dev/null 2>&1 || true
fi

if [ -d "${HOME}/.local/bin" ]; then
    case ":${PATH}:" in
        *":${HOME}/.local/bin:"*) ;;
        *) export PATH="${HOME}/.local/bin:${PATH}" ;;
    esac
fi

exec "$@"

