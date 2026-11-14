# Server and utility network access

These are the default **host** ports in `compose.yaml` and `compose.utilities.yaml`. Optional utility ports appear only when their services run. This guide does not change the host firewall or router.

| Purpose | Protocol | Default host ports |
| --- | --- | --- |
| RoF2/SoD login | UDP | 5999 |
| Legacy login clients | UDP | 5998 |
| World game connection | UDP | 9000 |
| Zone game connections | UDP | 7000–7010 |
| Universal chat/mail | UDP | 7778 |
| Spire web administration | TCP | 3000 |
| SSH / SFTP | TCP | 2222 |
| Explicit FTP over TLS control | TCP | 2121 |
| Passive FTP over TLS data | TCP | 30000–30009 |
| phpMyAdmin | TCP | 8080 |
| PEQ Editor | TCP | 8081 |
| Scheduled local backups | — | None |

## Bind the ports to a reachable address

The defaults bind to `127.0.0.1`. To access the stack from other computers, set `BIND_ADDRESS` and `UTILITY_BIND_ADDRESS` in the private `.env` to the Docker host's LAN IP (or `0.0.0.0` for all IPv4 interfaces). `PUBLIC_ADDRESS` is the IP/DNS name game clients use, `LOCAL_ADDRESS` is the LAN address, and `FTP_PUBLIC_HOST` is the address reachable by FTPS clients. These settings do not open a firewall. Recreate the affected containers after changing port bindings.

If `SPIRE_PORT`, `SSH_PORT`, `FTP_PORT`, `PHPMYADMIN_PORT` or `PEQ_EDITOR_PORT` is customized, use the configured **host** value in firewall/router rules and client addresses. Passive FTPS uses the fixed TCP 30000–30009 range. If the Zone range is changed, update both the server configuration and Compose UDP mapping, plus the firewall/router range.

## Allow the selected services through the network

For LAN access, permit the game UDP ports from player networks and the selected utility TCP ports from the administrative network in the host firewall. Any intervening network firewall must allow the same traffic. Docker publishes ports using its own firewall rules; check the effective Docker/firewall policy as well as any firewall frontend rules.

For Internet game access, forward UDP 5999, 9000, 7000–7010 and 7778 on the router to the Docker host's fixed LAN IP. Forward UDP 5998 too when supporting legacy login clients. Clients must use the correct public address. Remote utilities can use a VPN or SSH tunnel; direct remote utility access also requires forwarding their selected TCP ports. FTPS needs both its control port and its complete passive data range, with `FTP_PUBLIC_HOST` set to the externally reachable address.

With the default utility host ports, browse `http://HOST:3000` for Spire, `http://HOST:8080` for phpMyAdmin and `http://HOST:8081` for PEQ Editor. Connect SSH/SFTP to `HOST:2222`; connect FTPS to `HOST:2121` with explicit TLS and passive mode. Normal authentication remains required.

## Internal connections

MariaDB TCP 3306, World interserver TCP 9001 and login registration TCP 5998 are internal container connections in this package, not client-facing host mappings. World management TCP 9000 is a different protocol from the public game UDP 9000; Spire's local World management connection must stay inside its server network namespace. Opening TCP 9000 on the router does not enable the game UDP listener. No inbound port is needed for local scheduled backups.

## Check the effective mappings

Run `docker compose --profile utilities ps` to see the actual published host bindings, then check game connectivity from a player machine and each enabled utility from an administrative machine. A published mapping alone does not prove access through a router/firewall. No host firewall, host SSH daemon or router configuration is modified by the setup helpers.
