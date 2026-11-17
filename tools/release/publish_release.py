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


def latest_release(repo):
    result = subprocess.run(['gh', 'api', 'repos/' + repo + '/releases/latest'], text=True, capture_output=True)
    if result.returncode:
        if '404' in result.stderr:
            return {}
        raise RuntimeError('Could not establish existing Latest; no promotion attempted')
    return json.loads(result.stdout)


def still_current(repo, sha, number, previous_tag=None):
    head = api('repos/' + repo + '/git/ref/heads/develop')['object']['sha']
    if head != sha:
        return False
    latest = latest_release(repo)
    marker = re.search(r'<!-- eqemu-ci-run: (\d+) -->', latest.get('body') or '')
    if marker and int(marker.group(1)) > number:
        return False
    return previous_tag is None or latest.get('tag_name', 'none') == previous_tag


def release_notes(path, previous_tag, number):
    if not path.is_file():
        raise ValueError(f'Missing build-specific changelog: {path}')
    header, separator, body = path.read_text(encoding='utf-8').partition('\n\n')
    if header != f'<!-- previous-release: {previous_tag} -->':
        raise ValueError(f'Changelog must name current Latest {previous_tag}: {path}')
    if not separator or not body.strip() or '<!-- eqemu-ci-run:' in body:
        raise ValueError(f'Changelog must contain a readable change list: {path}')
    return f'<!-- eqemu-ci-run: {number} -->\n\n' + body.rstrip('\n')


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
    previous_tag = latest_release(repo).get('tag_name', 'none')
    notes = release_notes(args.notes, previous_tag, number)
    shutil_inputs = directory / 'RELEASE-INPUTS.json'
    shutil_inputs.write_bytes(args.inputs.read_bytes())
    sums = ''.join(digest(directory / name) + '  ' + name + '\n' for name in sorted(expected | {'RELEASE-INPUTS.json'}))
    (directory / 'SHA256SUMS').write_text(sums, encoding='utf-8')
    tag = 'rof2-ci-r' + str(number) + '-a' + str(attempt) + '-' + sha[:12]
    if not still_current(repo, sha, number, previous_tag):
        print('::notice::Source advanced before staging; Latest remains untouched')
        return
    # A failed upload/readback leaves only an unpublished draft; no existing release is edited/deleted.
    created = api('repos/' + repo + '/releases', 'POST', {
        'tag_name': tag, 'target_commitish': sha, 'draft': True,
        'name': 'RoF2 CI runtime r' + str(number) + ' (' + sha[:12] + ')',
        'body': notes})
    if not created['draft'] or created['tag_name'] != tag or created['target_commitish'] != sha:
        raise RuntimeError('Created draft source/tag differs; no promotion attempted')
    release_id = created['id']
    files = sorted(p for p in directory.iterdir() if p.is_file())
    subprocess.run(['gh', 'release', 'upload', tag, '--repo', repo] + [str(p) for p in files], check=True)
    release = api('repos/' + repo + '/releases/' + str(release_id))
    if not release['draft'] or release['tag_name'] != tag or release['target_commitish'] != sha:
        raise RuntimeError('Staging release source/tag/draft differs')
    assets = {x['name']: x for x in release['assets']}
    if set(assets) != {p.name for p in files}:
        raise RuntimeError('Incomplete staged remote release; draft remains unpublished')
    for path in files:
        item = assets[path.name]
        if item['state'] != 'uploaded' or item['size'] != path.stat().st_size or item.get('digest') != 'sha256:' + digest(path):
            raise RuntimeError('Remote asset checksum/state differs; draft remains unpublished')
    if not still_current(repo, sha, number, previous_tag):
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
    parser.add_argument('--notes', type=Path, default=Path('tools/release/RELEASE-NOTES.md'))
    publish(parser.parse_args())
