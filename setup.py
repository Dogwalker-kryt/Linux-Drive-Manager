#!/usr/bin/env python3

# Drive Manager setup/cleanup script
# Repo: https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux

import os
import time
import shutil
import subprocess


def detect_package_manager():
    """Detect which package manager is available on the system."""
    for pm in ["apt", "dnf", "yum", "pacman", "zypper"]:
        if shutil.which(pm):
            return pm
    return None


def check_command(cmd, version_arg="--version"):
    try:
        result = subprocess.run([cmd, version_arg], capture_output=True, text=True)
        if result.returncode == 0:
            return True, f"{cmd} is installed: {result.stdout.strip().splitlines()[0]}"
        else:
            return False, f"{cmd} is installed but returned error code {result.returncode}"
    except FileNotFoundError:
        return False, f"{cmd} is not installed"


def ask_install(pkg_manager, package_name):
    print(f"{package_name} is not installed. Do you want to install it? (y/n)")
    choice = input().strip().lower()
    if choice == "y":
        try:
            if pkg_manager == "apt":
                subprocess.run(["sudo", "apt", "install", "-y", package_name])
            elif pkg_manager == "dnf":
                subprocess.run(["sudo", "dnf", "install", "-y", package_name])
            elif pkg_manager == "yum":
                subprocess.run(["sudo", "yum", "install", "-y", package_name])
            elif pkg_manager == "pacman":
                subprocess.run(["sudo", "pacman", "-S", "--noconfirm", package_name])
            elif pkg_manager == "zypper":
                subprocess.run(["sudo", "zypper", "install", "-y", package_name])
            print(f"{package_name} installed successfully.")
        except Exception as e:
            print(f"[ERROR] Failed to install {package_name}: {e}")
    else:
        print(f"Skipping installation of {package_name}.")


def ask_uninstall(pkg_manager, package_name):
    print(f"Do you want to uninstall {package_name}? (y/n)")
    choice = input().strip().lower()
    if choice == "y":
        try:
            if pkg_manager == "apt":
                subprocess.run(["sudo", "apt", "remove", "-y", package_name])
            elif pkg_manager == "dnf":
                subprocess.run(["sudo", "dnf", "remove", "-y", package_name])
            elif pkg_manager == "yum":
                subprocess.run(["sudo", "yum", "remove", "-y", package_name])
            elif pkg_manager == "pacman":
                subprocess.run(["sudo", "pacman", "-R", "--noconfirm", package_name])
            elif pkg_manager == "zypper":
                subprocess.run(["sudo", "zypper", "remove", "-y", package_name])
            print(f"{package_name} uninstalled successfully.")
        except Exception as e:
            print(f"[ERROR] Failed to uninstall {package_name}: {e}")
    else:
        print(f"Keeping {package_name} installed.")


def check_dependencies():
    pkg_manager = detect_package_manager()
    if not pkg_manager:
        print("[ERROR] No supported package manager found.")
        return

    deps = {
        "g++": "g++",
        "openssl": "libssl-dev" if pkg_manager == "apt" else "openssl-devel",
        "smartctl": "smartmontools"
    }

    for cmd, pkg in deps.items():
        installed, msg = check_command(cmd, "--version" if cmd != "openssl" else "version")
        print(msg)
        if not installed:
            ask_install(pkg_manager, pkg)

    print("\nDependency check complete.\n")


