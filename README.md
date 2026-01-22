# Unix-like Shell Implementation in C

## Overview
This project is a **Unix-like command-line shell** implemented from scratch in **C** using **POSIX system calls**.  
It demonstrates core operating system concepts including **process creation**, **inter-process communication**, **I/O redirection**, and **manual memory management**.

The shell follows a **layered architecture** consisting of a lexer, parser, and executor, similar to real-world shells.

---

## Features

- Custom **character-by-character lexer**
- Tokenization of:
  - Commands and arguments
  - Pipes (`|`)
  - Input redirection (`<`)
  - Output redirection (`>`)
  - Append redirection (`>>`)
- Command parsing into executable structures
- Support for **command pipelines**
- Built-in commands:
  - `cd`
  - `pwd`
  - `echo`
  - `type`
  - `exit`
- Process management using:
  - `fork`
  - `execvp`
  - `wait`
- Manual memory management suitable for long-running REPL execution

---

## Architecture

