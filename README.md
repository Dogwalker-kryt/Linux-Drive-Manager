# Drive Manager Linux

```text
                   ██╗ 
██████╗            ╚═╝            █████╗    ███╗   ██╗   ██████╗      
██╔══██╗   ██████╗ ██╗ ██╗   ██╗ █╗    █    ████╗ ████║ █╗     █  ██████╗
██║   ██║  ██╔═══╝ ██║ ╚██╗ ██╔╝ ██████╗    ██╔████╔██║  ██████╝  ██╔═══╝
██║   ██║  ██║     ██║  ╚████╔╝  █╗════╝    ██║╚██╔╝██║       ██╗ ██║      
██████╔╝   ██║     ██║   ╚██╔╝    █████╗    ██║ ╚═╝ ██║  ██████╔╝ ██║ 
╚═════╝    ╚═╝     ╚═╝    ╚═╝     ╚════╝    ╚═╝     ╚═╝  ╚═════╝  ╚═╝

```

A terminal-based drive management tool for Linux (Debian-based), written primarily in C++. This project provides a straightforward interface for managing storage devices — HDDs, SSDs, and more — directly from the command line.

---

## Version Control

**CLI:**

- Stable: `v0.8.88-08`  
  _(Stable version is functional but serves mostly as a safe baseline — the experimental version is recommended for more features)_
  
- Experimental: `v0.9.10-86`  
  _(New Fingerprinting and benchmakring functions. fingerprinting is working, while benchmakring is in development)_

**GUI (Not getting Updated anymore):**

- Rust GUI: `v0.1.5-alpha` (Lists drives; not feature complete)
- C++ GUI: `v0.1.1-alpha`  
  _(GUI versions lag behind CLI/TUI)_

**TUI (Experimental/In Progress):**

- The TUI may be unstable depending on system and configuration.
- TUI elements will be implmented in CLI version

**General Notes:**
- Check for new releases regularly.
- **GUI** -> discontiued

	-> _(not recommended to use!)_
  
- **TUI** -> mixed with CLI

	-> _(not recommended to use!)_
  
- **CLI** -> Main programm and getting TUI 	

	-> _(use this)_

---

## Warning

**This tool is for advanced users only!** Formatting, encrypting, or modifying drives is inherently risky.  
- Always back up your data.
- Do not type arbitrary or dangerous input at prompts (such as `/dev/sda rm -rf`).  
- Double-check all device paths before confirming any destructive operation.

---

## Currently in Development

- TUI and CLI enhancements
- Architecture documentaion

See [issues](https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux/issues) and [projects](https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux/projects) for more.

---

## Project Structure

```
/DriveMgr_CLI      - CLI source code (C++)
    /include       - Headers (.h)
    /src           - Core sources
/DriveMgr_GUI      - GUI version (Rust/C++)
/DriveMgr_TUI      - TUI version (WIP)
build_src.sh       - Build/install script
bin/               - Built executables
config.conf        - Example config
```

---

## Features

