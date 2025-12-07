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

### 3. Silent Payments (NEW - BIP 352)
**Bitcoin-compatible stealth addresses** for private transactions:

- **Sender Anonymity**: Sender-receiver relationship hidden on-chain
- **Unique Outputs**: Each payment creates unique derived address
- **ECDH-based**: Uses standard Elliptic Curve Diffie-Hellman
- **Recipient Scanning**: Only recipient can detect payments via scanning private key
- **Zero Consensus Changes**: Pure wallet-level, uses only standard Bitcoin features
- **BIP 352 Compliant**: Full compatibility with Bitcoin Silent Payments standard

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

## 💳 Silent Payments - Privacy Transactions (BIP 352)

### What are Silent Payments?

Silent Payments are a privacy-preserving way to receive cryptocurrency without creating an on-chain link between sender and receiver. Unlike regular Bitcoin addresses that can be reused and traced, silent payments generate unique, untrackable outputs.

**Privacy Flow**:
1. **Receiver** publishes a Silent Payment address (scan_key + spend_key)
2. **Sender** scans the blockchain for receiver inputs and derives a unique output key
3. **Payment** is created to that derived key - invisible to observers
4. **Receiver** automatically detects the payment by scanning with their private key

**Key Advantage**: No blockchain observer can link sender to receiver, and each payment is unique even from the same sender.

### Creating Silent Payment Addresses

```bash
# Create a new silent payment address
bitcoinsilver-cli silentpaymentaddress "my-sp-address"
```

**Output**:
```json
{
  "sp_address": "sp1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq",
  "scan_key": "02a1b2c3d4e5f6...",
  "spend_key": "03f6e5d4c3b2a1...",
  "label": "my-sp-address"
}
```

**Security Notes**:
- Share only `sp_address` publicly
- Keep `scan_key` in your hot wallet (needed for auto-scanning)
- Backup `spend_key` to cold storage (needed for spending)

### Sending with Silent Payments

```bash
# Send 1.5 BTC to a silent payment address
bitcoinsilver-cli sendsilent \
  "sp1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq" \
  1.5 \
  0.001 \
  false

# Output:
{
  "txid": "abc123def456...",
  "size": 234,
  "status": "broadcasted"
}
```

**How it works**:
1. Your wallet scans the recipient's recent inputs
2. Computes ECDH shared secret with each input pubkey
3. Derives unique output key for this specific transaction
4. Creates output to that key (recipient can detect it, but observer cannot)

### Monitoring Silent Payments

```bash
# Check total balance of all silent payment addresses
bitcoinsilver-cli silentpaymentbalance

# Output:
{
  "balance": "2.50000000",
  "confirmations_needed": 6
}
```

```bash
# List all your silent payment addresses
bitcoinsilver-cli listsilentpaymentaddresses true

# Output:
[
  {
    "address": "sp1qqq...",
    "scan_key": "02...",
    "spend_key": "03..."
  },
  {
    "address": "sp1qqq...",
    "scan_key": "02...",
    "spend_key": "03..."
  }
]
```

### Importing Silent Payment Addresses

For receive-only wallets (cold storage), you can import a silent payment address to scan for incoming payments without holding spending keys:

```bash
# Import a watch-only silent payment address
bitcoinsilver-cli importsilentpayment \
  "sp1qqq..." \
  "watch-only-cold-storage"

# Output:
{
  "success": true,
  "address": "sp1qqq...",
  "label": "watch-only-cold-storage"
}
```

This allows:
- ✅ Scanning for incoming payments
- ✅ Deriving output addresses
- ❌ Spending received funds (need the spend_key for that)

### Understanding Silent Payment Cryptography

Silent Payments use **ECDH (Elliptic Curve Diffie-Hellman)** for key derivation:

```
Sender Side:
  input_keys = extract_pubkeys(transaction_inputs)
  shared_secret = ECDH(sender_privkey, receiver_scan_pubkey)
  output_key = receiver_spend_pubkey + hash(shared_secret || payment_index)
  → Create output to output_key

Receiver Side:
  For each transaction output:
    shared_secret = ECDH(receiver_scan_privkey, input_pubkey)
    expected_key = receiver_spend_pubkey + hash(shared_secret || output_index)
    If output_key == expected_key:
      → This payment is for me!
      receive_key = receiver_spend_privkey + hash(shared_secret || output_index)
      → Can now spend this output
```

**Security Properties**:
- Only receiver (with scan_key) can detect payments
- Only receiver (with spend_key) can spend payments
- No on-chain link between sender and receiver
- Each payment creates unique derived key
- Compatible with all Bitcoin script types

### Silent Payments in Code

