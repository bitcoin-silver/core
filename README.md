# BitcoinSilver Core

**BitcoinSilver** is an enhanced fork of Bitcoin Core 30.x with innovative wallet features for cryptocurrency security and privacy. It maintains full compatibility with Bitcoin consensus rules while adding optional wallet-level features like **Vault Mechanisms** and planned **Silent Payments**.

## 🚀 Quick Overview

| Component | Details |
|-----------|---------|
| **Network** | Mainnet (port 10566), Testnet, Signet, Regtest |
| **Genesis** | July 12, 2024 |
| **Consensus** | Bitcoin Core 30.x compatible |
| **Binaries** | bitcoinsilverd, bitcoinsilver-cli, bitcoinsilver-qt |
| **Key Feature** | ⚡ **Vault Mechanisms** with time-locked recovery |

## ✨ Features

### 1. Vault Mechanisms (NEW)
**Time-locked recovery transactions** for enhanced cryptocurrency security:

- **Cold Storage Protection**: Automatically recover funds to cold storage after a configurable delay
- **CheckSequenceVerify Delays**: Customize time locks per vault (typical: 2-4 weeks)
- **Dual-Path Spending**: Both normal and recovery paths enforced cryptographically
- **Zero Consensus Changes**: Pure wallet-level implementation
- **Full RPC API**: Create, manage, and recover vaults via command line

**Use Case**: Protect against key compromise or theft by having funds automatically recoverable, giving you time to catch the attack before permanent loss.

### 2. Bitcoin Core 30.x Compatibility
- All standard transaction types (P2PKH, P2SH, P2WPKH, P2WSH, Taproot)
- BIP standards: BIP32, BIP39, BIP141, BIP340, BIP341, BIP325
- Full segwit and Taproot support
- Multi-signature support
- Network propagation and consensus validation

### 3. Silent Payments (Planned)
Bitcoin-compatible stealth addresses (BIP 352) for privacy without on-chain sender/receiver links.

## 🏗️ Quick Start

### Build (Linux/macOS)

```bash
# Clone and prepare
git clone https://github.com/bitcoin-silver/core.git
cd core

# Auto tools build
./autogen.sh
./configure
make -j$(nproc)

# Or CMake build
mkdir build && cd build
cmake .. && cmake --build . --parallel

# Run
./src/bitcoinsilverd --version
```

### Start Node

```bash
# Mainnet
bitcoinsilverd -server -rpcport=10567 -rpcuser=user -rpcpassword=pass

# Testnet
bitcoinsilverd -testnet -server

# Local regtest
bitcoinsilverd -regtest -server
bitcoinsilver-cli -regtest generatetoaddress 101 "BSomeAddress"
```

## 🔐 Vault Mechanisms - Complete Guide

### What is a Vault?

A vault is a Bitcoin smart contract that splits control into two paths:

**Path 1 - Normal Spending**: Send funds immediately to recipient  
**Path 2 - Recovery**: After CSV delay (e.g., 2016 blocks ≈ 2 weeks), recover to cold storage

### Vault Script Structure

```
OP_IF
  <recipient_pubkey> OP_CHECKSIG    # Normal path
OP_ELSE
  <csv_blocks> OP_CHECKSEQUENCEVERIFY OP_DROP
  <recovery_pubkey> OP_CHECKSIG     # Recovery path after delay
OP_ENDIF
```

**Benefits**:
- ✅ Uses only standard Bitcoin opcodes
- ✅ No consensus changes required
- ✅ Works with all wallet types
- ✅ Compatible with Taproot
- ✅ Simple to understand and audit

### RPC Commands

#### Create a Vault

```bash
bitcoinsilver-cli createvault \
  "BRecipientAddress" \
  "BColdStorageAddress" \
  2016 \
  "my-vault"
```

**Output**:
```json
{
  "vault_id": "vault_main_1703001234",
  "vault_address": "bs1qvault2ks9kfdfh3n...",
  "recipient": "BRecipientAddress",
  "recovery_address": "BColdStorageAddress",
  "csv_blocks": 2016
}
```

#### Send to Vault

```bash
bitcoinsilver-cli sendvaulted \
  "vault_main_1703001234" \
  "BDestinationAddress" \
  1.5
```

#### Monitor Vault Status

```bash
bitcoinsilver-cli getvaultinfo "vault_main_1703001234"
```

**Output**:
```json
{
  "vault_id": "vault_main_1703001234",
  "vault_address": "bs1qvault2ks9kfdfh3n...",
  "recipient": "BRecipientAddress",
  "recovery_address": "BColdStorageAddress",
  "csv_blocks": 2016,
  "status": "active"
}
```

#### Initiate Recovery

```bash
# If you suspect compromise or need to recover funds
bitcoinsilver-cli recovervault \
  "vault_main_1703001234" \
  "abc123...def456" \
  0

# Output shows recovery transaction ID and blocks until broadcast
```

### RPC API Reference

| Command | Purpose |
|---------|---------|
| `createvault <recipient> <recovery_addr> [csv] [label]` | Create vault |
| `sendvaulted <vault_id> <dest> <amount> [fee]` | Send to vault |
| `recovervault <vault_id> <txid> <idx>` | Initiate recovery |
| `getvaultinfo <vault_id>` | Get vault details |

### Vault Security Best Practices

✅ **DO**:
- Use separate key for recovery address (cold storage)
- Set CSV delay long enough to detect + respond (2-4 weeks typical)
- Test recovery before relying on production
- Keep recovery keys completely offline
- Document recovery procedures

❌ **DON'T**:
- Reuse recovery addresses across vaults
- Use small CSV delays (too risky)
- Mix recovery keys with hot wallets
- Lose backup of vault configuration

