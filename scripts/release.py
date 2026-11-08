"""Signed firmware releases for ESP32-LED.

Run with PlatformIO's Python, which has PlatformIO; add `cryptography` once:
    Windows:     %USERPROFILE%\\.platformio\\penv\\Scripts\\python -m pip install cryptography
    Linux/macOS: ~/.platformio/penv/bin/python -m pip install cryptography

    python scripts/release.py keygen                    # once: create the signing key
    python scripts/release.py release v1.2.0 --dry-run  # build + sign, publish nothing
    python scripts/release.py release v1.2.0            # build, sign, publish
    python scripts/release.py verify v1.2.0             # check a published release
    python scripts/release.py verify <dir>              # check a local release dir

The private key lives outside the repo (~/.esp32-led/release-signing-key.pem)
and never goes to GitHub, so someone with access to the GitHub account can
publish files but not sign them. Boards accept an image only if its signature
verifies against the public key compiled into them (src/config/ReleaseKey.h).

The signed message (UTF-8, exactly these lines, each ending in "\\n"):
    esp32-led-firmware
    board=<MQTT client id, e.g. esp32_desk_led>
    version=<release tag, e.g. v1.2.0>
    size=<image size in bytes>
    sha256=<lowercase hex SHA-256 of the image>
signed with ECDSA P-256 over SHA-256. manifest.json carries the DER signature
in base64. Binding board and version stops an image being installed on the
wrong board or replayed as a different version.
"""

import argparse
import base64
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

try:
    from cryptography.exceptions import InvalidSignature
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import ec
except ImportError:
    sys.exit(f"Missing 'cryptography'. Install it with:\n  {sys.executable} -m pip install cryptography")

REPO = Path(__file__).resolve().parent.parent
KEY_PATH = Path.home() / ".esp32-led" / "release-signing-key.pem"
PUBKEY_HEADER = REPO / "src" / "config" / "ReleaseKey.h"
OUT_ROOT = REPO / ".pio" / "release"
GH_REPO = "mladBits/ESP32-LED"

# PlatformIO env -> board identity (MQTT_CLIENT_ID in src/config/Config.h).
BOARDS = {
    "peg_esp32": "esp32_peg_led",
    "desk_esp32": "esp32_desk_led",
    "wall_esp32": "esp32_wall_led",
}

# Values that must never appear in a published image. MQTT_USER is left out:
# it can legitimately appear inside topic strings (e.g. "homeassistant/...").
SECRET_MACROS = ("WIFI_SSID", "WIFI_PASSWORD", "OTA_PASSWORD", "MQTT_SERVER", "MQTT_PASSWORD")

VERSION_RE = re.compile(r"^v(\d+)\.(\d+)\.(\d+)$")


def run(cmd, cwd=REPO, capture=True):
    result = subprocess.run(cmd, cwd=cwd, text=True, capture_output=capture)
    if result.returncode != 0:
        detail = (result.stderr or result.stdout or "").strip() if capture else ""
        raise SystemExit(f"Command failed: {' '.join(map(str, cmd))}\n{detail}")
    return result.stdout.strip() if capture else ""


def signed_message(board: str, version: str, size: int, sha256_hex: str) -> bytes:
    return (f"esp32-led-firmware\nboard={board}\nversion={version}\n"
            f"size={size}\nsha256={sha256_hex}\n").encode()


def header_public_key():
    """The public key the firmware embeds, parsed back out of ReleaseKey.h."""
    if not PUBKEY_HEADER.exists():
        raise SystemExit(f"{PUBKEY_HEADER.relative_to(REPO)} is missing; run `keygen` first")
    literals = re.findall(r'"((?:[^"\\]|\\.)*)"', PUBKEY_HEADER.read_text())
    pem = "".join(literals).replace("\\n", "\n")
    return serialization.load_pem_public_key(pem.encode())


def load_private_key():
    if not KEY_PATH.exists():
        raise SystemExit(f"No signing key at {KEY_PATH}; run `keygen` (or restore it from backup)")
    return serialization.load_pem_private_key(KEY_PATH.read_bytes(), password=None)


def public_pem(key) -> bytes:
    """PEM of the public key, given either half of the key pair."""
    if isinstance(key, ec.EllipticCurvePrivateKey):
        key = key.public_key()
    return key.public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo)


def local_secrets() -> dict:
    """Secret values from the gitignored local headers, for the leak scan."""
    secrets = {}
    for name in ("WifiCreds.h", "Mqtt.h"):
        path = REPO / "src" / "config" / name
        if path.exists():
            for macro, value in re.findall(r'#define\s+(\w+)\s+"([^"]*)"', path.read_text()):
                if macro in SECRET_MACROS and value:
                    secrets[macro] = value
    return secrets


def parse_version(tag: str):
    m = VERSION_RE.match(tag)
    return tuple(int(x) for x in m.groups()) if m else None


