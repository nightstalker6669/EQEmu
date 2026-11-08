#!/usr/bin/env python3
"""Credential-free release launch template based on the recorded direct supervisor. No eqlaunch/clock/TCP death inference.

Birth identity is recorded before Popen; only this object's exact owned child,
reaped by Popen.wait(), can be certified. No public mark-dead CLI operation exists.
A supervisor crash leaves unreaped children quarantined; no takeover on restart.
"""
import argparse
import sys
import json
import os
from pathlib import Path
import secrets
import shutil
import signal
import subprocess
import tempfile
import time

def executable(server, name):
    return server / "bin" / (name + (".exe" if os.name == "nt" else ""))

class SupervisorDatabase:
    def __init__(self, server_directory, expected_database):
        cfg = json.loads((server_directory / "eqemu_config.json").read_text())["server"]["database"]
        if cfg["db"] != expected_database:
            raise RuntimeError("Configured database does not match explicit expected database")
        self.name = cfg["db"]
        self.client = shutil.which("mariadb") or shutil.which("mysql")
        if not self.client:
            raise RuntimeError("Database client unavailable")
        self.private = tempfile.TemporaryDirectory(prefix="eqemu-supervisor-")
        self.options = Path(self.private.name) / "mysql.cnf"
        def quoted(value):
            return '"' + str(value).replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'
        self.options.write_text("[client]\n" + "\n".join(
            key + "=" + quoted(value) for key, value in {
                "host": cfg["host"], "port": cfg["port"], "user": cfg["username"],
                "password": cfg["password"], "protocol": "TCP"
            }.items()) + "\n")
        self.options.chmod(0o600)

    def query(self, sql):
        result = subprocess.run([self.client, "--defaults-extra-file=" + str(self.options),
            "--batch", "--raw", "--skip-column-names", "--database=" + self.name],
            input=sql, text=True, capture_output=True)
        if result.returncode:
            # SQL/driver stderr can contain private capabilities; never print it.
            raise RuntimeError("Supervisor database operation failed; ownership remains quarantined")
        return result.stdout.strip()

