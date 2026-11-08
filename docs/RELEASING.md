# Releasing firmware

A release is a set of signed firmware images on GitHub that the boards install themselves from Home Assistant (each board's **Firmware** update entity). Releases are built and signed **on the PC** with [`scripts/release.py`](../scripts/release.py): the private signing key never goes to GitHub, so control of the GitHub account alone can't put firmware on the boards. GitHub's own "Draft a new release" can't produce a release the boards accept (it can't sign), but editing a published release's title or notes on GitHub is harmless.

## Setup (once per PC, e.g. after a reinstall)

All commands use PlatformIO's Python, written below as `python`. A bare `python` on Windows is usually the Microsoft Store placeholder ("Python was not found"), so type the full path: `~/.platformio/penv/Scripts/python.exe` in Git Bash or Claude Code (`! ~/.platformio/penv/Scripts/python.exe scripts/release.py …`), `%USERPROFILE%\.platformio\penv\Scripts\python` in cmd, `~/.platformio/penv/bin/python` elsewhere.

1. **PlatformIO** installed (it provides that Python).
2. **`cryptography`** in PlatformIO's Python: `python -m pip install cryptography`. A PlatformIO upgrade can reset that environment; the script then says so and prints this command.
3. **GitHub CLI** logged in with repo access: `gh auth login`, check with `gh auth status`.
4. **Signing key** restored to `~/.esp32-led/release-signing-key.pem`, from the Proton Drive backup at `My files\backup\esp32-led\`. **Do not run `keygen` on a new PC.** A new key isn't trusted by the boards (see [Key rotation](#key-rotation)).
5. **Credential headers** `src/config/WifiCreds.h` and `src/config/Mqtt.h`, restored from `backup\esp32-led\config\`. Releases don't need them, but uploads from the PC do (OTA password, and provisioning a board).

## Making a release

1. Get the changes onto `main`, pushed, with **CI green** (the script checks).
2. Pick the version: `vMAJOR.MINOR.PATCH`, higher than the latest (`gh release list`). Patch for fixes, minor for features.
3. Dry run, which builds and signs everything but publishes nothing:
   `python scripts/release.py release v1.2.0 --dry-run`
4. Publish (about 8 minutes, mostly clean builds of every board):
   `python scripts/release.py release v1.2.0`
5. Check what GitHub serves: `python scripts/release.py verify v1.2.0`
6. Install. Each board offers the release within 6 hours; publishing `check` to `homeassistant/light/<client id>/update/command` makes it check now. Press **Install** on the board's Firmware entity (*Settings → Updates*). An install takes about 45 s: the LEDs go dark while the image downloads, then the board reboots and restores its saved light state (on/off, brightness, effect, palette).

## What the script refuses, and the fix

| Message | Fix |
|---|---|
| `working tree has uncommitted changes` | Commit or stash them. Releases are built from the tagged commit only. |
| `tag vX.Y.Z already exists` / `is not newer than the latest release` | Use a higher version. A local leftover tag from an interrupted run: `git tag -d vX.Y.Z` (only if `git ls-remote --tags origin vX.Y.Z` is empty). |
| `not on main` / `HEAD is not pushed` / `CI … still running` / `concluded failure` | Switch to `main`, push, wait for or fix CI. |
| `… does not match the public key in ReleaseKey.h` | Wrong key file. Restore the right one from backup. |
| `image contains …` (a secret) / `is not stamped exactly vX.Y.Z` | Shouldn't happen. A credential header leaked into the clean checkout, or the tag isn't annotated. Investigate, don't work around. |
| `Missing 'cryptography'` | Reinstall it (setup step 2). |

An interrupted run can leave a temporary worktree: `git worktree list`, then `git worktree remove --force <path>`.

## If a release turns out bad

- A board whose new firmware can't reach MQTT within 10 minutes, or keeps rebooting, **rolls back by itself** to the firmware it ran before, and its Firmware entity says so.
- Boards **never downgrade** from a release. Fix forward: publish a higher version with the fix. Deleting a bad release on GitHub only stops boards that haven't installed it yet.
- Emergency: upload a known-good build from the PC (`pio run -t upload -e <board>_ota`). That needs the credential headers.

## Key rotation

Needed only if the private key is lost or exposed. Boards trust the public key compiled into the firmware they run, so a new key needs one PC upload per board.

1. Move the old key out of `~/.esp32-led/`, then `python scripts/release.py keygen`. It refuses to overwrite an existing key.
2. Back up the new key to Proton Drive `backup\esp32-led\`, commit `src/config/ReleaseKey.h`, push.
3. Upload that build to every board from the PC. From then on they accept only releases signed with the new key; old releases fail their signature check.

## Details

- **Signed message:** exact format in the docstring of `scripts/release.py`, mirrored by `src/net/ReleaseFormat.h`. Both must change together; `test/test_release_format` pins them to the published v1.0.0.
- **Boards:** the script's `BOARDS` map lists every board env and its `MQTT_CLIENT_ID`; a new board must be added there, and to the CI matrix.
- **"Latest" release:** boards check GitHub's latest release. A non-firmware release made on GitHub must be marked pre-release or not "latest", or update checks fail until a signed release is latest again.
- **Root certificates:** the firmware trusts the GitHub roots in `src/net/RootCAs.h`. If GitHub changes certificate authority, checks fail with a TLS error; regenerate with `python scripts/gen_root_cas.py` and upload from the PC.

## For Claude

- The **user** runs the publishing step. Claude Code's permission check blocks creating public GitHub releases, so hand over the command. Claude can run `--dry-run` and `verify`, and can trigger `check`/`install` over MQTT.
- **Never run a dry run while the user might be running a real release.** A dry run holds a local `vX.Y.Z` tag for minutes, and a real run in that window refuses with "tag already exists".
