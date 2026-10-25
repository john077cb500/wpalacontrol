# WPalaControl 3.7.6 — Jotul MQTT/TLS experiment

This branch is an ESP8266 experiment, not an official Jotul firmware or a claim
of compatibility with any particular stove. The default PlatformIO environments,
version number 3.7.6 and release workflow remain unchanged. Only
`jotul_mqtt_tls` enables the changes; its firmware identifies itself with
`-jotul-mqtt-tls-experimental` and has a distinct binary filename.

## Build only

```sh
python -m pip install platformio==6.1.18
pio run -e jotul_mqtt_tls
c++ -std=c++11 -Wall -Wextra -Werror test/jotul_policy_test.cpp -o /tmp/jotul-policy-test
/tmp/jotul-policy-test
```

The experimental environment pins the ESP8266 platform and direct libraries.
GitHub Actions builds the binary, checks its size against the board's sketch
budget and uploads the binary and size report as an Actions artifact. It does
not publish a release, upload a filesystem or flash any device. The size check
is a flash budget check, not a guarantee of OTA space or runtime TLS heap.

## Configuration and trust

Use a DNS hostname matching the broker certificate, a TLS listener (usually
8883), and a dedicated nonempty MQTT username/password in the existing local
configuration. Existing configurations retain their configured port; migrate
that port explicitly. TLS 1.2 is required. No plaintext retry or insecure certificate mode exists
in this experimental MQTT transport.

Provision the trusted CA certificate chain as `/mqtt-ca.pem` in the device's
LittleFS using a trusted local administrative process. This change does not
provision it or touch a device. The PEM is public trust material, at most 8192
bytes; never include a private key. No CA, credentials or broker identity are
embedded in this repository or build artifacts. Missing or invalid CA data,
missing credentials, unavailable Wi-Fi or an unsynchronized clock block MQTT.
A parsed certificate must also validate the broker's chain, DNS hostname and
validity interval through BearSSL. The trust object remains alive for the TLS
client's lifetime. Configure the broker to provide intermediates and a chain
compatible with ESP8266 BearSSL; mutual TLS is not implemented.

The firmware starts nonblocking SNTP against `pool.ntp.org` and
`time.cloudflare.com` and requires the time-set callback plus a time at or after
2026-01-01 before connecting. It retries a blocked initial connection every 20
seconds. SNTP is unauthenticated: control DNS/NTP on the trusted device network.
Certificate validation cannot establish trusted time by itself.

## Remote command policy

The experiment intentionally permits MQTT **read commands only**. `SET`, `CMD`,
`EXT`, raw bus commands, stove ignition/shutdown, parameter changes and all MQTT
firmware updates are disabled. The updater has neither an MQTT subscription nor
an MQTT execution path in this build. Home Assistant discovery and MQTT update
checks are suppressed to avoid advertising incompatible control/update entities.
Previously retained discovery entities are not automatically removed; remove
those entries at the broker before using this experiment.

Listen for a non-retained `<base>/session` message at each MQTT connection, then
send a JSON object to the exact `<base>/cmd` topic:

```json
{"session":"<32 hexadecimal characters received on /session>","seq":1,"ts":<current Unix seconds>,"command":"GET STAT"}
```

The example is schematic (`ts` must be a JSON integer). The whole payload is
limited to 256 bytes and JSON nesting to two levels. Session and command are
compared with their full lengths, including any embedded NUL. `seq` is a positive
32-bit integer and must increase within the session. `ts` must be an integer no
more than 30 seconds old and cannot be in the future. Accepted requests are
limited to one per two seconds. Responses use `<base>/result`.

Allowed commands: `GET STAT`, `GET TMPS`, `GET FAND`, `GET CNTR`, `GET TIME`,
`GET SETP`, `GET POWR`, `GET DPRS`. Legacy plain text commands are rejected.
A fresh random 128-bit session value on each connection prevents old retained
commands and cross-reconnection replays. Sequence checks prevent duplicates in
one session. These values are freshness controls, not authentication secrets;
the MQTT library callback does not expose the retained flag. Broker ACLs must
limit publishing to `/cmd` to authorized operators, deny retained command
publishes, and restrict each device to its own unique base topic. A broker or
an authorized publisher remains trusted. TLS protects the connection to that
broker, not end-to-end against it.

## Remaining validation

Host tests exercise the actual command policy: replay, sequence rollback,
expiry/future timestamps, rate limiting, wrong sessions, dangerous commands,
embedded NUL, and millisecond counter rollover. A successful build does not
validate a physical stove or TLS memory behavior on a running ESP8266.

Before any later hardware trial, validate on an isolated bench: valid broker;
wrong hostname; unknown CA; expired/not-yet-valid certificate; missing/invalid
PEM; blocked NTP; reconnect and old retained commands; ACLs and heap headroom;
network loss while serving local functions. Confirm the specific Jotul model's
bus compatibility separately. No hardware trial or flash is performed here.

Local HTTP/UDP APIs, configuration storage, access-point defaults and local OTA
inherit upstream behavior. They are outside this MQTT experiment and must stay
on a trusted network; this change does not make the device Internet-safe.

## Local build results (2026-10-25)

PlatformIO Core 6.1.18, Espressif8266 platform 4.2.1, Arduino core 3.1.2:

| Build | Binary bytes | Linked flash bytes | Static RAM bytes |
| --- | ---: | ---: | ---: |
| Experimental `jotul_mqtt_tls` | 606736 | 602577 / 1044464 | 34220 / 81920 |
| Unmodified upstream `d1_mini` at `5d5765b` | 613872 | 609721 / 1044464 | 33932 / 81920 |

Experimental binary margin: **437728 bytes** below the 1044464-byte board
budget. The experiment also removes discovery/update polling from the active
MQTT path, so these numbers are not a measurement of TLS's isolated code cost.
Static RAM figures exclude live TLS allocations and fragmentation.

The branch's default `d1_mini` build also passes with the same linked flash
and static RAM usage as the unmodified upstream build.