class OwnedZone:
    def __init__(self, db, server_directory, ordinal, journal_directory):
        self.db = db
        self.key = secrets.token_hex(32)
        self.capability = secrets.token_hex(32)  # Parent-only, never passed to child.
        self.process = None
        self.journal = journal_directory / (self.key + ".json")
        self.log = None
        self._write_journal({"process_key": self.key, "state": "birth-recorded"})
        db.query("INSERT INTO housing_inventory_processes(process_key,supervisor_token) VALUES ('" +
            self.key + "',UNHEX('" + self.capability + "'));\n")
        environment = os.environ.copy()
        environment["EQEMU_INVENTORY_SUPERVISED"] = "1"
        environment["EQEMU_INVENTORY_PROCESS_KEY"] = self.key
        try:
            self.log = (server_directory / "logs" / ("local-zone-" + str(ordinal) + ".log")).open("ab")
            self.process = subprocess.Popen([str(executable(server_directory, "zone"))],
                cwd=server_directory, env=environment, stdout=self.log, stderr=subprocess.STDOUT)
        except Exception:
            # No child was created. Delete this unused birth only if no lease or
            # custody link could have bound it; never fabricate a reaped PID.
            db.query("DELETE FROM housing_inventory_processes WHERE process_key='" + self.key +
                "' AND supervisor_token=UNHEX('" + self.capability + "') AND child_pid IS NULL " +
                "AND NOT EXISTS(SELECT 1 FROM housing_inventory_sessions WHERE process_key='" + self.key +
                "') AND NOT EXISTS(SELECT 1 FROM housing_inventory_custody_links WHERE process_key='" + self.key + "');\n")
            raise
        try:
            if db.query("UPDATE housing_inventory_processes SET child_pid=" + str(self.process.pid) +
                " WHERE process_key='" + self.key + "' AND supervisor_token=UNHEX('" + self.capability +
                "') AND reaped=0 AND child_pid IS NULL;\nSELECT ROW_COUNT();\n").splitlines()[-1] != "1":
                raise RuntimeError("Child PID binding refused")
            self._write_journal({"process_key": self.key, "pid": self.process.pid, "state": "spawned"})
        except Exception:
            # Never lose an already spawned child because recording its PID failed.
            self.terminate()
            code = self.process.wait()
            self._write_journal({"process_key": self.key, "pid": self.process.pid,
                "state": "reaped-unpersisted", "exit_code": code})
            if self.log:
                self.log.close()
            raise

    def _write_journal(self, value):
        with self.journal.open("w") as output:
            os.chmod(self.journal, 0o600)
            json.dump(value, output)
            output.flush()
            os.fsync(output.fileno())
        # File fsync is supported on both platforms. Directory fsync is POSIX-only;
        # Windows does not receive a directory-entry crash-durability guarantee.
        # Journal contents never replace the exact parent's owned-child wait proof.
        if os.name == "posix":
            directory = os.open(self.journal.parent, os.O_RDONLY)
            try:
                os.fsync(directory)
            finally:
                os.close(directory)

    def terminate(self):
        if self.process and self.process.poll() is None:
            if os.name == "nt":
                # run-server.cmd gives this direct-parent family a private console.
                # Broadcast Ctrl-C invokes the existing native SIGINT handlers.
                os.kill(0, signal.CTRL_C_EVENT)
            else:
                self.process.terminate()  # Success is NOT a death certificate.

    def finish_owned_child(self):
        if self.process is None:
            raise RuntimeError("No owned child to reap")
        code = self.process.wait()  # Exact owned Popen; no timeout/foreign PID.
        self._write_journal({"process_key": self.key, "pid": self.process.pid,
            "state": "reaped", "exit_code": code})
        # A repeated identical certificate is idempotent; any differing PID or
        # exit code refuses. Proof failure leaves all leases quarantined.
        sql = ("UPDATE housing_inventory_processes SET reaped=1,exit_code=" + str(code) +
            ",reaped_at=COALESCE(reaped_at,NOW()) WHERE process_key='" + self.key + "' AND supervisor_token=UNHEX('" +
            self.capability + "') AND child_pid=" + str(self.process.pid) +
            " AND (reaped=0 OR (reaped=1 AND exit_code=" + str(code) + "));\n" +
            "SELECT COUNT(*) FROM housing_inventory_processes WHERE process_key='" + self.key +
            "' AND supervisor_token=UNHEX('" + self.capability + "') AND child_pid=" + str(self.process.pid) +
            " AND reaped=1 AND exit_code=" + str(code) + ";\n")
        if self.db.query(sql).splitlines()[-1] != "1":
            raise RuntimeError("Exact reaped child certificate did not bind")
        if self.log:
            self.log.close()
        return self.key

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server-directory", type=Path, default=Path(__file__).resolve().parent)
    parser.add_argument("--recovery-executable", type=Path, required=True)
    parser.add_argument("--enable-supervised-inventory", action="store_true")
    parser.add_argument("--zone-count", type=int, choices=range(1, 7), default=3)
    parser.add_argument("--expected-database", required=True)
    args = parser.parse_args()
    if not args.enable_supervised_inventory:
        raise SystemExit("Optional supervised inventory remains disabled")
    if not args.recovery_executable.is_file():
        raise SystemExit("Matching recovery executable required before launching")
    os.umask(0o077)
    server = args.server_directory.resolve()
    (server / "logs").mkdir(exist_ok=True)
    journal = server / "logs" / "inventory-supervisor"
    journal.mkdir(mode=0o700, exist_ok=True)
    if os.name == "nt":
        import ctypes
        if not ctypes.windll.kernel32.GetConsoleWindow():
            raise SystemExit("Windows launcher requires its own interactive console (run-server.cmd)")
    db = SupervisorDatabase(server, args.expected_database)
    # Verify optional schema before creating any child.
    if db.query("SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND engine='InnoDB' AND table_name IN ('housing_inventory_processes','housing_inventory_custody_links','housing_inventory_death_reconciliations');\n") != "3":
        raise SystemExit("Optional supervised recovery schema unavailable")
    children, zones, logs = [], [], []
    stopping = False
    windows_console_interrupt = False
    def stop_signal(_signal, _frame):
        nonlocal stopping, windows_console_interrupt
        stopping = True
        if os.name == "nt" and _signal == signal.SIGINT:
            windows_console_interrupt = True
    signal.signal(signal.SIGTERM, stop_signal)
    signal.signal(signal.SIGINT, stop_signal)
    try:
        for binary in ("loginserver", "world", "ucs"):
            log = (server / "logs" / ("local-" + binary + ".log")).open("ab")
            logs.append(log)
            children.append(subprocess.Popen([str(executable(server, binary))],cwd=server,stdout=log,stderr=subprocess.STDOUT))
            if binary in ("loginserver", "world"):
                import socket
                port = 5998 if binary == "loginserver" else 9001
                for _ in range(120):
                    if stopping or children[-1].poll() is not None:
                        raise RuntimeError("Stack startup interrupted")
                    try:
                        with socket.create_connection(("127.0.0.1", port), timeout=0.25):
                            break
                    except OSError:
                        time.sleep(1)
                else:
                    raise RuntimeError("Stack startup port unavailable")
        for ordinal in range(1, args.zone_count + 1):
            zone = OwnedZone(db, server, ordinal, journal)
            zones.append(zone)
        while not stopping:
            if any(child.poll() is not None for child in children) or any(zone.process.poll() is not None for zone in zones):
                break
            time.sleep(0.25)
    finally:
        if os.name == "nt":
            if not windows_console_interrupt and (any(z.process.poll() is None for z in zones) or any(c.poll() is None for c in children)):
                os.kill(0, signal.CTRL_C_EVENT)
        else:
            for zone in zones:
                zone.terminate()
            for child in children:
                if child.poll() is None:
                    child.terminate()
        failures = False
        for zone in zones:
            try:
                key = zone.finish_owned_child()
                result = subprocess.run([str(args.recovery_executable), key],cwd=server,capture_output=True,text=True)
                if result.returncode:
                    failures = True
            except Exception:
                failures = True  # Still reap all other exact owned children.
        for child in children:
            child.wait()
        for log in logs:
            log.close()
        if failures:
            raise RuntimeError("Reaped process recovery refused; affected actors remain quarantined")

if __name__ == "__main__":
    try:
        main()
    except Exception:
        raise SystemExit("Optional supervisor failed; unreconciled inventory ownership remains quarantined")
