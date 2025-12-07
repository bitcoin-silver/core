// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINSILVER_WALLET_SILENT_PAYMENTS_H
#define BITCOINSILVER_WALLET_SILENT_PAYMENTS_H

#include <key.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>

#include <optional>
#include <string>
#include <vector>

class CWallet;

/**
 * Silent Payments (BIP 352)
 *
 * A privacy-preserving payment scheme that hides the sender-recipient relationship on-chain.
 * 
 * Key Features:
 * - Receiver generates a Silent Payment address from scanning and spending keys
 * - Sender scans inputs to derive shared secret and outputs
 * - No persistent address reuse (improves privacy with each transaction)
 * - Compatible with Bitcoin protocol (uses only standard features)
 * - Recipient is only identifiable via scanning private key
 *
 * Privacy Properties:
 * - Sender identity hidden from observer
 * - Receiver identity hidden from observer  
 * - No on-chain link between sender and receiver
 * - Supports batched payments (multiple recipients in one tx)
 *
 * Reference: https://github.com/bitcoin/bips/blob/master/bip-0352.mediawiki
 */

/**
 * Silent Payment Address Components
 *
 * A silent payment address encodes scanning and spending public keys
 * in a way compatible with BIP 352 specification.
 */
struct SilentPaymentAddress {
    CPubKey scan_key;       // Public key for scanning (non-tweakable)
    CPubKey spend_key;      // Public key for spending (tweakable)
    
    // Optional: network encoding
    std::string encoded;    // Bech32m encoded address string
    
    SilentPaymentAddress() = default;
    SilentPaymentAddress(const CPubKey& scan, const CPubKey& spend)
        : scan_key(scan), spend_key(spend) {}

    /**
     * Check if this is a valid silent payment address
     */
    bool IsValid() const {
        return scan_key.IsValid() && spend_key.IsValid() && scan_key.IsCompressed();
    }

    /**
     * Serialize to bech32m string
     */
    std::string ToString() const;

    /**
     * Deserialize from bech32m string
     */
    static std::optional<SilentPaymentAddress> FromString(const std::string& str);
};

/**
 * Silent Payment Receiver
 *
 * Handles the receiver side of silent payments:
 * - Generates silent payment addresses
 * - Scans transactions for payments
 * - Derives output keys and scripts
 */
class SilentPaymentReceiver {
private:
    CKey m_scan_key;        // Private scanning key
    CKey m_spend_key;       // Private spending key

public:
    SilentPaymentReceiver() = default;
    explicit SilentPaymentReceiver(const CKey& scan_key, const CKey& spend_key)
        : m_scan_key(scan_key), m_spend_key(spend_key) {}

    /**
     * Generate a new random silent payment receiver
     */
    static SilentPaymentReceiver GenerateNew();

    /**
     * Get the silent payment address from receiver keys
     */
    SilentPaymentAddress GetAddress() const;

    /**
     * Get public scanning key
     */
    CPubKey GetScanKey() const { return m_scan_key.GetPubKey(); }

    /**
     * Get public spending key
     */
    CPubKey GetSpendKey() const { return m_spend_key.GetPubKey(); }

    /**
     * Scan a transaction for payments to this receiver
     * 
     * Uses ECDH to derive shared secrets from inputs and derives
     * the actual output keys. Returns payment info if found.
     *
     * @param tx Transaction to scan
     * @param block_height Current block height (for ordering)
     * @return List of detected silent payments in this transaction
     */
    struct DetectedPayment {
        uint32_t output_index;      // Index of detected output
        CScript output_script;      // The payment script
        CAmount amount;             // Amount received
        uint256 shared_secret;      // Derived shared secret
    };

    std::vector<DetectedPayment> ScanTransaction(const CTransaction& tx, int block_height) const;

    /**
     * Derive the output key for a given output index
     *
     * @param tx Transaction containing inputs
     * @param output_index Which output to derive for
     * @param tweak_data Additional tweak data
     * @return Derived public key for this output
     */
    std::optional<CPubKey> DeriveOutputKey(const CTransaction& tx,
                                          uint32_t output_index,
                                          const uint256& tweak_data) const;

    /**
     * Derive the private key for a detected silent payment
     * (for spending the received funds)
     *
     * @param shared_secret The shared secret from scanning
     * @param output_index Output index being derived
     * @return Private key for spending this output
     */
    std::optional<CKey> DerivePrivateKey(const uint256& shared_secret,
                                         uint32_t output_index) const;
};

/**
 * Silent Payment Sender
 *
 * Handles the sender side of silent payments:
 * - Encodes payment information in inputs
 * - Derives output keys from recipient address
 * - Creates outputs that only recipient can detect
 */
