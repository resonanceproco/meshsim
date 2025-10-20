# Part 1: meshsim - ESP32-S3 Firmware for SIM Mesh Nodes

This folder contains the firmware, configuration, and tests for ESP32-S3 based SIM Mesh Nodes. Each node manages 20 SIM cards and participates in an AES-256 encrypted mesh network.

## 📋 Overview

**Hardware Platform:** ESP32-S3 (Dual-core Xtensa LX7 @ 240MHz, 16MB Flash, 8MB PSRAM)  
**Framework:** Arduino + PlatformIO  
**Network:** painlessMesh (encrypted WiFi mesh)  
**Security:** AES-256 encryption, Secure boot ready  

### Key Capabilities

| Feature | Description | Status |
|---------|-------------|--------|
| **SIM Management** | 20× SIM slots with multiplexing | 🟡 Partial |
| **Mesh Networking** | AES-256 encrypted mesh | 🟡 Partial |
| **Persian SMS** | UCS2 encoding for Farsi | 🟡 Partial |
| **Heartbeat** | 10s interval health monitoring | 🟡 Partial |
| **OTA Updates** | Secure over-the-air firmware updates | 🔴 Minimal |
| **Auto Detection** | SIM card insertion/removal | 🟡 Partial |
| **Health Monitoring** | System and SIM health tracking | 🟡 Partial |

**Legend:** 🟢 Complete | 🟡 Partial | 🔴 Minimal/Missing

---

## 🏗️ Hardware Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         ESP32-S3 Node                               │
│                                                                     │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │   Core System    │  │  SIM Management  │  │  Connectivity   │  │
│  │                  │  │                  │  │                 │  │
│  │ • ESP32-S3       │  │ • 20× SIM Slots  │  │ • WiFi 802.11n  │  │
│  │ • 16MB Flash     │  │ • 5× TS3A5017    │  │ • BLE 5.0       │  │
│  │ • 8MB PSRAM      │  │   Multiplexers   │  │ • Mesh Network  │  │
│  │ • Dual-core      │  │ • SIM Detection  │  │ • 2× Antennas   │  │
│  │   240MHz         │  │ • Hot-swap       │  │                 │  │
│  │ • Secure Boot    │  │   Protection     │  │                 │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
│                                                                     │
│  ┌──────────────────┐  ┌──────────────────┐  ┌─────────────────┐  │
│  │  GSM Module      │  │  Power System    │  │  Interfaces     │  │
│  │                  │  │                  │  │                 │  │
│  │ • SIM800C        │  │ • 5-24V Input    │  │ • UART Debug    │  │
│  │ • Quad-band GSM  │  │ • Battery Backup │  │ • GPIO LEDs     │  │
│  │ • GPRS Class 12  │  │ • INA219 Monitor │  │ • I2C/SPI       │  │
│  │ • AT Commands    │  │ • Low Power Mode │  │ • USB-C         │  │
│  └──────────────────┘  └──────────────────┘  └─────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

### SIM Multiplexing Architecture

```
ESP32-S3                    TS3A5017 Multiplexers              SIM Slots
┌──────┐                    ┌─────────────────┐
│ GPIO │────── Control ────►│  4:1 MUX #1     │─── SIM 1-4
│      │                    └─────────────────┘
│      │                    ┌─────────────────┐
│ SIM  │───── Data ────────►│  4:1 MUX #2     │─── SIM 5-8
│ UART │                    └─────────────────┘
│      │                    ┌─────────────────┐
│ +    │                    │  4:1 MUX #3     │─── SIM 9-12
│      │                    └─────────────────┘
│ DET  │                    ┌─────────────────┐
│ GPIO │                    │  4:1 MUX #4     │─── SIM 13-16
└──────┘                    └─────────────────┘
                            ┌─────────────────┐
                            │  4:1 MUX #5     │─── SIM 17-20
                            └─────────────────┘
```

---

## 📂 Folder Structure

