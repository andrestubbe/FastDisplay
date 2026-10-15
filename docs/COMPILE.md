# Building from Source

## Prerequisites

- JDK 17+
- Maven 3.9+
- **Windows:** Visual Studio 2019+ or Build Tools

## Build

### Windows

```bash
compile.bat
mvn clean package
```

The build script compiles the native library and packages it with the JAR.

## Run Examples

```bash
run-demo.bat
run-demo-theme-listener.bat
run-benchmark.bat
```

## Installation

### JitPack (Recommended)

```xml
<repositories>
    <repository>
        <id>jitpack.io</id>
        <url>https://jitpack.io</url>
    </repository>
</repositories>

<dependencies>
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastDisplay</artifactId>
        <version>0.1.2</version>
    </dependency>
    <dependency>
        <groupId>com.github.andrestubbe</groupId>
        <artifactId>FastCore</artifactId>
        <version>0.1.0</version>
    </dependency>
</dependencies>
```

### Gradle (JitPack)

```groovy
repositories {
    maven { url 'https://jitpack.io' }
}

dependencies {
    implementation 'com.github.andrestubbe:FastDisplay:0.1.2'
    implementation 'com.github.andrestubbe:FastCore:0.1.0'
}
```