# --- keygen ------------------------------------------------------------------

def cmd_keygen(_args):
    if KEY_PATH.exists():
        raise SystemExit(f"{KEY_PATH} already exists; refusing to overwrite the signing key.\n"
                         "Rotating keys means flashing every board (from the PC) with the new public key.")
    key = ec.generate_private_key(ec.SECP256R1())
    KEY_PATH.parent.mkdir(parents=True, exist_ok=True)
    KEY_PATH.write_bytes(key.private_bytes(serialization.Encoding.PEM,
                                           serialization.PrivateFormat.PKCS8,
                                           serialization.NoEncryption()))
    try:
        KEY_PATH.chmod(0o600)
    except OSError:
        pass

    pem_lines = public_pem(key).decode().strip().splitlines()
    body = "\n".join(f'    "{line}\\n"' for line in pem_lines)
    PUBKEY_HEADER.write_text(
        "#pragma once\n\n"
        "// Public half of the release signing key, generated by\n"
        "// `scripts/release.py keygen`; the private half never leaves the release PC.\n"
        "// Boards accept a downloaded firmware image only if its signature verifies\n"
        "// against this key. The signed message format is in scripts/release.py.\n"
        f"static const char RELEASE_PUBLIC_KEY_PEM[] =\n{body};\n")

    print(f"Private key: {KEY_PATH}  (back this up somewhere safe; never commit it)")
    print(f"Public key:  {PUBKEY_HEADER.relative_to(REPO)}  (commit this)")


# --- release -----------------------------------------------------------------

def check_preconditions(version: str, dry_run: bool):
    problems, warnings = [], []
    if parse_version(version) is None:
        raise SystemExit(f"Version must look like v1.2.3, got {version!r}")

    if run(["git", "status", "--porcelain", "--untracked-files=no"]):
        problems.append("working tree has uncommitted changes")
    if run(["git", "tag", "--list", version]):
        problems.append(f"tag {version} already exists locally")

    existing = [t for t in run(["git", "tag", "--list", "v*"]).split() if parse_version(t)]
    if existing:
        latest = max(existing, key=parse_version)
        if parse_version(version) <= parse_version(latest):
            problems.append(f"{version} is not newer than the latest release {latest}")

    # Publishing needs the commit on GitHub with green CI; a dry run only warns.
    sha = run(["git", "rev-parse", "HEAD"])
    run(["git", "fetch", "-q", "origin"])
    publish_checks = []
    if run(["git", "rev-parse", "--abbrev-ref", "HEAD"]) != "main":
        publish_checks.append("not on main")
    if run(["git", "rev-parse", "origin/main"]) != sha:
        publish_checks.append("HEAD is not pushed to origin/main")
    if run(["git", "ls-remote", "--tags", "origin", version]):
        publish_checks.append(f"tag {version} already exists on GitHub")
    runs = json.loads(run(["gh", "run", "list", "-R", GH_REPO, "--workflow", "ci.yml", "--commit", sha,
                           "--json", "status,conclusion", "--limit", "1"]) or "[]")
    if not runs:
        publish_checks.append("no CI run for HEAD (push first)")
    elif runs[0]["status"] != "completed":
        publish_checks.append("CI for HEAD is still running")
    elif runs[0]["conclusion"] != "success":
        publish_checks.append(f"CI for HEAD concluded {runs[0]['conclusion']}")
    (warnings if dry_run else problems).extend(publish_checks)

    # Signing with a key the boards don't trust would produce a useless release.
    if public_pem(load_private_key()) != public_pem(header_public_key()):
        problems.append(f"{KEY_PATH.name} does not match the public key in {PUBKEY_HEADER.name}")

    for w in warnings:
        print(f"warning (dry run, would block publishing): {w}")
    if problems:
        raise SystemExit("Cannot release:\n  - " + "\n  - ".join(problems))
    return sha


