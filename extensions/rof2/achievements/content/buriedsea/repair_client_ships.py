#!/usr/bin/env python3
"""Enable existing RoF2 ship assets in Buried Sea; inspect unless --apply."""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path
import shutil
import tempfile


def inspect(client):
    target = client / 'buriedsea_chr.txt'
    if target.is_symlink() or target.stat().st_nlink != 1:
        raise ValueError('Use a separate client copy; the model list is linked.')
    before = target.read_bytes()
    lines = before.decode('ascii').splitlines()
    if not lines or not lines[0].isdigit() or int(lines[0]) != len(lines) - 1:
        raise ValueError('Unexpected model-list format; merge this custom file manually.')
    entries = lines[1:]
    if any(not entry or len(entry.split(',')) != 2 for entry in entries):
        raise ValueError('Unexpected model entry; no file changed.')
    required = ['b04,b04', 'b05,b05', 'b06,b06']
    for entry in required:
        if not (client / (entry.split(',')[1] + '.eqg')).is_file():
            raise ValueError('Missing original local ship asset: ' + entry.split(',')[1] + '.eqg')
    missing = [entry for entry in required if entry not in entries]
    newline = '\r\n' if b'\r\n' in before else '\n'
    after = (newline.join([str(len(entries) + len(missing))] + entries + missing) + newline).encode('ascii')
    return target, before, after, missing


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', required=True, type=Path, help='The RoF2 client folder, not its Proton prefix')
    parser.add_argument('--apply', action='store_true', help='Back up and update the model list; re-enter the zone afterward')
    args = parser.parse_args()
    target, before, after, missing = inspect(args.client)
    result = {'file': str(target), 'missing_entries': missing, 'applied': False}
    if args.apply and missing:
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        backup = target.with_name(target.name + '.before-ships-' + stamp)
        # Create a fresh preimage and refuse any change during inspection.
        with backup.open('xb') as stream:
            stream.write(before)
        shutil.copystat(target, backup)
        fd, temporary = tempfile.mkstemp(prefix=target.name + '.', dir=target.parent)
        try:
            with os.fdopen(fd, 'wb') as stream:
                stream.write(after)
            shutil.copystat(target, temporary)
            if target.is_symlink() or target.stat().st_nlink != 1 or target.read_bytes() != before:
                raise ValueError('Model list changed during inspection; backup retained, update refused.')
            os.replace(temporary, target)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)
        result.update(applied=True, backup=str(backup), before_sha256=hashlib.sha256(before).hexdigest(), after_sha256=hashlib.sha256(after).hexdigest())
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (OSError, UnicodeError, ValueError) as error:
        raise SystemExit(str(error))
