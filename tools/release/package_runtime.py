#!/usr/bin/env python3
"""Assemble runtime assets from this build and committed immutable public inputs. No DB access."""
import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import platform
import re
import shutil
import struct
import subprocess
import tempfile
import time
import urllib.error
import urllib.request
import zipfile


def digest(path):
    h = hashlib.sha256()
    with Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n', encoding='utf-8')


def capture(argv):
    return subprocess.check_output(argv, text=True, errors='replace').strip()


def member_name(info):
    p = PurePosixPath(info.filename)
    if p.is_absolute() or '..' in p.parts or '\\' in info.filename or (info.external_attr >> 16) & 0o170000 == 0o120000:
        raise ValueError('Unsafe input archive member: ' + info.filename)
    return p


def fetch(pin, cache):
    target = cache / pin['sha256']
    if not target.exists():
        part = target.with_suffix('.partial')
        for attempt in range(1, 4):
            part.unlink(missing_ok=True)  # A retry never appends to an earlier response.
            request = urllib.request.Request(pin['url'], headers={'User-Agent': 'EQEmu-runtime-packager'})
            try:
                with urllib.request.urlopen(request, timeout=180) as response, part.open('wb') as out:
                    shutil.copyfileobj(response, out, 1024 * 1024)
                if part.stat().st_size != pin['bytes'] or digest(part) != pin['sha256']:
                    raise ValueError('Pinned asset bytes/hash mismatch: ' + pin['url'])
                part.replace(target)  # Only complete, verified bytes become a cache entry.
                break
            except urllib.error.HTTPError as error:
                error.close()
                transient = error.code in (502, 503, 504)
                if not transient or attempt == 3:
                    raise RuntimeError(
                        f"Pinned asset download failed: HTTP {error.code}, attempt {attempt}/3: {pin['url']}"
                    ) from error
                print(
                    f"Transient HTTP {error.code} fetching {pin['url']}; "
                    f"attempt {attempt}/3 failed, retrying in {attempt}s",
                    flush=True,
                )
            finally:
                part.unlink(missing_ok=True)
            time.sleep(attempt)
    if target.stat().st_size != pin['bytes'] or digest(target) != pin['sha256']:
        raise ValueError('Pinned cached input differs')
    return target


def copy_member(z, info, target, expected=None):
    member_name(info)
    target.parent.mkdir(parents=True, exist_ok=True)
    with z.open(info) as src, target.open('xb') as out:
        shutil.copyfileobj(src, out, 1024 * 1024)
    target.chmod(0o644)
    if expected and digest(target) != expected:
        raise ValueError('Input member hash mismatch: ' + info.filename)


def database_content(path, contract, stage):
    required = contract['required_members_sha256']
    with zipfile.ZipFile(path) as z:
        infos = {i.filename: i for i in z.infolist() if not i.is_dir()}
        if len(infos) != contract['payload_inventory']['payload_member_count']:
            raise ValueError('Unexpected database payload inventory')
        for name, expected in required.items():
            if hashlib.sha256(z.read(name)).hexdigest() != expected:
                raise ValueError('Pinned database root member differs: ' + name)
        sums = {}
        for line in z.read('SHA256SUMS').decode('utf-8').splitlines():
            sha, name = line.split(None, 1)
            name = name.lstrip('* ')
            if not re.fullmatch('[0-9a-f]{64}', sha) or name in sums:
                raise ValueError('Malformed/duplicate input checksum')
            sums[name] = sha
        if set(sums) != set(infos) - {'SHA256SUMS'}:
            raise ValueError('Input checksums do not cover exact database payload')
        json_count = 0
        for name, info in infos.items():
            member_name(info)
            if name.startswith('housing-runtime-content/'):
                if len(PurePosixPath(name).parts) != 2 or not name.endswith('.json'):
                    raise ValueError('Unexpected Housing input')
                destination = stage / 'server' / PurePosixPath(name).name
                json_count += 1
            else:
                destination = stage / 'database' / name
            copy_member(z, info, destination, required.get(name) if name == 'SHA256SUMS' else sums[name])
        if json_count != contract['payload_inventory']['housing_json_member_count']:
            raise ValueError('Incomplete Housing content')
    if (stage / 'database/full.sql').stat().st_size != contract['payload_inventory']['full_sql_bytes']:
        raise ValueError('SQL length differs')