def install_Dmgr():
    REPO_DIR = os.path.dirname(os.path.abspath(__file__))
    items = os.listdir(REPO_DIR)
    if len(items) <= 1:
        print("[ERROR] The script can't find files for Drive Manager. "
              "Make sure the script is in the same folder as the Drive Manager files!")
        exit(1)

    HOME_DIR = os.path.expanduser("~")
    base_dir = os.path.join(HOME_DIR, ".local", "share", "DriveMgr")

    # Create directory structure
    dirs = [
        base_dir,
        os.path.join(base_dir, "bin"),
        os.path.join(base_dir, "data"),
        os.path.join(base_dir, "other"),
        os.path.join(base_dir, "bin", "launcher"),
        os.path.join(base_dir, "bin", "bin"),
        os.path.join(base_dir, "bin", "other_src"),
        os.path.join(base_dir, "bin", "Lume"),
    ]
    for d in dirs:
        os.makedirs(d, exist_ok=True)

    time.sleep(0.5)
    print("Directories finished creating\n")
    print("Next: Transporting files\n")

    files_to_copy = [
        ("DriveMgr_CLI", "bin/other_src"),
        ("config.conf", "data"),
        ("launcher/launcher", "bin/launcher"),
        ("launcher/launcher.c", "bin/launcher"),
        ("CODE_OF_CONDUCT.md", "other"),
        ("README.md", "other"),
        ("LICENSE.md", "other"),
        ("SECURITY.md", "other"),
        ("CONTRIBUTING.md", "other"),
        (".github", "other"),
        ("log and key file/key.bin", "data"),
        ("log and key file/log.dat", "data"),
        ("Lume/Lume", "bin/Lume"),
        ("Lume/main_Lume.cpp", "bin/other_src"),
    ]

    for src_rel, dst_rel in files_to_copy:
        src = os.path.join(REPO_DIR, src_rel)
        dst = os.path.join(base_dir, dst_rel)
        try:
            if os.path.isdir(src):
                shutil.copytree(src, os.path.join(dst, os.path.basename(src)), dirs_exist_ok=True)
            else:
                shutil.copy(src, dst)
            print(f"Copied {src_rel} -> {dst_rel}")
        except Exception as e:
            print(f"[ERROR] Failed to copy {src_rel}: {e}\n")

    print("File transferring complete")
    print("Next: Checking dependencies\n")
    check_dependencies()

    print("Do you want to automatically compile the application? (y/n)")
    auto_compiling = input().strip().lower()
    auto_compile = auto_compiling == "y"

    print("Do you want the CLI to be compiled? (y/n)")
    cli_input = input().strip().lower()
    # TODO: implement actual compile logic here


    # After compile, ask about uninstalling deps
    pkg_manager = detect_package_manager()
    if pkg_manager:
        for pkg in ["g++", "libssl-dev" if pkg_manager == "apt" else "openssl-devel", "smartmontools"]:
            ask_uninstall(pkg_manager, pkg)

    if auto_compile:
        if cli_input == "y":
            cli_src = os.path.expanduser("~/.local/share/DriveMgr/bin/other_src/DriveMgr_CLI/src/DriveMgr_experi.cpp")
            cli_out = os.path.expanduser("~/.local/share/DriveMgr/bin/bin/DriveMgr_CLI")

            subprocess.run(["g++", "-std=c++17", cli_src, "-I..", "-o", cli_out, "-lssl", "-lcrypto"])

        elif cli_input == "n":
            print("Skipping CLI compilation")
        else:
            print("Invalid input for CLI compilation, skipping...")


def uninstall_Dmgr():
    print("Do you really want to uninstall Drive Manager? (y/n)\n")
    input_choice_uninstall = input().strip().lower()
    if input_choice_uninstall == "y":
        print("Uninstalling Drive Manager...\n")
        target_dir = os.path.expanduser("~/.local/share/DriveMgr")
        if os.path.exists(target_dir):
            shutil.rmtree(target_dir)
            print("Drive Manager uninstalled successfully\n")
        else:
            print("Drive Manager is not installed\n")
        exit(0)
    elif input_choice_uninstall == "n":
        print("Uninstallation cancelled\n")
        exit(0)
    else:
        print("Invalid input, exiting...\n")
        exit(1)


def main():
    print("Welcome to Drive Manager setup")
    print("––––––––––––––––––––––––––––––––––––––––––––––––––––––")
    print("NOTE:\n")
    print("Make sure the setup script is in the folder with the Drive Manager files!\n")
    print("When reinstalling the program, make sure it is FULLY uninstalled and the dirs etc. are deleted, "
          "otherwise the installation will corrupt.\n")
    print("It is also better if you run the setup with sudo, if you want to add a command to quickly call the application\n")
    print("––––––––––––––––––––––––––––––––––––––––––––––––––––––")
    print("Do you want to install or uninstall Drive Manager? (inst/uninst)\n")
    input_choice_install_uninstall = input().strip().lower()
    if input_choice_install_uninstall == "inst":
        install_Dmgr()
    elif input_choice_install_uninstall == "uninst":
        uninstall_Dmgr()
    else:
        print("Invalid input, exiting...")
        exit(1)

if __name__ == "__main__":
    main()
