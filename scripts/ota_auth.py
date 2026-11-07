# Reads OTA_PASSWORD from the gitignored WifiCreds.h and passes it to espota,
# so the password lives in exactly one place and never in platformio.ini.
Import("env")
import re

with open("src/config/WifiCreds.h") as f:
    m = re.search(r'#define\s+OTA_PASSWORD\s+"([^"]*)"', f.read())

if m:
    env.Append(UPLOAD_FLAGS=["--auth=" + m.group(1)])
else:
    print("ota_auth.py: OTA_PASSWORD not found in src/config/WifiCreds.h; uploading without auth")


# The platform runs espota with --debug, which logs every option, including
# the password, in plain text. The platform sets those flags after pre-scripts
# run, so strip --debug just before the upload instead. espota still prints
# its invitation/auth/progress lines; only its debug and info logging goes.
# (`pio run -v` still echoes the full upload command, --auth included.)
def _drop_espota_debug(target, source, env):
    env.Replace(UPLOADERFLAGS=[f for f in env["UPLOADERFLAGS"] if f != "--debug"])

env.AddPreAction("upload", _drop_espota_debug)