class SilentPaymentSender {
private:
    std::vector<CTxIn> m_inputs;
    const SilentPaymentAddress& m_recipient;

    /**
     * Compute shared secret using ECDH from sender inputs
     * Following BIP 352 specification
     */
    std::optional<uint256> ComputeSharedSecret() const;

public:
    explicit SilentPaymentSender(const SilentPaymentAddress& recipient)
        : m_recipient(recipient) {}

    /**
     * Add input to transaction for silent payment derivation
     * Inputs are used to create deterministic output keys
     */
    void AddInput(const CTxIn& input) { m_inputs.push_back(input); }

    /**
     * Derive the output key for a specific payment index
     *
     * Each payment gets a unique output key derived from:
     * - Recipient's public keys
     * - Sender's inputs (ECDH)
     * - Payment index
     *
     * @param payment_index Which payment this is (0, 1, 2, ...)
     * @return Derived public key for this output
     */
    std::optional<CPubKey> DeriveOutputKey(uint32_t payment_index) const;

    /**
     * Create a silent payment output script
     *
     * @param payment_index Payment index
     * @param amount Amount to send
     * @return CTxOut with proper script and amount
     */
    std::optional<CTxOut> CreateOutput(uint32_t payment_index, CAmount amount) const;

    /**
     * Verify that output keys were correctly derived
     * (useful for testing and validation)
     */
    bool VerifyOutput(const CPubKey& output_key, uint32_t payment_index) const;
};

/**
 * Silent Payment Wallet Integration
 *
 * Manages silent payment addresses and scanning for a wallet
 */
class SilentPaymentWallet {
private:
    CWallet* wallet;
    std::vector<SilentPaymentReceiver> receivers;
    std::map<std::string, int> address_index;  // Maps addresses to receiver index

public:
    explicit SilentPaymentWallet(CWallet* w) : wallet(w) {}

    /**
     * Create a new silent payment address
     * @return The new address details
     */
    std::optional<SilentPaymentAddress> CreateAddress();

    /**
     * List all silent payment addresses in wallet
     */
    std::vector<SilentPaymentAddress> ListAddresses() const;

    /**
     * Scan a block for all silent payments to this wallet
     * @param block Block data to scan
     * @param block_height Height of the block
     * @return Detected payments
     */
    struct WalletPayment {
        std::string address;
        CAmount amount;
        uint256 txid;
        uint32_t output_index;
    };

    std::vector<WalletPayment> ScanBlock(const CBlock& block, int block_height);

    /**
     * Create a silent payment transaction
     * @param recipient Address string (will be decoded)
     * @param amount Amount to send
     * @param fee_rate Fee rate in sat/vB
     * @return Signed transaction ready to broadcast
     */
    std::optional<CTransactionRef> CreateSilentPayment(
        const std::string& recipient_address,
        CAmount amount,
        CAmount fee_rate);

    /**
     * Get balance of silent payment addresses
     */
    CAmount GetSilentPaymentBalance() const;

    /**
     * Import a silent payment address to watch (for receive-only)
     */
    bool ImportAddress(const SilentPaymentAddress& address);
};

/**
 * BIP 352 Helper Functions
 */
namespace SilentPaymentUtils {
    /**
     * HMAC-SHA256 for BIP 352 key derivation
     */
    uint256 HmacSHA256(const uint256& key, const std::vector<uint8_t>& data);

    /**
     * Compute ECDH shared secret
     * result = hash(x-coordinate of (privkey * pubkey))
     */
    std::optional<uint256> ECDHSecret(const CKey& private_key, const CPubKey& public_key);

    /**
     * Derive child key from parent using tweak
     * child_key = parent_key + hash(tweak)
     */
    std::optional<CPubKey> TweakPublicKey(const CPubKey& parent, const uint256& tweak);

    /**
     * Same but for private keys
     */
    std::optional<CKey> TweakPrivateKey(const CKey& parent, const uint256& tweak);

    /**
     * Encode silent payment address to bech32m
     */
    std::string EncodeAddress(const CPubKey& scan_key, const CPubKey& spend_key);

    /**
     * Decode silent payment address from bech32m
     */
    std::optional<std::pair<CPubKey, CPubKey>> DecodeAddress(const std::string& address);

    /**
     * Compute input hash for ECDH (BIP 352 spec)
     */
    uint256 ComputeInputHash(const std::vector<CTxIn>& inputs);
}

#endif // BITCOINSILVER_WALLET_SILENT_PAYMENTS_H