def seed_content(path, pin, stage):
    count = 0
    with zipfile.ZipFile(path) as z:
        for info in z.infolist():
            if info.is_dir() or not info.filename.startswith(pin['archive_root']):
                continue
            name = info.filename[len(pin['archive_root']):]
            if not (name in pin['allow_files'] or any(name.startswith(p) for p in pin['allow_prefixes'])):
                continue  # In particular never copy old native binaries or metadata.
            member_name(info)
            if name.startswith('server/quests/'):
                if not name.endswith(('.lua', '.pl', '.md')) and PurePosixPath(name).name not in ('LICENSE', 'COPYING'):
                    raise ValueError('Unexpected quest content type')
                count += name.endswith(('.lua', '.pl'))
            copy_member(z, info, stage / name)
    if count != pin['quest_script_count']:
        raise ValueError('Incomplete pinned quest overlay')
    config = stage / 'server/eqemu_config.json.example'
    cfg = json.loads(config.read_text(encoding='utf-8'))
    cfg['server']['directories'].update(plugins='quests/plugins/', lua_modules='quests/lua_modules/')
    write_json(config, cfg)  # Portable on Windows too; no symlink-only module lookup.


def architecture(path, windows):
    with path.open('rb') as f:
        head = f.read(64)
        if windows:
            if head[:2] != b'MZ':
                raise ValueError('Not a Windows executable: ' + path.name)
            f.seek(struct.unpack_from('<I', head, 60)[0])
            if f.read(6) != b'PE\0\0d\x86':
                raise ValueError('Not Windows x64: ' + path.name)
        elif head[:5] != b'\x7fELF\x02' or struct.unpack_from('<H', head, 18)[0] != 62:
            raise ValueError('Not Linux x86-64: ' + path.name)


def linux_requirements(binaries):
    cache = capture(['/sbin/ldconfig', '-p'])
    libs = {}
    for line in cache.splitlines():
        fields = line.split()
        if 'x86-64' in line and '=>' in fields:
            libs.setdefault(fields[0], fields[-1])
    needed, seen, queue = set(), set(), list(binaries)
    while queue:
        file = queue.pop()
        if str(file) in seen:
            continue
        seen.add(str(file))
        dynamic = capture(['readelf', '-d', str(file)])
        for soname in re.findall(r'Shared library: \[([^]]+)\]', dynamic):
            if soname not in libs:
                raise ValueError('Unresolved runtime library: ' + soname)
            needed.add(soname)
            queue.append(Path(libs[soname]))
    os_info = Path('/etc/os-release').read_text(encoding='utf-8')
    return {'system_libraries': sorted(needed), 'build_distribution': os_info,
            'glibc': capture(['getconf', 'GNU_LIBC_VERSION']),
            'Perl_version': capture(['perl', '-e', 'print $^V']),
            'policy': 'Install matching distribution libraries/Perl ABI; they are not copied into this ZIP.'}


def windows_requirements(binaries, binary_dir, build, destination):
    dumpbin = shutil.which('dumpbin')
    if not dumpbin:
        raise ValueError('MSVC dumpbin is required to resolve the Windows DLL closure')
    search = [binary_dir, build / 'vcpkg_installed/x64-windows/bin']
    search += [Path(p) for p in os.environ.get('PATH', '').split(os.pathsep) if p]
    system = Path(os.environ['SystemRoot']) / 'System32'
    queue, seen, bundled, external = list(binaries), set(), {}, set()
    while queue:
        path = queue.pop()
        if path.name.lower() in seen:
            continue
        seen.add(path.name.lower())
        output = capture([dumpbin, '/NOLOGO', '/DEPENDENTS', str(path)])
        names = re.findall(r'^\s+([A-Za-z0-9_.+-]+\.dll)\s*$', output, re.M | re.I)
        for name in names:
            lower = name.lower()
            if lower.startswith(('api-ms-', 'ext-ms-', 'vcruntime', 'msvcp', 'concrt')):
                external.add(name)
                continue
            if re.fullmatch(r'perl\d+(?:_?\d+)*\.dll', lower):
                external.add(name)  # Full matching Strawberry Perl is an explicit prerequisite, including its modules.
                continue
            found = next((directory / name for directory in search if (directory / name).is_file()), None)
            if found is None and (system / name).is_file():
                external.add(name)
                continue
            if found is None:
                raise ValueError('Required Windows DLL was not found: ' + name)
            if found.parent.resolve() == system.resolve():
                external.add(name)
                continue
            target = destination / name
            if not target.exists():
                architecture(found, True)
                shutil.copy2(found, target)
            elif digest(target) != digest(found):
                raise ValueError('Conflicting Windows runtime DLL: ' + name)
            bundled[name] = digest(target)
            queue.append(found)
    return {'bundled_DLLs': bundled, 'external_DLLs': sorted(external),
            'Perl_version': capture(['perl', '-e', 'print $^V']),
            'policy': 'Windows x64 plus VC++ 2015-2022-or-newer x64 Redistributable and matching Strawberry Perl x64 on PATH. Perl distribution/standard modules are external, not a lone DLL substitute.'}


