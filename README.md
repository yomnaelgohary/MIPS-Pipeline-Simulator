
# MIPS Pipeline Simulator

## 📌 Project Overview

This project is a **C-based simulator** for a simplified **5-stage MIPS pipeline architecture**, developed as part of the **CSEN601 - Computer Systems Architecture** course at the German University in Cairo (Spring 2025). It implements **Package 1: Spicy Von Neumann Fillet with Extra Shifts**.

The simulator models the complete MIPS instruction pipeline with hazard flushing and branch handling, based on a Von Neumann architecture featuring **shared instruction and data memory**.

---

## 🔧 Features

- Simulates a 5-stage pipelined MIPS processor:  
  **Instruction Fetch (IF) → Instruction Decode (ID) → Execute (EX) → Memory Access (MEM) → Write Back (WB)**
- Reads and decodes binary instruction files (`instructions.bin`)
- Models instruction-level parallelism with up to 4 instructions simultaneously in the pipeline
- Handles data hazards and pipeline control hazards via flushing
- Supports essential MIPS instructions including:  
  - Jumps (`J`) and conditional branches (`BNE`)  
  - ALU operations and shift operations  
  - Memory read/write
- Word-addressable unified memory
- Prints register and memory states after each clock cycle

---

## 📁 Architecture Details

- **Memory**: Unified 2048 x 32-bit memory  
  - Addresses 0–1023: Instruction memory  
  - Addresses 1024–2047: Data memory  
- **Registers**:  
  - 32 general-purpose 32-bit registers (`R0`–`R31`), with `R0` hardwired to zero  
  - 32-bit Program Counter (`PC`)  
- **Instruction Set**: 12 supported MIPS-like instructions  
- **Pipeline Constraints**:  
  - Shared memory implies **IF** and **MEM** stages cannot run simultaneously  
  - Clock cycle formula for `n` instructions: `7 + (n - 1) * 2`

---

## ▶️ How to Run

### Prerequisites
- GCC or any standard C compiler
- Visual Studio Code (recommended)
- Ubuntu 


## 🖨️ Output Format

The simulator outputs the following at each clock cycle:

- Clock cycle number
- Active instructions per pipeline stage
- Input/output per stage
- Changes in registers or memory
- Final state of all registers and memory

---

## 🧠 Project Scope

This implementation focuses on **Package 1** as specified in the CSEN601 project description. Additional features such as data forwarding or full hazard detection were not implemented, following package requirements.

---

## 📜 License

This project is for **educational use only**. All rights reserved to the contributors and the CSEN601 teaching staff.

---

## 📦 Deliverables

This repository includes the full implementation of the MIPS Pipeline Simulator, along with:

- `Project_Team_60_ReportandVideo.zip`:  
  Contains the final project report and a 1-minute demonstration video showcasing the simulator in action.

The ZIP file is located in the root of the repository.
