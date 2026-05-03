import os

target_path = "/usr/local/bin/sectrctl"


def read_env(path: str) -> dict:
    env = {}
    try:
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                if "=" not in line:
                    continue

                key, value = line.split("=", 1)
                value = value.strip().strip('"').strip("'")
                env[key.strip()] = value
    except Exception as e:
        print(f"Error: {e}")
    return env

def EnvSys(path: str) -> str:
    env = read_env(path)
    Dmgr_root = env.get("DMGR_ROOT")
    return Dmgr_root


launcher_loc = EnvSys(".env") + "/Sectrctl/bin/launcher"

content = f"""#!/bin/bash
    {launcher_loc}
"""
with open(target_path, "w") as f:
    f.write(content)

os.chmod(target_path, 0o755)

print(f"Installed command at {target_path}")
