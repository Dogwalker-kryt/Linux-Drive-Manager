#!/usr/bin/env python3

# ========== DriveMgr Setup Script ==========
# Copyright (C) 2024 Dogwalker-kryt
# Part of DriveMgr (https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux)
# Licensed under GNU General Public License v3

import os
import sys
import time
import shutil
import subprocess


# ============= EnvSys =============

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


# ========== Configuration ==========

PKG_MANAGER_CMD = {
    "apt": {"install": ["sudo", "apt", "install", "-y"], "remove": ["sudo", "apt", "remove", "-y"]},
    "dnf": {"install": ["sudo", "dnf", "install", "-y"], "remove": ["sudo", "dnf", "remove", "-y"]},
    "yum": {"install": ["sudo", "yum", "install", "-y"], "remove": ["sudo", "yum", "remove", "-y"]},
    "pacman": {"install": ["sudo", "pacman", "-S", "--noconfirm"], "remove": ["sudo", "pacman", "-R", "--noconfirm"]},
    "zypper": {"install": ["sudo", "zypper", "install", "-y"], "remove": ["sudo", "zypper", "remove", "-y"]},
}

DEPENDENCIES = {
    "g++": "g++",
    "openssl": "libssl-dev",
    "smartctl": "smartmontools"
}

# Mark which dependencies are packages (vs command binaries)
PACKAGE_DEPS = {"openssl", "smartctl"}
COMMAND_DEPS = {"g++"}

# ========== Utility Functions ==========

def detect_package_manager():
    """Detect which package manager is available on the system."""
    for pm in list(PKG_MANAGER_CMD.keys()):
        if shutil.which(pm):
            return pm
    return None


