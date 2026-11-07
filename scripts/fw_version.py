# Stamps the firmware with `git describe` as FW_VERSION, published as the HA
# device's sw_version. Applied to project sources only (projenv), so a new
# commit recompiles src/ rather than the whole framework.
Import("env", "projenv")
import subprocess

try:
    version = subprocess.check_output(
        ["git", "describe", "--always", "--dirty"],
        cwd=env.subst("$PROJECT_DIR"), text=True).strip()
except Exception:
    version = "unknown"

projenv.Append(CPPDEFINES=[("FW_VERSION", env.StringifyMacro(version))])
