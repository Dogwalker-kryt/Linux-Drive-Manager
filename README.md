# Linux-Drive-Manager

A terminal-based drive management tool for Linux (Debian-based), written primarily in C++. 

This application provides a straightforward interface for managing storage devices — HDDs, SSDs, and more — directly from the command line.

## Why This Utility Exists

Performing Disk operations on Linux often requires chaining multiple commands together, like lsblk, dd and more.

A small mistake can lead to:
- Data loss
- System instability
- Irreversible changes

This tool exists to make those operations safer and reproducible.

It provides:

- Dry-run mode before execution
- Explicit command preview in the source code
- Input validation
- Safety confirmations
- Structured logging
- TUI for controlled interaction

The goal is not to replace underlying tools,
but to add a safety and orchestration layer on top of them.

---

## Version Control

#### Activly in Development

### CLI:

- Experimental: `v0.9.24.46`  
  _(new -cfg-src and -sd commands)_

### GUI (Not getting Updated anymore):

- Rust GUI: `v0.1.5-alpha` (Lists drives; not feature complete)
- C++ GUI: `v0.1.1-alpha`  
  _(GUI versions lag behind CLI/TUI)_

---

## Warning

**This tool is for advanced users only!** Formatting, encrypting, or modifying drives is inherently risky.  
- Always back up your data.
- Do not type arbitrary or dangerous input at prompts (such as `/dev/sda rm -rf`).  
- Double-check all device paths before confirming any destructive operation.

---

## Project Structure

```
/DriveMgr_CLI      - CLI source code (C++)
    /include       - Headers (.h)
    /src           - Core sources

/DriveMgr_GUI      - GUI version (Rust/C++)

/Launcher
	launcher.cpp   - Launcher source code (C++)
	launcher       - Launcher binary

/Lume
	Lume (elf)     - Text editor for Configuration editing in ldisk
	main_lume.cpp  - Lume source code (C++)

/Log and Key file
	log.dat		   - Example log file

setuo.py	       - Build/install script
config.conf        - Example config

```

---

## Features

#### More in development 

### Feature Overview

