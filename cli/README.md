# 🛡️ HyperDecode v3.1.0 (Beta 3)
### *The Ultimate Multi-Layer Decoding Suite for Professionals*

**HyperDecode** is a high-performance, intelligent de-obfuscation engine designed to peel back layers of encoding with surgical precision. Whether you are a security researcher, a CTF player, or a developer dealing with complex data formats, HyperDecode automates the "guesswork" and lets you focus on the data itself.

---

## 🔥 Why HyperDecode?

Ever spent hours manually running Base64 -> Hex -> XOR -> URL decode only to find out you missed a step? **HyperDecode stops the madness.**

- **🧠 "AI-Like" Heuristic Search**: It doesn't just decode; it *understands*. Our engine analyzes data patterns to predict the next logical decoding step.
- **⚡ Performance First**: Written in pure C and optimized for high-speed processing, it runs smoothly even on vintage hardware (i5-7200U tested).
- **🔋 Feather-Light**: Consumes <32MB of RAM. Keep it open in the background without even noticing it's there.
- **🛠️ Unified Experience**: Use the **CLI** for rapid scripting/piping or the **Qt GUI** for a deep-dive visual workspace.

---

## ✨ Key Features

### 1. Smart Pipeline Search
Drop your obfuscated string, and let the engine hunt for the original payload. It explores hundreds of transformation paths simultaneously, discarding dead ends and prioritizing high-confidence results.

### 2. The Recipe System
Create and save custom "Recipes"—predefined chains of transformations (e.g., *Base64 -> Reverse -> Hex*). Apply them to thousands of files instantly using the **Batch Processor**.

### 3. Professional CLI Tools
A fully colorized terminal interface with support for:
- **Interactive Trace**: See exactly how the engine derived the result.
- **JSON Export**: Integrated seamlessly into your existing automation pipelines.
- **Piping**: `cat file.bin | hyperdecode --pipeline`

### 4. Modern Compact GUI
A sleek, "Hacker-style" dark interface designed to provide a distraction-free environment. All tools (Decode, Encode, Pipeline, History, Recipes) are just one click away.

---

## 📦 Getting Started

### Installation (CLI)
HyperDecode is portable. To install it globally:
1. **Download** the `HyperDecode-CLI.rar` from the [Releases](#).
2. **Extract** it into your project root.
3. **Run the Installer**:
   ```powershell
   .\install_cli.ps1
   ```
4. Restart your terminal, and you're ready!

### Usage Examples
- **Automatic Search**: `hyperdecode "SGVsbG8=" --pipeline`
- **Manual Trace**: `hyperdecode input.txt --trace`
- **JSON Output**: `hyperdecode input.txt --json`

---

## 🤝 Roadmap
- [ ] Plugin System (Lua/Python integration)
- [ ] Advanced Entropy Analysis
- [ ] Network Packet Decoding

**Developed with ❤️ by the HyperDecode Team.**
*Peel the layers, find the truth.*