### Implementation Details

**Location**: `src/wallet/vault.*` and `src/wallet/rpc/vault.cpp`

**Classes**:
```cpp
struct VaultConfig;           // Vault configuration
class VaultBuilder;           // Transaction construction
class RecoveryManager;        // Recovery tracking
```

**No Consensus Changes**: Vaults are implemented as wallet-level features using existing Bitcoin opcodes.

## 🌍 Network Configuration

### Mainnet
- **Port**: 10566
- **Network ID**: f7f7f7f7
- **Genesis**: 00000ea8e97e04892a03df35947ff0c4df705723f5b18be7cc6456ed16e9788e
- **DNS Seeds**:
  - 213.165.83.94 (Olafs)
  - 78.138.45.19 (Elvas 1)
  - 109.205.181.171 (Elvas 2)
  - 2a06:1fc0:0:1::3e3 (IPv6)

### Testnet
- **Port**: 18333
- **Network ID**: 0b110907
- Resets periodically

### Signet
- **Port**: 38333
- Deterministic, good for stable testing

### Regtest (Local)
- **Port**: 18444
- Instant block generation for development

## 💻 Using Vaults Programmatically

### C++

```cpp
#include <wallet/vault.h>

// Create vault configuration
VaultConfig config(recipient, recovery, 2016, "my-vault");

// Build vault transaction
VaultBuilder builder(wallet.get(), config);
auto tx = builder.CreateVaultedTransaction(destination, amount);

// Send transaction...
```

### Command Line

```bash
# Full workflow
VAULT=$(bitcoinsilver-cli createvault "B..." "B..." 2016)
VAULT_ID=$(echo $VAULT | jq -r '.vault_id')

bitcoinsilver-cli sendvaulted "$VAULT_ID" "B..." 1.0

# Later if needed
bitcoinsilver-cli recovervault "$VAULT_ID" "abc..." 0
```

### Python

```python
import subprocess
import json

def create_vault(recipient, recovery, csv=2016):
    cmd = ["bitcoinsilver-cli", "createvault", recipient, recovery, str(csv)]
    result = subprocess.run(cmd, capture_output=True, text=True)
    return json.loads(result.stdout)

vault = create_vault("BAddress1", "BAddress2")
print(f"Vault ID: {vault['vault_id']}")
```

## 📝 Configuration

**File**: `~/.bitcoinsilver/bitcoinsilver.conf`

```ini
# Network
server=1
port=10566
rpcport=10567
rpcuser=bitcoinsilver
rpcpassword=yourpassword

# Vault defaults (optional)
vault_csv_default=2016
vault_recovery_address=BColdStorageAddr

# Performance
dbcache=1000
maxconnections=50

# Logging
debug=vault
debug=rpc
```

## 🧪 Testing

```bash
# Build with tests
make check

# Run test suite
./src/test/test_bitcoinsilver

# Functional tests (Python)
python3 test/functional/test_basic.py
```

## 🔧 RPC Examples

### Get Block Info
```bash
bitcoinsilver-cli getblockchaininfo
bitcoinsilver-cli getblock $(bitcoinsilver-cli getblockhash 0)
```

### Wallet Operations
```bash
bitcoinsilver-cli getwalletinfo
bitcoinsilver-cli listunspent
bitcoinsilver-cli getbalance
bitcoinsilver-cli getnewaddress
```

### Vault Operations
```bash
bitcoinsilver-cli createvault "B1..." "B2..."
bitcoinsilver-cli getvaultinfo "vault_..."
bitcoinsilver-cli recovervault "vault_..." "abc..." 0
```

## 📚 Documentation

- [Vault Mechanisms](#-vault-mechanisms---complete-guide) - Detailed vault guide
- [Build Instructions](doc/build-unix.md)
- [Developer Notes](doc/developer-notes.md)
- [Bitcoin Script Reference](https://en.bitcoin.it/wiki/Script)
- [BIPs](https://github.com/bitcoin/bips)

## 🤝 Contributing

We welcome contributions! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/xyz`)
3. Make your changes with tests
4. Submit a pull request

See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## 🛡️ Security

### Reporting Vulnerabilities

⚠️ **Do not** open public issues for security vulnerabilities.  
See [SECURITY.md](SECURITY.md) for responsible disclosure.

### Key Management

- Never expose private keys in configuration files
- Use strong passphrases for wallet encryption
- Keep recovery keys offline and secure
- Regularly backup wallet data

## 📜 License

BitcoinSilver Core is released under the MIT License. See [COPYING](COPYING).

Bitcoin Core is copyright (c) 2009-2024 The Bitcoin Core Developers.

## 📊 Key Differences from Bitcoin

| Feature | Bitcoin | BitcoinSilver |
|---------|---------|---------------|
| Port | 8333 | 10566 |
| Genesis | 2009-01-03 | 2024-07-12 |
| Currency | BTC | BS |
| Consensus | Bitcoin Core 30.x | Bitcoin Core 30.x ✓ |
| Vaults | ❌ No | ✅ Yes |
| Silent Payments | ❌ No | ⏳ Planned |

## 🚦 Status

- ✅ Bitcoin Core 30.x merged and tested
- ✅ Complete rebranding (bitcoin → bitcoinsilver)
- ✅ Vault mechanisms implemented
- ✅ RPC API for vault management
- ⏳ Silent Payments (in development)
- ⏳ GUI wallet enhancements

## 📞 Support

- **Repository**: https://github.com/bitcoin-silver/core
- **Issues**: https://github.com/bitcoin-silver/core/issues
- **Documentation**: See doc/ folder

---

**Version**: 30.1-BitcoinSilver  
**Last Updated**: December 2024  
**Status**: Production Ready