def build_and_sign(version: str, out_dir: Path) -> dict:
    key = load_private_key()
    secrets = local_secrets()
    if not secrets:
        print("warning: no local credential headers found, skipping the secret scan")

    worktree = Path(tempfile.mkdtemp(prefix="esp32-led-release-"))
    shutil.rmtree(worktree)  # git worktree add wants to create the directory itself
    run(["git", "worktree", "add", "--detach", str(worktree), version])
    try:
        # A clean checkout has no gitignored credential headers, so this is
        # exactly the secret-free build CI checks on every push.
        for name in ("WifiCreds.h", "Mqtt.h"):
            if (worktree / "src" / "config" / name).exists():
                raise SystemExit(f"{name} is present in the release checkout; refusing to build")

        envs = [arg for env in BOARDS for arg in ("-e", env)]
        print(f"Building {', '.join(BOARDS)} at {version} (clean checkout, no credential headers)...")
        run([sys.executable, "-m", "platformio", "run", "-d", str(worktree), *envs], capture=False)

        out_dir.mkdir(parents=True, exist_ok=True)
        manifest = {"format": 1, "version": version, "boards": {}}
        for env, board in BOARDS.items():
            image = (worktree / ".pio" / "build" / env / "firmware.bin").read_bytes()

            leaked = [m for m, v in secrets.items() if v.encode() in image]
            if leaked:
                raise SystemExit(f"{env}: image contains {', '.join(leaked)}; refusing to release")
            if version.encode() + b"\0" not in image:
                raise SystemExit(f"{env}: image is not stamped exactly {version} (dirty or untagged build?)")

            digest = hashlib.sha256(image).hexdigest()
            signature = key.sign(signed_message(board, version, len(image), digest),
                                 ec.ECDSA(hashes.SHA256()))
            filename = f"{board}-{version}.bin"
            (out_dir / filename).write_bytes(image)
            manifest["boards"][board] = {
                "file": filename,
                "size": len(image),
                "sha256": digest,
                "sig": base64.b64encode(signature).decode(),
            }
            print(f"  {board}: {len(image)} bytes, sha256 {digest[:16]}..., signed")

        (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        return manifest
    finally:
        run(["git", "worktree", "remove", "--force", str(worktree)])


def release_notes(version: str) -> str:
    tags = sorted((t for t in run(["git", "tag", "--list", "v*"]).split()
                   if parse_version(t) and t != version), key=parse_version)
    span = f"{tags[-1]}..{version}" if tags else version
    log = run(["git", "log", "--no-merges", "--format=- %s", span])
    if not tags:
        log = "First signed release."
    return (f"{log}\n\nEach board's image is signed with the release key (ECDSA P-256); "
            "boards verify the signature, board and version before installing.")


def cmd_release(args):
    version = args.version
    check_preconditions(version, args.dry_run)
    out_dir = OUT_ROOT / version

    # describe needs an annotated tag to stamp the image with the version.
    run(["git", "tag", "-a", version, "-m", version])
    pushed = False
    try:
        manifest = build_and_sign(version, out_dir)
        verify_dir(out_dir)
        if args.dry_run:
            print(f"\nDry run complete: signed files in {out_dir}\nNothing was published.")
            return
        run(["git", "push", "origin", version])
        pushed = True
        files = [str(out_dir / b["file"]) for b in manifest["boards"].values()] + [str(out_dir / "manifest.json")]
        run(["gh", "release", "create", version, "-R", GH_REPO, "--verify-tag",
             "--title", version, "--notes", release_notes(version), *files])
        print(f"\nPublished https://github.com/{GH_REPO}/releases/tag/{version}")
    finally:
        if not pushed:
            run(["git", "tag", "-d", version])


# --- verify ------------------------------------------------------------------

def verify_dir(directory: Path) -> bool:
    public_key = header_public_key()
    manifest = json.loads((directory / "manifest.json").read_text())
    version, ok = manifest["version"], True
    for board, entry in manifest["boards"].items():
        image = (directory / entry["file"]).read_bytes()
        digest = hashlib.sha256(image).hexdigest()
        problems = []
        if len(image) != entry["size"]:
            problems.append("size mismatch")
        if digest != entry["sha256"]:
            problems.append("sha256 mismatch")
        try:
            public_key.verify(base64.b64decode(entry["sig"]),
                              signed_message(board, version, len(image), digest),
                              ec.ECDSA(hashes.SHA256()))
        except InvalidSignature:
            problems.append("BAD SIGNATURE")
        print(f"  {board} {version}: {'OK' if not problems else ', '.join(problems)}")
        ok &= not problems
    return ok


def cmd_verify(args):
    target = Path(args.target)
    if not target.is_dir():
        if parse_version(args.target) is None:
            raise SystemExit(f"{args.target} is neither a directory nor a version tag")
        target = Path(tempfile.mkdtemp(prefix="esp32-led-verify-"))
        run(["gh", "release", "download", args.target, "-R", GH_REPO, "-D", str(target)])
    print(f"Verifying {target}:")
    if not verify_dir(target):
        raise SystemExit("Verification FAILED")
    print("All signatures valid.")


def main():
    parser = argparse.ArgumentParser(description="Signed firmware releases for ESP32-LED")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("keygen", help="create the release signing key (once)").set_defaults(func=cmd_keygen)
    rel = sub.add_parser("release", help="build, sign and publish a release")
    rel.add_argument("version", help="release tag, e.g. v1.2.0")
    rel.add_argument("--dry-run", action="store_true", help="build and sign, publish nothing")
    rel.set_defaults(func=cmd_release)
    ver = sub.add_parser("verify", help="check a release's signatures")
    ver.add_argument("target", help="version tag (downloads from GitHub) or local release directory")
    ver.set_defaults(func=cmd_verify)
    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
