<h1 align="center">
<br>Taskmaster</h1>
<h3 align=center> Beyond the code </h3>

<table align="center">
  <tr>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/u/69441843?v=4" width="150"><br>
      <a href=https://github.com/Droied4>Droied</a>
    </td>
    <td align="center">
      <img src="https://avatars.githubusercontent.com/u/76682946?v=4" width="150"><br>
      <a href=https://github.com/TuTaRdrgZ>Tuta</a>
    </td>
  </tr>
</table>
<div align="center">



<p align="center">
<img src="https://img.shields.io/badge/42-Barcelona-000000?style=flat-square&logo=42&logoColor=white" />
</p>
<img src="https://img.shields.io/github/languages/code-size/Droied4/Taskmaster?style=flat-square" />
<img src="https://img.shields.io/github/languages/top/Droied4/Taskmaster?style=flat-square" />

</div>

---

## 📖 Table of Contents
- [📍 Overview](#-overview)
- [📦 Features](#-features)
- [📂 Repository Structure](#-repository-structure)
- [🚀 Getting Started](#-getting-started)
  - [🔧 Installation](#-installation)
  - [🤖 Usage](#-usage)
- [📄 License](#-license)

---

## 📍 Overview

Taskmaster is a lightweight daemon designed to supervise and control processes in a Unix-like environment.
It provides a client-server architecture to manage program execution, monitor state, and ensure reliability through automatic restarts and logging.

---

## 📦 Features

- Client–server architecture  
- Daemon-based process supervision  
- Command Line Interface (CLI) control tool  
- Advanced logging (stdout / stderr management)  
- Configuration hot reload  
- Process group management

---

## 📂 Repository Structure

```sh
taskmaster/
├── Makefile
├── inc/
    ├── server/
    ├── client/
├── src/
    ├── server/
    ├── client/
├── config.lua
├── LICENSE
└── README.me
```

## 🚀 Getting Started

### 🔧 Installation

1. Clone the repository:
```sh
git clone --recursive https://github.com/Droied4/Taskmaster Taskmaster
```

2. Change to the project directory:
```sh
cd Taskmaster
```

3. Compile the dependencies:
```sh
make
```

### 🤖 Usage
```sh
#Daemon
./taskmasterd -h
--help                  -h -- print this ussage message and exit
--configuration         -c -- configuration file path
--logfile               -l -- logfile  path
--loglevel              -e -- log level
--nodaemon              -n -- run in the foreground
--version               -v -- print taskmasterd version number and exit


#Client (⚠️ Daemon must be running)
./taskmasterctl
╔══════════════════════╗
║   Taskmaster CLI     ║
╚══════════════════════╝
Use TAB for autocomplete. 'help' for commands, 'exit' to quit.

taskmaster> help
default commands (type help <topic>):
=====================================
start stop restart status reload pid shutdown help fg
taskmaster>
```

## 📄 License

This project is protected under the [UNLICENSE](https://choosealicense.com/licenses/unlicense) License. For more details, refer to the [LICENSE](LICENSE) file.

---