**C++**:
```cpp
#include <wallet/silent_payments.h>

// Create silent payment receiver
auto receiver = SilentPaymentReceiver::GenerateNew();
auto sp_address = receiver.GetAddress();

std::cout << "Your Silent Payment Address: " << sp_address.ToString() << std::endl;
std::cout << "Scan Key: " << receiver.GetScanKey().GetHex() << std::endl;

// Later: scan a transaction for payments
auto payments = receiver.ScanTransaction(tx, block_height);
for (const auto& payment : payments) {
    std::cout << "Detected payment of " << payment.amount << " satoshis" << std::endl;
    
    // Derive the private key to spend this output
    auto spending_key = receiver.DerivePrivateKey(payment.shared_secret, payment.output_index);
}
```

**Command Line Workflow**:
```bash
#!/bin/bash

# 1. Create a new silent payment address
SP_ADDR=$(bitcoinsilver-cli silentpaymentaddress "my-address")
ADDRESS=$(echo $SP_ADDR | jq -r '.sp_address')
echo "Your Silent Payment Address: $ADDRESS"

# 2. Share address with sender
# (send $ADDRESS to your counterparty)

# 3. Sender sends funds
bitcoinsilver-cli sendsilent "$ADDRESS" 1.0

# 4. Your wallet automatically detects it
sleep 10  # Wait for confirmation
BALANCE=$(bitcoinsilver-cli silentpaymentbalance)
echo "New balance: $BALANCE"

# 5. Spend the received funds
bitcoinsilver-cli sendsilent "$ANOTHER_ADDRESS" 0.5
```

### Silent Payments vs Traditional Bitcoin Addresses

| Feature | Traditional Address | Silent Payment |
|---------|-------------------|-----------------|
| **Address Reuse** | Yes (public link) | No (unique per payment) |
| **Sender Privacy** | No | ✅ Yes |
| **Receiver Privacy** | No | ✅ Yes |
| **On-Chain Analysis** | Linkable | Unlinkable |
| **Scanning Required** | No | Yes (automatic) |
| **Complexity** | Simple | Medium |
| **Bitcoin Compatible** | Native | ECDH-derived |

### Silent Payments vs Vault Mechanisms

| Feature | Silent Payments | Vault Mechanisms |
|---------|-----------------|------------------|
| **Primary Goal** | Privacy | Security (theft protection) |
| **On-Chain Visible** | Hidden | Visible (but time-locked) |
| **Recovery Path** | Not applicable | Yes (CSV delay) |
| **Address Reuse** | No (unique outputs) | Yes (same vault) |
| **Scanning** | Required | Not required |
| **Use Case** | Hide identity | Protect against theft |

### Best Practices for Silent Payments

✅ **DO**:
- Create multiple silent payment addresses for different contacts
- Backup both scan_key and spend_key securely
- Test send/receive in testnet first
- Use long addresses (more data = more privacy)
- Monitor scanning for performance
- Combine with Tor for network privacy