- List all drives and partitions
- Format, label, and set filesystem type
- Drive encryption (AES-256) and decryption
- Partition resizing
- S.M.A.R.T. drive health checking
- Partition management
- Data overwrite/wiping
- Change/history log (aka. Loggin system)
- Metadata viewing
- Forensic tools (experimental)
- Disk space visualization (experimental)
- Log viewer
- Clone a drive
- Config file Editor, with my own tiny light weight text editor [Lume](https://github.com/Dogwalker-kryt/Lume)
- Fingerprinting
- Benchmarking
- Color theme choosable
- More in development or im too lazy to add them here

---

## Installation
_If during the installation somthing unexpected happens, you can ask for help by opening an issue or a discussion_
### Requirements

- Linux
- C++17 compiler (e.g., g++) or higher
- OpenSSL dev libraries
- build-essential, smartmontools

_The Requirements are only for when you manualy build the Application from scratch. If you use the build script, it will automaticly check and get the Required things_

### Clone the Repository

```sh
git clone https://github.com/Dogwalker-kryt/Drive-Manager-Linux
cd Drive-Manager-Linux
```

### Build Options

#### Option 1: Automated Build (if you have python3 installed)

```sh
python3 setup.py 	
```

_Defaults: creates necessary folders, builds the binary, and prompts for installation of missing packages._

##### Only if you want to have a command shortcut
After this you can run the command_creation.py
This will create in `/usr/local/bin` a script that will can be called in the terminal to start the Program

```
sudo python3 command_creation.py
```
#### Option 2: Manual Compile

```sh

g++ DriveMgr_CLI/src/DriveMgr_experi.cpp -Iinclude -o DriveMgr -lssl -lcrypto
```
For the GUI version:
```sh
cd DriveMgr_GUI
cargo build
```

If required, create these directories by hand:

```sh
mkdir -p ~/.local/share/DriveMgr/bin
mkdir -p ~/.local/share/DriveMgr/bin/bin
mkdir -p ~/.local/share/DriveMgr/bin/launcher
mkdir -p ~/.local/share/DriveMgr/bin/other_src
mkdir -p ~/.local/share/DriveMgr/data
touch ~/.local/share/DriveMgr/data/log.dat
touch ~/.local/share/DriveMgr/data/keys.bin
touch ~/.local/share/DriveMgr/data/config.conf
```

---

## Usage

Start by running the program (root required for some features):

```sh
sudo ./DriveMgr         # or through the launcher
or
sudo ./DriveMgr --operation-name  		# for quick acsess to operations
or
dmgr	# through command form the commandcreation
```

When started, you'll see a menu, for example:

```
 ┌─────────────────────────────────────┐
 │      DRIVE MANAGEMENT UTILITY       │
 ├─────────────────────────────────────┤
 │ 1. List Drives                      │
 │ 2. Format Drive                     │
 │ 3. Encrypt/Decrypt Drive            │
 │ ...                                 │
 │ 0. Exit                             │
 └─────────────────────────────────────┘
```

Navigate by using the Arrow Key's (in main menu only) to the desired action. The tool will prompt you for additional information (drive number, confirm, etc).  
For dangerous actions, an extra key (e.g., generated security key) is required as a safety confirmation.

---

## Example Workflow

1. **Start the tool:**
   ```sh
   sudo ./DriveMgr_experi
   ```
2. **Select an operation:**
   - E.g., View drive metadata: select with arrow and enter keys
3. **Choose a drive with arrow and enter keys**
4. **Follow prompts to complete your selected operation**

---

## Quick Guide to Folders and Files

- `DriveMgr_CLI/src/` and `include/` — the core CLI codebase.
- `DriveMgr_GUI/` — incomplete experimental GUIs (Rust, C++).
- `build_src.sh` — comprehensive setup/build script.

---

## Roadmap
_What will be in future_
- [ ] Experimental GUI/TUI modes
- [ ] Forensics and better diskspace visualization
- [x] Polishing dry run mode, CLI flags, refactoring

---

## License

Distributed under the [GPL-3.0 License](./LICENSE).

---

## Support, Ideas, and Contribution

- Found a bug or have an idea? [Open an issue](https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux/issues)
- Want to contribute? See [CONTRIBUTING.md](./CONTRIBUTING.md)
- Like the tool? Star the repo!

---

## Architecture

This Project has a lot code that has many relation to many things inside the Program. 

The Architecture folder will contain files with the Functionality and explantion and more information to the Code.

Its intended to help you (if you want) to edit the Program to your expectations easier and with less bugs.

---

## Known Issues

No current known Bugs/Issues

**Disclaimer:** This tool is in active development. Features may not always be stable. Use at your own risk—always test on non-critical systems!

---

## Do you like the Application?
```text
							    kc'.      .,cx
                            0c.             ....;dK
                         X'                ..     'kW
                        Wc   ..        ..''..       oW
                        WW; :k:lx0,   ;XOc;ldXd      .WW
                        WWc ,k,.,OxlooxXx'..;Kx      .WW
						WWx .cxxkOOO0KKKK0Okxkl       xW
						WWX  ;doodxxxxxxddxkkkx'  .,,. ;KW
					  WWKo. ;KXKOkxxxkkOKXNWWMMM0'       'xNW
                   WXx;. .xWMMMMWWNNWWWMMMMMMMMMMMk         'o0W
                 Wd'    'NWWMMMMMWWWMMMMMWWWWNNNXXX0,           :0W
               WX.    .dNWMMMMMMMMMMMMMMMMMMMMMMWWNNNNd.          '0W
             WWx.   .kMMMMMMMMMMWMMMMMMMMMMMMMMMMMMMMMMW:     .     oW
			Wx. .  ,WMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMMMMW.    .      lW
          Wx.   .  WMMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMMMMM.    .       N
          Wo';:;,..ONMMMMMMMMMMWWMMMMMMMMMMMMMMMMMMMMWNNX.        .. :W
   XXXXXK0kxkOOOOOo,..;oONMMMMMMWMMMMMMMMMMMMMMMMMWWXokOO:        .;dO0
  xxkOOOOOOOOOOOOOOOkl'   .,oXMMMMMMMMMMMMMMMMMMMWXK0oxOOkoc:::cokOOOOk0
  OxkOOOOOOOOOOOOOOOOOOx:....cMMMMMMMMMMMMMMMMMMMKo,,oxOOOOOOOOOOOOOOOOOOOO0K
 XxxkOOOOOOOOOOOOOOOOOOOOkxk00XWMMMMMMMMMMWXOdc'    ;oxOOOOOOOOOOOOOkkkxxddxk
'xddddxxxkkkkkkOOOOOOOOOOkxoc.   ........          .:lxkkOOOkkkxdoloxk0K
            Okkxxdooddddolc:ck0KXXNNNNNNXXKK0OOkdoll;,;:ccccc:cok
```