| Feature        					| Status     	| Description                          					|
|-----------------------------------|---------------|------------------------------------------------------|
| List Drives and Partitions	    | Working    	| Shows all connected storage devices  					|
| SMART disk health check		    | Working 	 	| Reads health data from drives        					|
| Format Disk (lable, fstype)    	| Wokring  	 	| Helps Formatting drives 	            				|
| Partition Management		 		| Wokring  	 	| Helps Manaing Partitions             					|
| USB Drive En- an Decryption  		| Working	 	| Encrypts USB Drives 									|
| Drive Overwriting					| Working	 	| Wipes drive to 0										|
| Logging and config system			| Working		| Change history / For start preferences and colors		|
| viewing Metadata					| Working	 	| View Metadata of Drives; Works with raw and SMART		|
| Forensic/Recovery Tools			| Experimental	| For file recovery and stuff							|
| Low viewer						| Working		| View last Loggs and wipe Log file clean if you want	|
| Clone Drives						| Wokring		| Clone content of a Drive from one to another			|
| Config viewer and editor with [Lume](https://github.com/Dogwalker-kryt/Lume)	| Working		| View Config setting and change them i prefered		|
| Fingerprinting					| Working		| Create a sha256 hash Fingerprint of you drive			|
| Color themes						| Wokring		| Choose color of Menu boxes and selection color		|
| Intigrated Tests					| Working       | Test side and helper functions right in the program   |


### Start Commands

Every command has a normal name and a shortend abstract version

| Command					| Description 									|
|---------------------------|-----------------------------------------------|
| --version, -v				| Prints the current version of the Dmgr 		|
| --help, -h				| Prints the printusage for all CLI commands	|
| --dry-run, -n				| Disables the execution of commads through the exec_cmd.h api in the current session |
| --no-color, -c			| Disables the use of colors in the current session |
| --no-log, -nl				| Disables loggin in the current session |
| --debug, -d				| Enables that debug_msg() functions are printed and enables the usage of the Test option in the current session |
| --info, -i				| Show Dmgr info |
| --logs, -l 				| Show log file content |
| --select [device], -sd [device] | Pre selects the drive you entered, skiping the drive selection in the current session |
| --config-src [path], -cfg-src [path] | With this you can load the program with another config file in the current session |
| --operation-name 			| With this you can directly jump to functions/operation wihtout using the menu |

---


## Installation
_If during the installation somthing unexpected happens, you can ask for help by opening an issue or a discussion_
### Tech Stack / Requirements

- Linux (debian based recommended, but it should also run on other distros)
- C++17 compiler (e.g., g++) or higher
- OpenSSL dev libraries
- build-essential, smartmontools
- Python3 with os libary 

_The Requirements are only for when you manualy build the Application from scratch. If you use the build script, it will automaticly check and get the Required things_


### Clone the Repository

```sh
git clone https://github.com/Dogwalker-kryt/Drive-Manager-Linux
cd Drive-Manager-Linux
```

### Step 1: Setup the Application envroment

There are 2 ways to do that:


#### Option 1: Python setup script

##### Step 1.1: Check

If you choose to use the setup script you have to do/check the following:

- Choose a path where the Dmgr should be installed in the ``` .env ``` file
- You have one of these package managers:
	- apt
   	- zypper
   	- dnf
   	- yum
   	- pacman
- You have ``` python3 ``` installed


##### Step 1.2: Run python script

If everything is checked, then run this command:

_(You may need to run it with ``` sudo ``` if the needed packages aren't installed)_
```sh
python3 setup.py
```


##### Step 1.3: Build

If everything went successfull, 
then you will be prompted if you want the script to compile the Dmgr.

If you choose ``` y ``` then you finished the installtion and the Dmgr is ready to use


#### Option 2: Manual Installtion

##### Step 2.1: Application Enviroment setup

Create in a dir you like
```sh
mkdir -p ~/.local/share/DriveMgr/bin
mkdir -p ~/.local/share/DriveMgr/bin/bin
mkdir -p ~/.local/share/DriveMgr/bin/launcher
mkdir -p ~/.local/share/DriveMgr/bin/other_src
```


**Create data and move files:**
Create the ``` data ``` dir in DriveMgr and move reqired file into it

```sh
mkdir -p ~/.local/share/DriveMgr/data
touch ~/.local/share/DriveMgr/data/log.dat
mv [your location where the DriveMgr clone is right now]/Linux-Drive-Manager/config.conf ~/.local/share/DriveMgr/data
```

### Step 2: Build the Binary

You have 2 Options, the first is to run the automated script, the second is manualy build the binary you want


#### Option 1: Automated Build (if you have python3 installed)

- Choose your desired path where the Dmgr should live in, in the ``` .env ``` file!!!

```sh
python3 setup.py 	
```

_Defaults: creates necessary folders, builds the binary (in the dir you choose in .env), and prompts for installation of missing packages._


##### Only if you want to have a command shortcut (Dont use, its under development)
After this you can run the command_creation.py
This will create in `/usr/local/bin` a script that will can be called in the terminal to start the Program

```
sudo python3 command_creation.py
```

#### Option 2: Manual Compilation

##### for C++ CLI

``` cd ``` into the ```DriveMgr_CLI``` folder, them run:
```sh
make
```
##### For the GUI version:

``` cd ``` into the ```DriveMgr_GUI``` folder, them run or the Rust version (if you have rust compiler):
```sh
cargo build
```
###### for C++ GUI:

``` cd ``` into the ```DriveMgr_GUI/C++-GTK-GUI``` folder, them run or the Rust version (if you have gtk):
```sh
make
```

---

## Usage

The recommended terminal hight and width are:
- hight : 40
- width : 120

when not using the min recommended hight and width, this can lead to corrupted text prints

Start by running the program (root required for some features):

**starting raw binary**
```sh
sudo ./DriveMgr        
```

**with flags**
```sh
sudo ./DriveMgr --operation-name  		
```

**through Launcher**
```sh
sudo dmgr	
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

## License

Distributed under the [GPL-3.0 License](./LICENSE).

---

## Support, Ideas, and Contribution

- Found a bug or have an idea? [Open an issue](https://github.com/Dogwalker-kryt/Drive-Manager-for-Linux/issues)
- Want to contribute? See [CONTRIBUTING.md](./CONTRIBUTING.md)
- Like the tool? Star the repo!


---

## Known Issues

- Info block sometimes not printing right
- Drive Fingerprinting sometimes returning to main menu before it could do anything

**You found an issue?**
If you found an issue that is presistent after restarting the Application, please report it by opening an issue with a detailed report

**Disclaimer:** This tool is in active development. Features may not always be stable. Use at your own risk—always test on non-critical systems!

---

## Do you like the Application?

**if you like the Drive Manager Utility, please leave a star**