```
meshsim/
├── README.md                          # This file
├── .env.example                       # Environment config template
├── platformio.ini                     # Build configuration
│
├── src/                               # Firmware source code
│   ├── main.cpp                       # Application entry point
│   │
│   ├── core/                          # Core system modules
│   │   ├── persistent_node_manager.cpp    # Unique node ID management
│   │   ├── configuration_manager.cpp      # Runtime configuration
│   │   └── env_config.cpp                 # Environment loader (NVS)
│   │
│   ├── mesh/                          # Mesh networking
│   │   └── mesh_network_manager.cpp   # painlessMesh wrapper
│   │
│   ├── sim/                           # SIM card management
│   │   ├── sim_multiplexer.cpp        # Hardware MUX control
│   │   ├── auto_sim_detector.cpp      # SIM presence detection
│   │   └── persian_sms_handler.cpp    # UTF-8 to UCS2 conversion
│   │
│   ├── gsm/                           # GSM communication
│   │   └── (AT command handlers)      # TBD: GSM module interface
│   │
│   ├── security/                      # Security modules
│   │   └── aes256_encryption.cpp      # AES-256 implementation
│   │
│   ├── system/                        # System management
│   │   ├── health_monitor.cpp         # Health checks
│   │   └── loki_logger.cpp            # Structured logging
│   │
│   ├── communication/                 # External communication
│   │   └── (MQTT, HTTP clients)       # TBD: Server integration
│   │
│   └── plugins/                       # Extensibility
│       └── (plugin system)            # TBD: Plugin architecture
│
├── include/                           # Header files
│   ├── env_config.h                   # Config loader interface
│   ├── aes256_encryption.h
│   ├── auto_sim_detector.h
│   ├── health_monitor.h
│   ├── loki_logger.h
│   ├── mesh_network_manager.h
│   ├── persian_sms_handler.h
│   ├── persistent_node_manager.h
│   ├── sim_multiplexer.h
│   └── README                         # Header documentation
│
├── lib/                               # External libraries
│   └── README                         # Library documentation
│
├── test/                              # Test suites
│   ├── unit/                          # Unit tests
│   │   └── test_aes256.cpp            # AES performance test
│   ├── integration/                   # Integration tests
│   └── hardware/                      # Hardware-in-loop tests
│
├── config/                            # Configuration headers
│   ├── mesh_config.h                  # Mesh network settings
│   ├── security_config.h              # Security parameters
│   └── sensor_config.h                # Sensor calibration
│
└── docs/                              # Documentation
    ├── hardware/                      # Hardware schematics
    ├── api/                           # API documentation
    └── deployment/                    # Deployment guides
```

---

## 🚀 Quick Start

### Prerequisites

1. **PlatformIO** - Install via VS Code extension or CLI:
   ```bash
   pip install platformio
   ```

2. **ESP32 USB Drivers** - Install appropriate driver for your OS:
   - Windows: CH340/CP2102 driver
   - macOS: Usually automatic
   - Linux: Add user to `dialout` group

3. **Hardware** - ESP32-S3 development board

### Build and Upload

```bash
# Navigate to firmware directory
cd meshsim

# Build firmware
pio run

# Upload to connected ESP32-S3
pio run -t upload

# Monitor serial output
pio device monitor

# Or combine upload + monitor
pio run -t upload && pio device monitor
```

### Expected Output

```
Connecting........_____.....___...
Writing at 0x00010000... (10 %)
...
Writing at 0x000e0000... (100 %)
Wrote 1234567 bytes at 0x00010000 in 12.3 seconds (effective 1234.5 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...

--- Terminal on /dev/ttyUSB0 | 115200 8-N-1
--- Quit: Ctrl+C | Menu: Ctrl+T | Help: Ctrl+T followed by Ctrl+H ---

[INFO] ESP32-S3 SIM Mesh Node Starting...
[INFO] Node ID: ABC123DEF456
[INFO] Loading configuration from NVS...
[INFO] Mesh network initializing...
[INFO] Mesh SSID: SIM_MESH_ABC123
[INFO] Mesh network started successfully
[INFO] Scanning for SIM cards...
[INFO] SIM slot 1: DETECTED (ICCID: 89xxxxxxxxxxxxxxxxxx)
[INFO] SIM slot 2: EMPTY
...
```

---

## ⚙️ Configuration

### Environment Variables (.env)

Configuration is stored in Non-Volatile Storage (NVS) and can be loaded from a `.env` file. Copy the example:

```bash
cp .env.example .env
# Edit .env with your settings
```

**Key Configuration Options:**

```ini
# Device Identification
DEVICE_ID=auto                    # Auto-generate from hardware
DEVICE_NAME=Node-01               # Human-readable name

# Mesh Network
MESH_SSID=SIM_MESH                # Mesh network SSID prefix
MESH_PASSWORD=CHANGE_ME           # Mesh network password (min 8 chars)
MESH_PORT=5555                    # Mesh communication port
MESH_MAX_HOPS=6                   # Maximum routing hops

# Security
AES_KEY=YOUR_32_BYTE_KEY_HERE     # ⚠️ CHANGE IN PRODUCTION!
HMAC_KEY=YOUR_HMAC_KEY_HERE       # ⚠️ CHANGE IN PRODUCTION!

# SIM Configuration
SIM_SLOTS=20                      # Number of SIM slots
SIM_DETECT_INTERVAL=5000          # SIM detection interval (ms)

# Heartbeat
HEARTBEAT_INTERVAL=10000          # Heartbeat interval (ms)
HEARTBEAT_TIMEOUT=30000           # Heartbeat timeout (ms)

# Logging
LOG_LEVEL=INFO                    # DEBUG, INFO, WARN, ERROR
LOKI_ENABLED=true                 # Enable Loki logging
LOKI_URL=http://server-ip:3100    # Loki server URL
```

### Runtime Configuration

Configuration can also be updated remotely via the server API:

```cpp
// Example: Update config via JSON
{
  "mesh_password": "new_password",
  "heartbeat_interval": 10000,
  "log_level": "DEBUG"
}
```

The node will save changes to NVS and apply them immediately (or after reboot for some settings).

---

## 🧪 Testing

### Unit Tests

```bash
# Run all unit tests
pio test

# Run specific test environment
pio test -e native

# Verbose output
pio test -v

# With filter
pio test -f test_aes256
```

**Example Test:** AES-256 Performance

```cpp
// test/unit/test_aes256.cpp
void test_encryption_performance() {
    AES256 aes;
    uint8_t key[32] = { /* 256-bit key */ };
    uint8_t block[16] = "TestBlock123456";
    
    unsigned long start = micros();
    for(int i = 0; i < 1000; i++) {
        aes.encryptBlock(block, block);
    }
    unsigned long duration = micros() - start;
    
    TEST_ASSERT_LESS_THAN(1000000, duration); // < 1 second
}
```

### Integration Tests

Integration tests require hardware or hardware simulators:

```bash
# Run hardware-in-loop tests
pio test -e hardware

# Or manually test specific features
pio test -e esp32s3 -f test_sim_detection
```

### Performance Benchmarking

```bash
# Build with profiling enabled
pio run -e profiling

# Upload and monitor
pio run -e profiling -t upload
pio device monitor
```

Monitor output will show:
- Heap usage
- CPU utilization
- AES encryption time
- Mesh message latency
- SIM switching time

---

## 🔐 Security & OTA

### Secure Boot (Production Recommendation)

For production deployment, enable secure boot:

1. Generate signing key:
   ```bash
   espsecure.py generate_signing_key secure_boot_signing_key.pem
   ```

2. Enable in `platformio.ini`:
   ```ini
   build_flags = 
       -D SECURE_BOOT_ENABLED=1
   ```

3. Flash bootloader with signed firmware

**⚠️ Warning:** Secure boot is irreversible once enabled!

### Flash Encryption

Enable flash encryption to protect firmware and data:

```ini
build_flags = 
    -D FLASH_ENCRYPTION_ENABLED=1
```

### OTA Updates

OTA update mechanism (to be implemented):

```cpp
// Pseudo-code for OTA
void performOTA(String firmwareURL) {
    // 1. Download firmware
    // 2. Verify signature
    // 3. Write to OTA partition
    // 4. Set boot partition
    // 5. Reboot
}
```

**Planned features:**
- Signed firmware verification
- Incremental updates
- Rollback on failure
- Version checking

---

## 📊 Known Gaps & Limitations

### 🔴 Critical Gaps (Must Fix)