❌ **DON'T**:
- Share spend_key with anyone
- Lose scan_key (can't detect payments)
- Reuse same SP address across different contexts
- Assume complete anonymity (chain analysis possible)
- Mix SP outputs with regular UTXO tracking
- Disable wallet scanning (automatic detection needed)

### RPC Reference for Silent Payments

| Command | Purpose |
|---------|---------|
| `silentpaymentaddress [label]` | Create new SP address |
| `sendsilent <sp_addr> <amount> [fee_rate]` | Send with SP |
| `silentpaymentbalance` | Get SP balance |
| `listsilentpaymentaddresses [keys]` | List SP addresses |
| `importsilentpayment <addr> [label]` | Import watch-only |

### Limitations & Future Improvements

**Current Implementation**:
- ✅ Full BIP 352 architecture
- ✅ RPC command suite
- ✅ ECDH-based key derivation
- ⏳ Optimized block scanning
- ⏳ Hardware wallet integration
- ⏳ Qt wallet UI
- ⏳ Batch payment optimization

**Performance**:
- Scanning requires testing private key against each output
- For many addresses, this can be CPU intensive
- Future: BIP 352 client-side filtering opcodes

### Resources

- **BIP 352 Specification**: https://github.com/bitcoin/bips/blob/master/bip-0352.mediawiki
- **Silent Payments Documentation**: https://silentpayments.xyz
- **Cryptography**: secp256k1 ECDH + SHA256-HMAC

## 🔗 Combining Vault Mechanisms + Silent Payments

You can use both features together for **maximum security AND privacy**:

### Use Case 1: Secure Cold Storage Receive

```bash
# 1. Create vault for security (recovery after 2 weeks if stolen)
VAULT=$(bitcoinsilver-cli createvault "BHotWallet" "BColdStorage" 2016)
VAULT_ID=$(echo $VAULT | jq -r '.vault_id')

# 2. Create silent payment for privacy (no one knows you received it)
SP_ADDR=$(bitcoinsilver-cli silentpaymentaddress "private-receive")
SP_ADDRESS=$(echo $SP_ADDR | jq -r '.sp_address')

# 3. Share SP_ADDRESS with counterparty
# They send with: bitcoinsilver-cli sendsilent "$SP_ADDRESS" 1.0

# Result:
# ✅ Privacy: Counterparty cannot be linked to you
# ✅ Security: Funds auto-recoverable to cold storage if compromised
```

### Use Case 2: Multi-Sig + Time-Lock + Privacy

```bash
# Create multi-sig wallet for team
# Then vault each multi-sig output
# Then receive via silent payments
# Result: Highest security + best privacy
```

### Feature Comparison

| Scenario | Solution |
|----------|----------|
| **Receive privately** | Silent Payments |
| **Protect from theft** | Vault Mechanisms |
| **Both** | Vault + Silent Payments |
| **Hide identity** | Silent Payments |
| **Recover if hacked** | Vault Mechanisms |
| **Maximum protection** | Vault + Silent Payments |

### Combined Workflow Example

```bash
#!/bin/bash

# Setup Phase
echo "=== Setting up secure & private wallet ==="

# 1. Create vault for emergency recovery
VAULT=$(bitcoinsilver-cli createvault \
  "B_your_hot_wallet" \
  "B_your_cold_storage" \
  2016)  # 2 week recovery window

VAULT_ID=$(echo $VAULT | jq -r '.vault_id')
echo "✅ Vault created: $VAULT_ID"

# 2. Create silent payment address for receiving
SP_ADDR=$(bitcoinsilver-cli silentpaymentaddress "my-private-address")
SP_ADDRESS=$(echo $SP_ADDR | jq -r '.sp_address')
echo "✅ Silent Payment address: $SP_ADDRESS"

# Distribution Phase
echo "Share this address with payers:"
echo "$SP_ADDRESS"

# Payment Reception Phase (automatic)
# When payer does: bitcoinsilver-cli sendsilent "$SP_ADDRESS" 1.0
# Your wallet:
# 1. Automatically detects payment (via scanning)
# 2. Funds go to vaulted output (protected by time-lock)
# 3. No observer can link payer to you

# Spending Phase
echo "=== Spending received funds ==="

# If all is normal, spend normally to recipient
bitcoinsilver-cli sendsilent "sp1q_recipient_address" 0.5

# If you detect suspicious activity:
# 1. Initiate recovery to cold storage
bitcoinsilver-cli recovervault "$VAULT_ID" "vault_txid" 0
# 2. Wait 2 weeks (2016 blocks)
# 3. Funds automatically recover to cold storage
echo "Recovery initiated - funds safe in 2 weeks"
```

### Privacy + Security Properties

With **Both** features:
- 🔒 **Sender Privacy**: Who paid you is hidden
- 🔒 **Receiver Privacy**: That you received payment is hidden
- 🔐 **Theft Protection**: Funds auto-recoverable
- 🔐 **Time Cushion**: 2 weeks to detect and respond to theft
- ⚡ **Automatic**: Scanning and recovery are automatic
- 🔑 **Full Control**: You control both paths

### When to Use Each Feature

**Use Vault Mechanisms** when:
- You want protection against key theft
- You can afford to wait (CSV delay) for emergency access
- You run a business with high-value transactions
- You want deterministic recovery

**Use Silent Payments** when:
- You need privacy from public/observer scrutiny
- You want to hide transaction history
- You receive from multiple sources
- You need deniability

**Use BOTH** when:
- You need maximum security AND privacy (recommended for serious users)
- You're managing significant cryptocurrency
- You operate in hostile environments
- You want defense-in-depth

## 📝 Configuration

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
| Silent Payments | ❌ No | ✅ Yes (BIP 352) |

## 🚦 Status

- ✅ Bitcoin Core 30.x merged and tested
- ✅ Complete rebranding (bitcoin → bitcoinsilver)
- ✅ Vault mechanisms implemented with RPC API
- ✅ Silent Payments (BIP 352) implemented
- ⏳ Qt wallet enhancements
- ⏳ Hardware wallet integration

## 📞 Support

- **Repository**: https://github.com/bitcoin-silver/core
- **Issues**: https://github.com/bitcoin-silver/core/issues
- **Documentation**: See doc/ folder

---

**Version**: 30.1-BitcoinSilver  
**Last Updated**: December 2024  
**Status**: Production Ready

