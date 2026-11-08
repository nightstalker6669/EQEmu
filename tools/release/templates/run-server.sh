#!/bin/sh
set -eu
umask 077
cd "$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
exec python3 ./supervise-server.py --server-directory "$PWD" --expected-database "${EQEMU_DATABASE_NAME:-peq}" --recovery-executable "$PWD/bin/inventory_recovery_cli" --enable-supervised-inventory --zone-count "${EQEMU_ZONE_COUNT:-6}"