1. **Hardcoded Secrets**
   - AES keys in `security_config.h`
   - HMAC keys hardcoded
   - **Fix:** Use NVS encrypted storage + remote key management

2. **No Mutual Authentication**
   - Nodes don't verify each other
   - **Fix:** Implement certificate-based authentication

3. **No Key Rotation**
   - Static encryption keys
   - **Fix:** Implement 24h key rotation as per PRD

4. **OTA Not Signed**
   - Firmware updates not verified
   - **Fix:** Implement Ed25519 signature verification

5. **Limited Testing**
   - Only one unit test example
   - No multi-node integration tests
   - **Fix:** Achieve >85% coverage per PRD

### 🟡 High Priority Gaps

1. **GSM Integration Incomplete**
   - AT command layer not fully implemented
   - No actual SMS sending tested
   - **Fix:** Complete GSM module integration

2. **SIM Health Scoring Missing**
   - Basic detection exists, no health metrics
   - **Fix:** Implement signal quality tracking

3. **Heartbeat Timing Not Validated**
   - 10s ±500ms precision not proven
   - **Fix:** Add timing validation tests

4. **Master Node Logic Missing**
   - No master/slave distinction
   - No load balancing
   - **Fix:** Implement master node coordinator

### 🟢 Medium Priority

1. **Plugin System** - Not implemented
2. **Advanced Routing** - Basic mesh only
3. **Power Optimization** - No sleep modes
4. **Diagnostics** - Limited debugging tools

---

## 🎯 Development Roadmap

### Phase 1: Security Hardening (Weeks 1-2)
- [ ] Remove hardcoded secrets
- [ ] Implement NVS encrypted storage
- [ ] Add certificate-based node auth
- [ ] Implement key rotation
- [ ] Add OTA signature verification
- [ ] Enable secure boot in build

### Phase 2: Core Features (Weeks 3-6)
- [ ] Complete GSM AT command layer
- [ ] Implement Persian SMS end-to-end
- [ ] Add SIM health scoring
- [ ] Implement master node logic
- [ ] Add hop-count routing optimization
- [ ] Complete MQTT bridge

### Phase 3: Testing (Weeks 7-8)
- [ ] Unit tests for all modules (>85%)
- [ ] Multi-node integration tests
- [ ] Hardware-in-loop automation
- [ ] Performance benchmarking
- [ ] 72-hour stability test

### Phase 4: Production Prep (Weeks 9-10)
- [ ] Documentation completion
- [ ] Production build scripts
- [ ] Manufacturing test suite
- [ ] Deployment automation
- [ ] Monitoring integration

---

## 🤝 Contributing

### Development Guidelines

1. **Code Style**
   - Follow Arduino/C++ style guide
   - Use meaningful variable names
   - Add comments for complex logic
   - Keep functions under 50 lines

2. **Testing**
   - Write unit tests for new features
   - Test on actual hardware before PR
   - Include performance benchmarks
   - Document test procedures

3. **Security**
   - Never commit secrets
   - Use secure random for keys
   - Validate all external inputs
   - Review crypto implementations

4. **Commits**
   - Use conventional commits
   - Keep commits atomic
   - Write descriptive messages

### Pull Request Process

1. Create feature branch from `main`
2. Implement feature with tests
3. Run `pio test` - all must pass
4. Update documentation
5. Submit PR with description
6. Address review feedback
7. Squash and merge

---

## 📚 Additional Resources

- **PlatformIO Docs:** https://docs.platformio.org/
- **ESP32-S3 Datasheet:** https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- **painlessMesh Library:** https://gitlab.com/painlessMesh/painlessMesh
- **Arduino ESP32:** https://docs.espressif.com/projects/arduino-esp32/
- **SIM800C AT Commands:** See `docs/hardware/SIM800C_AT_Commands.pdf`

---

## 📞 Support

- **Issues:** Report bugs via GitHub Issues
- **Discussions:** Technical questions in GitHub Discussions
- **Hardware:** Contact hardware team for PCB/schematic issues
- **Security:** Report vulnerabilities privately to security@your-domain.com

---

**Status:** Early-stage prototype (35% PRD compliance)  
**Next Milestone:** Security hardening & GSM integration  
**Target:** Production-ready firmware with full PRD compliance

