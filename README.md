# <p align="center">🛡️ HyperDecode</p>
<p align="center">
  <img src="https://img.shields.io/badge/Intelligence-Probabilistic_Search-brightgreen" alt="Intelligence">
  <img src="https://img.shields.io/badge/Language-Pure_C-blue" alt="Language">
  <img src="https://img.shields.io/badge/RAM_Usage-<32MB-orange" alt="RAM">
  <img src="https://img.shields.io/badge/Status-Beta_3.1-yellow" alt="Status">
</p>

<p align="center">
  <strong>High-Confidence Heuristic Engine for Multi-Layer De-obfuscation.</strong><br>
  <em>Treating decoding as a probabilistic search problem with near real-time exploration.</em>
</p>

---

## 🔍 Overview

**HyperDecode** treats decoding as a dynamic search problem rather than a static sequence of operations. 

Unlike traditional decoders that apply fixed transformations, HyperDecode explores a weighted tree of possible decoding paths using a **Heuristic Beam Search** strategy—simulating a lightweight inference process. The search process forms a **Directed Acyclic Graph (DAG)** where each path represents a potential decoding chain, allowing the engine to recover the most probable payload even when the exact transformation sequence is unknown.

---

## 🧬 How It Works (Core Concepts)

HyperDecode models de-obfuscation as a traversal through a **State Space**:

- **Nodes**: Each intermediate output is treated as a state in the transformation graph.
- **Transition Function**: Decoders act as edges transforming one state into another (e.g., $f(Base64)$).
- **Heuristic Function**: The Scoring Engine acts as a **proxy for semantic understanding**, approximating how "meaningful" a decoded output is without requiring full context awareness.
- **Beam Width Control**: Limits exploration to the Top-K candidates at each depth level to prevent recursive combinatorial explosion.

### Signal Exploitation
Most obfuscated data retains **"Weak Signals"** that HyperDecode's scoring engine identifies:
- **Entropy Analysis**: Measures Shannon entropy; lower entropy in ASCII/JSON indicates high-confidence decoding.
- **Magic Number Detection**: Instant recognition of file signatures (e.g., `50 4B`, `7B 22`, `4D 5A`).
- **Frequency Analysis**: Exploits character distribution patterns to identify valid linguistic or structural signals.

---

## ✨ Key Features

- 🧠 **Search-Based Engine**: Dynamically explores a transformation graph to find the most probable decoding path.
- ⚡ **High-Speed C Core**: Native performance optimized for massive recursive tasks.
- 🔋 **Efficient Footprint**: Maintains a **<32MB RAM** footprint—ideal for embedded-friendly use.
- 📋 **Recipe System**: Design, save, and batch-apply custom transformation chains.
- ⌨️ **Hacker CLI**: Full ANSI color support with interactive trace and JSON metadata export.

---

## 📊 Performance Benchmark
*Tested on: Intel i5-7200U / 16GB RAM (Single-threaded mode)*

| Input Complexity | Obfuscation Layers | Time (Avg) | Confidence |
| :--- | :---: | :---: | :---: |
| Base64 → Hex → XOR | 3 | **12ms** | ✅ High |
| Double Base64 + Rot13 | 3 | **8ms** | ✅ High |
| Unknown Mixed Encoding | 5 | **35ms** | ✅ Med |

---

## 🚀 Quick Start

### Intelligent Pipeline Search
```bash
hyperdecode "SGVsbG8=" --pipeline
```

### Path Trace (Logic Inspection)
```bash
hyperdecode input.txt --trace
```

---

## ⚠️ Limitations

- **Path Depth**: Extremely nested transformations (>8 layers) significantly increase search complexity.
- **Binary Noise**: Heuristic scoring may mis-prioritize highly randomized binary data.
- **Probabilistic Nature**: As a heuristic engine, it target high-probability paths but does not guarantee a solution for 100% of custom encodings.

---

## 🛤️ Roadmap
- [ ] **Adaptive Beam Width**: Dynamically adjust search breadth based on entropy and confidence metrics.
- [ ] **Learned Scoring Model**: Integrate lightweight ML-based scoring for improved path accuracy.
- [ ] **Scripting Plugin**: Lua & Python support for custom transition functions.

---

**Developed with ❤️ by HyperDecode Team.**  
[Repository](https://github.com/tamvt-dev/HyperDecode) • [Report Issue](https://github.com/tamvt-dev/HyperDecode/issues)
