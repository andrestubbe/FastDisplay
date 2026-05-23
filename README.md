# FastDisplay â€” Native Display Monitoring & DPI API for Java [v0.1.0]

**High-performance display telemetry for the FastJava ecosystem. Monitor resolution, DPI scaling, refresh rates, and orientation changes with zero latency.**

[![Status](https://img.shields.io/badge/status-v0.1.0--alpha-orange.svg)]()
[![Java](https://img.shields.io/badge/Java-17+-blue.svg)](https://www.java.com)
[![Platform](https://img.shields.io/badge/Platform-Windows%2010+-lightgrey.svg)]()
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

---

**FastDisplay** is the dedicated display monitoring module of the FastJava ecosystem. It provides real-time events for display changes, allowing your Java application to respond instantly to resolution shifts or DPI scaling updates.

## Table of Contents
- [Features](#features)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Build from Source](#build-from-source)
- [License](#license)

## Features
- **ðŸ“Š Real-Time Telemetry**: Monitor Resolution, DPI, and Refresh Rate.
- **ðŸ”„ Event Driven**: Native callbacks for `WM_DISPLAYCHANGE` and `WM_DPICHANGED`.
- **ðŸ–¥ï¸ Multi-Monitor Support**: Detect and track attributes across multiple displays.
- **âš¡ Zero Overhead**: Lightweight JNI layer with no polling required.

## Quick Start

```bash
# Clone the repository
git clone https://github.com/andrestubbe/FastDisplay.git

# Build the native bridge
cd FastDisplay
.\compile.bat

# Launch the DisplayDemo
.\run-demo.bat
```

## Installation

### Option 1: Maven (Recommended)
Add the JitPack repository and the dependencies to your `pom.xml`:

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependencies>
    <!-- FastDisplay Library -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>fastdisplay</artifactId>
        <version>v0.1.0</version>
    </dependency>

    <!-- FastCore (Required Native Loader) -->
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>fastcore</artifactId>
        <version>v0.1.0</version>
    </dependency>
</dependencies>
```

### Option 2: Gradle (via JitPack)
```groovy
repositories {
    maven { url 'https://jitpack.io' }
}

dependencies {
    implementation 'com.github.andrestubbe:fastdisplay:v0.1.0'
    implementation 'com.github.andrestubbe:fastcore:v0.1.0'
}
```

### Option 3: Direct Download (No Build Tool)
Download the latest JARs directly to add them to your classpath:

1. 📦 **[fastdisplay-v0.1.0.jar](https://github.com/andrestubbe/FastDisplay/releases/download/v0.1.0/fastdisplay-v0.1.0.jar)** (The Core Library)
2. ⚙️ **[fastcore-v0.1.0.jar](https://github.com/andrestubbe/FastCore/releases/download/v0.1.0/fastcore-v0.1.0.jar)** (The Mandatory Native Loader)

> [!IMPORTANT]
> All JARs must be in your classpath for the native JNI calls to function correctly.


## Build from Source
- **JDK 17+**
- **Windows 10/11**

See [COMPILE.md](COMPILE.md) for detailed build instructions.

## License
MIT License â€” See [LICENSE](LICENSE) for details.

---
**Part of the FastJava Ecosystem** â€” *Making the JVM faster.*