def check_command(cmd, version_arg="--version"):
    """Check if a command is installed and get its version."""
    try:
        result = subprocess.run([cmd, version_arg], capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            ver_line = result.stdout.strip().split('\n')[0]
            return True, f"{cmd}: {ver_line}"
        return False, f"{cmd}: error (exit code {result.returncode})"
    except FileNotFoundError:
        return False, f"{cmd}: not found"
    except subprocess.TimeoutExpired:
        return False, f"{cmd}: timeout"
    except Exception as e:
        return False, f"{cmd}: {str(e)}"


def check_package_installed(pkg_manager, package_name):
    """Check if a package is installed using the package manager."""
    try:
        if pkg_manager == "apt":
            # Use dpkg to check for installed packages
            result = subprocess.run(["dpkg", "-l"], capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and package_name in result.stdout:
                return True, f"{package_name}: installed"
            return False, f"{package_name}: not installed"
        
        elif pkg_manager == "dnf":
            result = subprocess.run(["dnf", "list", "installed"], capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and package_name in result.stdout:
                return True, f"{package_name}: installed"
            return False, f"{package_name}: not installed"
        
        elif pkg_manager == "yum":
            result = subprocess.run(["yum", "list", "installed"], capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and package_name in result.stdout:
                return True, f"{package_name}: installed"
            return False, f"{package_name}: not installed"
        
        elif pkg_manager == "pacman":
            result = subprocess.run(["pacman", "-Q", package_name], capture_output=True, timeout=10)
            if result.returncode == 0:
                return True, f"{package_name}: installed"
            return False, f"{package_name}: not installed"
        
        elif pkg_manager == "zypper":
            result = subprocess.run(["zypper", "search", "-i", package_name], capture_output=True, text=True, timeout=10)
            if result.returncode == 0 and package_name in result.stdout:
                return True, f"{package_name}: installed"
            return False, f"{package_name}: not installed"
        
        return False, f"{package_name}: unknown package manager"
    
    except subprocess.TimeoutExpired:
        return False, f"{package_name}: check timed out"
    except Exception as e:
        return False, f"{package_name}: check failed ({str(e)})"


def ask_user(question, default="n"):
    """Prompt user for yes/no input."""
    prompt = f"{question} (y/n, default={default}): ".strip()
    while True:
        choice = input(prompt).strip().lower() or default
        if choice in ["y", "n"]:
            return choice == "y"
        print("Invalid input. Please enter 'y' or 'n'.")


def install_package(pkg_manager, package_name):
    """Install a package using the detected package manager."""
    if not pkg_manager or pkg_manager not in PKG_MANAGER_CMD:
        print(f"[ERROR] Unknown package manager: {pkg_manager}")
        return False
    
    cmd = PKG_MANAGER_CMD[pkg_manager]["install"] + [package_name]
    try:
        result = subprocess.run(cmd, timeout=300)
        return result.returncode == 0
    except subprocess.TimeoutExpired:
        print(f"[ERROR] Installation of {package_name} timed out")
        return False
    except Exception as e:
        print(f"[ERROR] Failed to install {package_name}: {e}")
        return False


def uninstall_package(pkg_manager, package_name):
    """Uninstall a package using the detected package manager."""
    if not pkg_manager or pkg_manager not in PKG_MANAGER_CMD:
        print(f"[ERROR] Unknown package manager: {pkg_manager}")
        return False
    
    cmd = PKG_MANAGER_CMD[pkg_manager]["remove"] + [package_name]
    try:
        result = subprocess.run(cmd, timeout=300)
        return result.returncode == 0
    except subprocess.TimeoutExpired:
        print(f"[ERROR] Uninstallation of {package_name} timed out")
        return False
    except Exception as e:
        print(f"[ERROR] Failed to uninstall {package_name}: {e}")
        return False

# ========== Dependency Management ==========


def check_dependencies(pkg_manager):
    """Check and optionally install required dependencies."""
    if not pkg_manager:
        print("[ERROR] No supported package manager found")
        return False
    
    print("\n[Checking Dependencies]")
    missing_deps = []
    installed_deps = []
    
    for cmd, pkg_name in DEPENDENCIES.items():
        # Use package check for packages, command check for binaries
        if cmd in PACKAGE_DEPS:
            installed, msg = check_package_installed(pkg_manager, pkg_name)
        else:
            installed, msg = check_command(cmd)
        print(f"  {msg}")
        if installed:
            installed_deps.append(cmd)
        else:
            missing_deps.append((cmd, pkg_name))
    
    if not missing_deps:
        print("All dependencies satisfied.\n")
        return True
    
    print(f"\n{len(missing_deps)} missing dependencies found")
    if ask_user("Install them now?", "y"):
        for cmd, pkg_name in missing_deps:
            print(f"Installing {pkg_name}...")
            if install_package(pkg_manager, pkg_name):
                print(f"  ✓ {pkg_name} installed")
            else:
                print(f"  ✗ {pkg_name} failed")
                return False
    else:
        print("Skipping dependency installation")
        return False
    
    return True

# ========== Installation Functions ==========


def create_directory_structure(base_dir):
    """Create required directory structure."""
    dirs = [
        base_dir,
        os.path.join(base_dir, "bin"),
        os.path.join(base_dir, "data"),
        os.path.join(base_dir, "other"),
        os.path.join(base_dir, "bin/launcher"),
        os.path.join(base_dir, "bin/bin"),
        os.path.join(base_dir, "bin/other_src"),
        os.path.join(base_dir, "bin/Lume"),
    ]
    for d in dirs:
        os.makedirs(d, exist_ok=True)
    print("[✓] Directory structure created")


def copy_files(repo_dir, base_dir):
    """Copy project files to installation directory."""
    files_to_copy = [
        ("DriveMgr_CLI", "bin/other_src"),
        ("config.conf", "data"),
        ("launcher/launcher", "bin/launcher"),
        ("launcher/launcher.cpp", "bin/launcher"),
        ("CODE_OF_CONDUCT.md", "other"),
        ("README.md", "other"),
        ("LICENSE", "other"),
        ("SECURITY.md", "other"),
        ("CONTRIBUTING.md", "other"),
        (".github", "other"),
        ("log and key file/log.dat", "data"),
        ("Lume/Lume", "bin/Lume"),
        ("Lume/main_Lume.cpp", "bin/other_src"),
    ]
    
    failed = []
    for src_rel, dst_rel in files_to_copy:
        src = os.path.join(repo_dir, src_rel)
        dst = os.path.join(base_dir, dst_rel)
        try:
            if os.path.isdir(src):
                shutil.copytree(src, os.path.join(dst, os.path.basename(src)), dirs_exist_ok=True)
                print(f"  {src_rel} → {dst_rel}")
            else:
                shutil.copy2(src, dst)
                print(f"  {src_rel} → {dst_rel}")
        except Exception as e:
            print(f"  [!] {src_rel}: {e}")
            failed.append(src_rel)
    
    if failed:
        print(f"\n[!] {len(failed)} file(s) failed to copy")
        return False
    print("[✓] Files copied successfully")
    return True


def compile_cli(base_dir, auto_compile=False):
    """Compile the CLI application."""
    cli_src_dir = os.path.join(base_dir, "bin/other_src/DriveMgr_CLI")
    
    if not auto_compile:
        if not ask_user("Compile CLI now?", "y"):
            print("Skipping compilation")
            return True
    
    print("[Compiling CLI]")
    try:
        result = subprocess.run(["make", "-C", cli_src_dir], timeout=600)
        if result.returncode == 0:
            print("[✓] CLI compiled successfully")
            return True
        
        else:
            print("[✗] Compilation failed")
            return False
        
    except subprocess.TimeoutExpired:
        print("[✗] Compilation timed out")
        return False
    
    except Exception as e:
        print(f"[✗] Compilation error: {e}")
        return False

def move_compiled_cli(base_dir):
    """Move compiled CLI binary to final location."""
    cli_src = os.path.join(base_dir, "bin/other_src/DriveMgr_CLI/DMgr_CLI")
    cli_dst = os.path.join(base_dir, "bin/bin/DMgr_CLI")
    
    if not os.path.exists(cli_src):
        print("[ERROR] Compiled CLI binary not found")
        return False
    
    try:
        shutil.move(cli_src, cli_dst)
        print(f"[✓] CLI moved to {cli_dst}")
        return True
    
    except Exception as e:
        print(f"[ERROR] Failed to move CLI: {e}")
        return False

def install_Dmgr():
    """Main installation function."""
    print("\n[Installing Drive Manager]")
    
    # Validate repo structure
    repo_dir = os.path.dirname(os.path.abspath(__file__))
    if len(os.listdir(repo_dir)) <= 1:
        print("[ERROR] Script location invalid. Must be in DriveMgr repo folder")
        return
    
    # Setup installation directories from EnvSys
    env_path = EnvSys(".env")
    if env_path:
        if "~" in env_path:
            env_path = os.path.expanduser(env_path)
        base_dir = os.path.join(env_path, "DriveMgr")
    else:
        # Fallback to default location
        home_dir = os.path.expanduser("~")
        base_dir = os.path.join(home_dir, ".local/share/DriveMgr")
    
    print(f"Installing to: {base_dir}")
    
    # Check if already installed
    if os.path.exists(base_dir):

        if not ask_user("DriveMgr already installed. Reinstall?", "n"):

            print("Installation cancelled")
            return
        
        shutil.rmtree(base_dir)
    
    # Execute installation steps
    create_directory_structure(base_dir)
    time.sleep(0.3)
    
    print("\n[Copying files]")

    if not copy_files(repo_dir, base_dir):
        print("[ERROR] Installation aborted due to copy errors")
        return
    
    # Handle dependencies
    pkg_manager = detect_package_manager()
    if pkg_manager:

        if check_dependencies(pkg_manager):

            compile_success = compile_cli(base_dir)

            if not compile_success:
                print("[!] Binary couldnt be moved due to compilation failure")
                return
            
            move_compiled_cli(base_dir)
    else:
        print("[!] Package manager not found, skipping dependency check")

        if ask_user("Continue without dependency check?", "n"):
            compile_cli(base_dir)
    
    print("\n[✓] Installation complete!")

# ========== Uninstallation Functions ==========


def uninstall_Dmgr():
    """Uninstall DriveMgr."""
    print("\n[Uninstalling Drive Manager]")
    
    # Get target directory from EnvSys
    env_path = EnvSys(".env")
    if env_path:
        if "~" in env_path:
            env_path = os.path.expanduser(env_path)
        target_dir = os.path.join(env_path, "DriveMgr")
    else:
        # Fallback to default location
        target_dir = os.path.expanduser("~/.local/share/DriveMgr")
    
    if not os.path.exists(target_dir):
        print("[!] DriveMgr is not installed")
        return
    
    print(f"Target: {target_dir}")

    if ask_user("Permanently delete DriveMgr?", "n"):

        try:

            shutil.rmtree(target_dir)
            print("[✓] DriveMgr uninstalled successfully")

        except Exception as e:

            print(f"[ERROR] Uninstallation failed: {e}")

    else:
        print("Uninstallation cancelled")

# ========== Main Menu ==========

def main():
    """Main entry point."""
    print("╔════════════════════════════════════════════════════════╗")
    print("║         DriveMgr Installation & Setup Script           ║")
    print("╚════════════════════════════════════════════════════════╝")
    print("\n[Prerequisites]")
    print("  • Script must be in the DriveMgr repository folder")
    print("  • Run with 'sudo' if you want system-wide integration")
    print("  • Full uninstall before reinstalling is recommended\n")
    
    while True:
        print("What would you like to do?")
        print("  (i)nstall   - Install DriveMgr")
        print("  (u)ninstall - Remove DriveMgr")
        print("  (q)uit      - Exit\n")
        
        choice = input("Choice: ").strip().lower()
        
        if choice in ["i", "install"]:
            install_Dmgr()
            break
        elif choice in ["u", "uninstall"]:
            uninstall_Dmgr()
            break
        elif choice in ["q", "quit"]:
            sys.exit(0)
        else:
            print("[!] Invalid choice. Try again.\n")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n[!] Interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n[ERROR] Unexpected error: {e}")
        sys.exit(1)