def package(args):
    repo, build, out = args.source.resolve(), args.build.resolve(), args.output.resolve()
    inputs = json.loads((repo / 'tools/release/release-inputs.json').read_text(encoding='utf-8'))
    sha = capture(['git', '-C', str(repo), 'rev-parse', 'HEAD'])
    if sha != os.environ['GITHUB_SHA']:
        raise ValueError('Checked-out source does not match this workflow commit')
    db = inputs['database']
    subprocess.run(['git', '-C', str(repo), 'merge-base', '--is-ancestor', db['required_code_boundary']['completed_actor_source_commit'], sha], check=True)
    windows = args.platform == 'windows'
    platform_id = 'Windows-x64' if windows else 'Linux-x86_64'
    stem = 'EQEmu-RoF2-' + platform_id
    binary_dir = build / 'bin/Release' if windows else build / 'bin'
    cache = build / 'release-input-cache'
    cache.mkdir(exist_ok=True)
    database = fetch(db['asset'], cache)
    seed = fetch(inputs['runtime_content'], cache)
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='eqemu-release-', dir=build) as tmp:
        stage = Path(tmp) / 'EQEmu-RoF2-server'
        stage.mkdir()
        seed_content(seed, inputs['runtime_content'], stage)
        database_content(database, db, stage)
        # New qualifications must come from this checkout, not the older content seed.
        merc_docs = stage / 'common/mercenary'
        merc_docs.mkdir(parents=True)
        for name in ('LEVEL100.md', 'RESOURCE-UPDATES.md', 'README.md'):
            shutil.copy2(repo / 'common/mercenary' / name, merc_docs / name)
        limits = stage / 'CONTENT-LIMITS.md'
        with limits.open('a', encoding='utf-8') as output:
            output.write('\n## Mercenary level 86–100 content\n\n'
                         'The numeric stats and NPC repertoire are documented emulator reconstruction; '
                         'an immutable May 2013 definition snapshot and exact historical Live balance/casts '
                         'are not authenticated. Read [the same-source level-100 guide]'
                         '(common/mercenary/LEVEL100.md) for scope, cache/upgrade requirements and native '
                         'acceptance limits. Build/package success does not establish gameplay acceptance.\n')
        native = stage / 'server/bin'
        native.mkdir(parents=True)
        hashes, binaries = {}, []
        for name in inputs['runtime_binaries']:
            filename = name + ('.exe' if windows else '')
            src = binary_dir / filename
            architecture(src, windows)
            dst = native / filename
            shutil.copy2(src, dst)
            original = digest(src)
            if not windows:
                subprocess.run(['strip', '--strip-debug', str(dst)], check=True)  # Staged copy only; keep build/debug artifacts out of runtime ZIP.
                dst.chmod(0o755)
            hashes[filename] = {'built_sha256': original, 'packaged_sha256': digest(dst)}
            binaries.append(dst)
        requirements = windows_requirements(binaries, binary_dir, build, native) if windows else linux_requirements(binaries)
        licenses = stage / 'licenses'
        for copyright in (build / 'vcpkg_installed').glob('*/share/*/copyright'):
            target = licenses / copyright.parent.name / 'copyright'
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(copyright, target)
        shutil.copy2(repo / 'LICENSE.md', stage / 'LICENSE.md')
        tools = stage / 'tools/housing'
        tools.mkdir(parents=True)
        for name in ('install_original_geometry.py', 'housing_platform.py'):
            shutil.copy2(repo / 'extensions/rof2/housing/tools' / name, tools / name)
        catalogs = tools / 'catalog/common/housing'
        catalogs.mkdir(parents=True)
        for name in inputs['geometry_catalogs']:
            shutil.copy2(repo / 'common/housing' / name, catalogs / name)
        templates = repo / 'tools/release/templates'
        for name in ('supervise-server.py', 'run-server.cmd' if windows else 'run-server.sh'):
            shutil.copy2(templates / name, stage / 'server' / name)
        if not windows:
            (stage / 'server/run-server.sh').chmod(0o755)
        for name in ('logs', 'shared', 'maps', 'export'):
            (stage / 'server' / name).mkdir(exist_ok=True)
        version = {'commit': sha, 'run_number': int(os.environ['GITHUB_RUN_NUMBER']),
                   'run_id': int(os.environ['GITHUB_RUN_ID']), 'run_attempt': int(os.environ['GITHUB_RUN_ATTEMPT']),
                   'platform': platform_id, 'binary_hashes': hashes, 'requirements': requirements,
                   'database_commit': db['source_commit'], 'database_asset_sha256': db['asset']['sha256'],
                   'SQL_sha256': db['required_members_sha256']['full.sql'],
                   'runtime_content_source': inputs['runtime_content']['source_commit'],
                   'runtime_content_asset_sha256': inputs['runtime_content']['sha256'],
                   'release_inputs_sha256': digest(repo / 'tools/release/release-inputs.json'),
                   'submodules': capture(['git', '-C', str(repo), 'submodule', 'status', '--recursive']),
                   'fresh_empty_database_only': True}
        write_json(stage / 'RELEASE.json', version)
        readme = (repo / 'tools/release/templates/INSTALL.md').read_text(encoding='utf-8')
        readme = readme.replace('@PLATFORM@', platform_id).replace('@COMMIT@', sha)
        readme = readme.replace('@REQUIREMENTS@', json.dumps(requirements, indent=2))
        readme = readme.replace('@LAUNCH@', 'run-server.cmd' if windows else './run-server.sh')
        (stage / 'INSTALL.md').write_text(readme, encoding='utf-8')
        sums = [digest(p) + '  ' + str(p.relative_to(stage)).replace(os.sep, '/') for p in sorted(stage.rglob('*')) if p.is_file()]
        (stage / 'SHA256SUMS').write_text('\n'.join(sums) + '\n', encoding='utf-8')
        archive = out / (stem + '.zip')
        with zipfile.ZipFile(archive, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
            for path in sorted(stage.rglob('*')):
                if path.is_dir() and not any(path.iterdir()):
                    z.write(path, 'EQEmu-RoF2-server/' + str(path.relative_to(stage)).replace(os.sep, '/') + '/')
                elif path.is_file():
                    z.write(path, 'EQEmu-RoF2-server/' + str(path.relative_to(stage)).replace(os.sep, '/'))
        with zipfile.ZipFile(archive) as z:
            if z.testzip() is not None:
                raise ValueError('Runtime archive CRC failure')
            if any('/source/' in n or '/build/' in n or n.endswith(('.pdb', '.obj', '.a', '.lib')) for n in z.namelist()):
                raise ValueError('Build/debug bulk in runtime archive')
        write_json(out / (stem + '.manifest.json'), version | {'asset': archive.name, 'asset_sha256': digest(archive), 'asset_bytes': archive.stat().st_size})
        (out / (stem + '.zip.sha256')).write_text(digest(archive) + '  ' + archive.name + '\n', encoding='utf-8')
        shutil.copy2(stage / 'INSTALL.md', out / (stem + '.INSTALL.md'))
        print(json.dumps({'platform': platform_id, 'asset': str(archive), 'sha256': digest(archive)}))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=Path('.'))
    parser.add_argument('--build', type=Path, default=Path('build'))
    parser.add_argument('--output', type=Path, default=Path('dist'))
    parser.add_argument('--platform', choices=['linux', 'windows'], required=True)
    package(parser.parse_args())
