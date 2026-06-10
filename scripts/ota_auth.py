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
