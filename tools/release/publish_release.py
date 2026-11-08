#!/usr/bin/env python3
"""Promote a complete trusted-develop CI release. Never modify an earlier release."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def api(endpoint, method='GET', data=None):
    args = ['gh', 'api', endpoint, '--method', method]
    if data is not None:
        args += ['--input', '-']
    result = subprocess.run(args, input=None if data is None else json.dumps(data), text=True, capture_output=True, check=True)
    return json.loads(result.stdout) if result.stdout.strip() else None


def latest_run_number(repo):
    result = subprocess.run(['gh', 'api', 'repos/' + repo + '/releases/latest'], text=True, capture_output=True)
    if result.returncode:
        if '404' in result.stderr:
            return -1
        raise RuntimeError('Could not establish existing Latest; no promotion attempted')
    release = json.loads(result.stdout)
    marker = re.search(r'<!-- eqemu-ci-run: (\d+) -->', release.get('body') or '')
    return int(marker.group(1)) if marker else -1


def still_current(repo, sha, number):
    head = api('repos/' + repo + '/git/ref/heads/develop')['object']['sha']
    return head == sha and latest_run_number(repo) <= number


def publish(args):
    repo = os.environ['GITHUB_REPOSITORY']
    sha = os.environ['GITHUB_SHA']
    number, attempt = int(os.environ['GITHUB_RUN_NUMBER']), int(os.environ['GITHUB_RUN_ATTEMPT'])
    inputs = json.loads(args.inputs.read_text(encoding='utf-8'))
    if os.environ.get('GITHUB_EVENT_NAME') != 'push' or os.environ.get('GITHUB_REF') != 'refs/heads/develop' or repo != inputs['runtime_content']['repository']:
        raise RuntimeError('Only trusted repository develop pushes may publish')
    # The release job is serialized; this second guard also rejects older runs finishing out of order.
    if not still_current(repo, sha, number):
        print('::notice::Stale source/run; previous good Latest is untouched')
        return
    directory = args.assets.resolve()
    platforms = ('Linux-x86_64', 'Windows-x64')
    expected = set()
    for platform in platforms:
        stem = 'EQEmu-RoF2-' + platform
        names = {stem + suffix for suffix in ('.zip', '.zip.sha256', '.manifest.json', '.INSTALL.md')}
        expected |= names
        manifest = json.loads((directory / (stem + '.manifest.json')).read_text(encoding='utf-8'))
        if manifest['commit'] != sha or manifest['platform'] != platform or manifest['run_number'] != number or manifest['run_attempt'] != attempt:
            raise ValueError('Artifact source/run binding differs')
        db = inputs['database']
        if manifest['database_commit'] != db['source_commit'] or manifest['SQL_sha256'] != db['required_members_sha256']['full.sql'] or manifest['database_asset_sha256'] != db['asset']['sha256']:
            raise ValueError('Platforms do not bind the committed identical database input')
        if manifest['release_inputs_sha256'] != digest(args.inputs):
            raise ValueError('Artifact release-input contract differs')
        archive = directory / (stem + '.zip')
        if archive.stat().st_size != manifest['asset_bytes'] or digest(archive) != manifest['asset_sha256']:
            raise ValueError('Artifact ZIP hash/size differs')
        if (directory / (stem + '.zip.sha256')).read_text(encoding='utf-8') != digest(archive) + '  ' + archive.name + '\n':
            raise ValueError('Artifact sidecar differs')
    if {p.name for p in directory.iterdir() if p.is_file()} != expected:
        raise ValueError('Incomplete or unexpected release artifact set')
    shutil_inputs = directory / 'RELEASE-INPUTS.json'
    shutil_inputs.write_bytes(args.inputs.read_bytes())
    sums = ''.join(digest(directory / name) + '  ' + name + '\n' for name in sorted(expected | {'RELEASE-INPUTS.json'}))
    (directory / 'SHA256SUMS').write_text(sums, encoding='utf-8')
    tag = 'rof2-ci-r' + str(number) + '-a' + str(attempt) + '-' + sha[:12]
    notes = directory.parent / 'release-notes.md'
    notes.write_text('''<!-- eqemu-ci-run: {number} -->
# Matched Linux x86-64 and Windows x64 runtime release

Source commit: [{sha}](https://github.com/{repo}/commit/{sha}). Both platform builds, existing tests and runtime packaging succeeded in [this workflow](https://github.com/{repo}/actions/runs/{run}).

Stable Latest downloads: [Linux](https://github.com/{repo}/releases/latest/download/EQEmu-RoF2-Linux-x86_64.zip) · [Windows](https://github.com/{repo}/releases/latest/download/EQEmu-RoF2-Windows-x64.zip).

Database source `{db}`; immutable input `{dbtag}`. Both ZIPs include identical full SQL `{sql}` and 331 portable Housing JSON files. **NEW EMPTY DATABASE ONLY:** full SQL replaces tables; no player-preserving upgrade. Original authenticated RoF2 resources/maps and each platform's documented OS/Perl ABI prerequisites remain external.

Read the platform install guide, CONTENT-LIMITS.md and per-platform manifest. 32 LOCAL scalars/two Dwarf quantity choices and source-UNCONFIRMED stew qualifications remain; three open recipe limits stay unsupported. No client archives, player data, private config, source/build/cache/debug bulk are in these runtime ZIPs. Source and pinned submodules are available at this exact commit; dependency notices accompany runtime files. No serving runtime or SQL deployment is performed by publishing.
'''.format(number=number, sha=sha, repo=repo, run=os.environ['GITHUB_RUN_ID'], db=inputs['database']['source_commit'], dbtag=inputs['database']['release_tag'], sql=inputs['database']['required_members_sha256']['full.sql']), encoding='utf-8')
    if not still_current(repo, sha, number):
        print('::notice::Source advanced before staging; Latest remains untouched')
        return
    # A failed upload/readback leaves only an unpublished draft; no existing release is edited/deleted.
    subprocess.run(['gh', 'release', 'create', tag, '--repo', repo, '--draft', '--target', sha,
                    '--title', 'RoF2 CI runtime r' + str(number) + ' (' + sha[:12] + ')', '--notes-file', str(notes)], check=True)
    files = sorted(p for p in directory.iterdir() if p.is_file())
    subprocess.run(['gh', 'release', 'upload', tag, '--repo', repo] + [str(p) for p in files], check=True)
    release = api('repos/' + repo + '/releases/tags/' + tag)
    if not release['draft']:
        raise RuntimeError('Staging release is unexpectedly public')
    assets = {x['name']: x for x in release['assets']}
    if set(assets) != {p.name for p in files}:
        raise RuntimeError('Incomplete staged remote release; draft remains unpublished')
    for path in files:
        item = assets[path.name]
        if item['state'] != 'uploaded' or item['size'] != path.stat().st_size or item.get('digest') != 'sha256:' + digest(path):
            raise RuntimeError('Remote asset checksum/state differs; draft remains unpublished')
    if not still_current(repo, sha, number):
        print('::notice::Newer source/Latest detected; complete draft left unpublished')
        return
    api('repos/' + repo + '/releases/' + str(release['id']), 'PATCH', {'draft': False, 'make_latest': 'true'})
    readback = api('repos/' + repo + '/releases/latest')
    if readback['tag_name'] != tag or readback['draft']:
        raise RuntimeError('Published release Latest readback differs')
    print('Published complete Latest: ' + readback['html_url'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--assets', type=Path, required=True)
    parser.add_argument('--inputs', type=Path, default=Path('tools/release/release-inputs.json'))
    publish(parser.parse_args())
