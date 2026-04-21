# PES-VCS: Lightweight Version Control System in C

## Table of Contents

1. Introduction
2. Problem Statement
3. Objectives
4. Key Features
5. System Architecture
6. Internal Working
7. Project Structure
8. Installation & Build
9. Command Usage
10. Example Workflow
11. Design Decisions
12. Challenges Faced
13. Testing Strategy
14. Learning Outcomes
15. Future Enhancements
16. Conclusion
17. Contributors

---

## 1. Introduction

Version control systems are essential tools in modern software development. They help developers track file changes, collaborate efficiently, restore previous versions, and maintain project history. Popular systems such as Git are widely used in the software industry.

PES-VCS is a simplified version control system developed in the C programming language to understand the internal working principles of real-world version control tools. Instead of relying on existing libraries, this project focuses on implementing core concepts manually using file systems, hashing, indexing, metadata handling, and command-line programming.

The project demonstrates how low-level programming concepts can be used to build practical developer tools.

---

## 2. Problem Statement

Students often use Git and GitHub without understanding what happens internally when commands such as `git init`, `git add`, or `git commit` are executed. This project aims to bridge that gap by building a lightweight version control system from scratch.

---

## 3. Objectives

* Understand how version control systems manage data.
* Implement repository initialization.
* Build a staging area for tracking files.
* Store files as hashed objects.
* Create snapshots of project directories.
* Maintain commit history.
* Practice modular programming in C.
* Strengthen Linux systems programming knowledge.

---

## 4. Key Features

### Repository Management

* Initialize a hidden repository directory.
* Create internal folders for objects, commits, logs, and metadata.

### File Tracking

* Add one or more files to the staging area.
* Detect modified files.
* Prevent duplicate storage of unchanged content.

### Content Addressable Storage

* Store file data using content hashes.
* Reuse identical objects for efficiency.

### Snapshot Management

* Generate tree objects representing folder structure.
* Preserve hierarchy information.

### Commit System

* Save snapshots with commit messages.
* Store timestamps and author information.
* Link commits to parent commits.

### History Commands

* View previous commits.
* Inspect repository evolution over time.

### Command Line Utility

* Easy-to-use terminal interface inspired by Git.

---

## 5. System Architecture

```text
User Command
   ↓
CLI Parser
   ↓
Command Dispatcher
   ↓
Modules
 ├── Init Module
 ├── Index Module
 ├── Object Store
 ├── Tree Builder
 ├── Commit Engine
 └── Log Viewer
   ↓
Filesystem Storage
```

Each command is parsed by the CLI layer and forwarded to the relevant module.

---

## 6. Internal Working

## 6.1 Repository Initialization

When the user runs:

```bash
./pes init
```

The system creates hidden internal directories such as:

```text
.pes/
 ├── objects/
 ├── index/
 ├── commits/
 ├── refs/
 └── logs/
```

This acts as the internal database of the VCS.

### 6.2 Add Command

When a file is added:

```bash
./pes add notes.txt
```

Steps performed:

1. Read file contents.
2. Compute content hash.
3. Store file object if not already present.
4. Update staging index.

### 6.3 Commit Command

When committing:

```bash
./pes commit "Initial Commit"
```

The system:

1. Reads staged entries.
2. Builds tree snapshot.
3. Creates commit object.
4. Stores metadata.
5. Updates HEAD pointer.

### 6.4 Log Command

Displays commit history in reverse chronological order.

---

## 7. Project Structure

```text
PES-VCS/
├── pes.c
├── init.c
├── object.c
├── index.c
├── tree.c
├── commit.c
├── log.c
├── utils.c
├── headers/
├── tests/
└── Makefile
```

### Module Description

* `pes.c` → Entry point and command parser.
* `init.c` → Repository setup.
* `object.c` → File hashing and object storage.
* `index.c` → Staging area logic.
* `tree.c` → Snapshot generation.
* `commit.c` → Commit metadata handling.
* `log.c` → Commit history display.
* `utils.c` → Shared helper functions.

---

## 8. Installation & Build

### Requirements

* Linux / Ubuntu environment
* GCC compiler
* Make utility

### Build Steps

```bash
git clone <repository-url>
cd PES-VCS
make
```

### Output

```bash
./pes
```

---

## 9. Command Usage

### Initialize Repository

```bash
./pes init
```

### Add File

```bash
./pes add file.txt
```

### Add Multiple Files

```bash
./pes add file1.txt file2.txt
```

### Commit Changes

```bash
./pes commit "Updated project"
```

### View History

```bash
./pes log
```

---

## 10. Example Workflow

```bash
./pes init
./pes add main.c
./pes commit "Added main program"
./pes add utils.c
./pes commit "Added utilities"
./pes log
```

Expected result:

* Two commits stored.
* Full history available.
* Files versioned safely.

---

## 11. Design Decisions

* Implemented in C for maximum control over memory and files.
* Modular structure for maintainability.
* CLI syntax similar to Git for familiarity.
* Filesystem-backed storage for simplicity and transparency.

---

## 12. Challenges Faced

* Handling binary-safe file reading.
* Preventing duplicate object creation.
* Maintaining index consistency.
* Parsing command-line arguments safely.
* Managing memory leaks in C.
* Updating commit pointers correctly.

---

## 13. Testing Strategy

The project was tested using:

* Small text files
* Repeated commits
* Modified file detection
* Empty repository cases
* Invalid command handling
* Multi-file staging scenarios

Sample test commands:

```bash
./pes init
./pes add demo.txt
./pes commit "test"
./pes log
```

---

## 14. Learning Outcomes

This project helped strengthen knowledge in:

* Data structures
* Hashing algorithms
* File systems
* Metadata storage
* Linux command-line tools
* Dynamic memory allocation
* Modular software engineering
* Real-world Git internals

---

## 15. Future Enhancements

* Branch creation and switching
* Merge support
* File difference viewer
* Restore previous versions
* Compression of stored objects
* Remote repository support
* Colored terminal UI
* Better error diagnostics
* Ignore file patterns (`.pesignore`)

---

## 16. Conclusion

PES-VCS successfully demonstrates the core principles behind distributed version control systems using low-level C programming. By implementing repository management, object storage, staging, commits, and logs, the project transforms theoretical concepts into practical understanding.

It serves as both an academic project and a strong systems-programming learning experience.

---

## 17. Contributors

Developed collaboratively as part of B.Tech Computer Science coursework.

---

## Thank You

If you found this project interesting, feel free to explore, improve, and extend it further.
