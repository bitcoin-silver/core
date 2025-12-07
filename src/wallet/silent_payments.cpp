// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/silent_payments.h>

#include <hash.h>
#include <hmac_sha256.h>
#include <key.h>
#include <primitives/block.h>
#include <script/script.h>
#include <span.h>
#include <util/bip32.h>
#include <wallet/wallet.h>

#include <algorithm>
#include <cstring>

// === SilentPaymentAddress ===

std::string SilentPaymentAddress::ToString() const
{
    if (encoded.empty()) {
        encoded = SilentPaymentUtils::EncodeAddress(scan_key, spend_key);
    }
    return encoded;
}

std::optional<SilentPaymentAddress> SilentPaymentAddress::FromString(const std::string& str)
{
    auto keys = SilentPaymentUtils::DecodeAddress(str);
    if (!keys) return std::nullopt;

    SilentPaymentAddress addr(keys->first, keys->second);
    addr.encoded = str;
    return addr;
}

// === SilentPaymentReceiver ===

SilentPaymentReceiver SilentPaymentReceiver::GenerateNew()
{
    CKey scan_key, spend_key;
    scan_key.MakeNewKey(true);   // Create random key, compressed
    spend_key.MakeNewKey(true);

    return SilentPaymentReceiver(scan_key, spend_key);
}

SilentPaymentAddress SilentPaymentReceiver::GetAddress() const
{
    return SilentPaymentAddress(m_scan_key.GetPubKey(), m_spend_key.GetPubKey());
}

std::vector<SilentPaymentReceiver::DetectedPayment> SilentPaymentReceiver::ScanTransaction(
    const CTransaction& tx,
    int block_height) const
{
    std::vector<DetectedPayment> payments;

    if (tx.vin.empty() || tx.vout.empty()) {
        return payments;
    }

    // Collect all input public keys
    std::vector<CPubKey> input_pubkeys;
    for (const auto& input : tx.vin) {
        // In a real implementation, we would extract pubkeys from previous outputs
        // For now, simplified placeholder
        CPubKey pubkey;
        // Extract from input script...
        // input_pubkeys.push_back(pubkey);
    }

    if (input_pubkeys.empty()) {
        return payments;
    }

    // For each output, try to derive shared secret
    for (uint32_t i = 0; i < tx.vout.size(); ++i) {
        const auto& output = tx.vout[i];

        // Try ECDH with each input
        for (const auto& input_pubkey : input_pubkeys) {
            // Compute shared secret using ECDH
            auto shared_secret = SilentPaymentUtils::ECDHSecret(m_scan_key, input_pubkey);
            if (!shared_secret) continue;

            // Derive output key from shared secret
            auto output_key = DeriveOutputKey(tx, i, *shared_secret);
            if (!output_key) continue;

            // Check if this matches the output script
            // (Simplified check - full implementation would verify script matches)
            DetectedPayment payment;
            payment.output_index = i;
            payment.output_script = output.scriptPubKey;
            payment.amount = output.nValue;
            payment.shared_secret = *shared_secret;

            payments.push_back(payment);
        }
    }

    return payments;
}

std::optional<CPubKey> SilentPaymentReceiver::DeriveOutputKey(
    const CTransaction& tx,
    uint32_t output_index,
    const uint256& tweak_data) const
{
    // Derive output key: spend_key + hash(tweak | output_index)
    std::vector<uint8_t> input_data(tweak_data.begin(), tweak_data.end());
    input_data.push_back(output_index & 0xff);

    uint256 tweak = Hash(input_data.begin(), input_data.end());
    return SilentPaymentUtils::TweakPublicKey(m_spend_key.GetPubKey(), tweak);
}

std::optional<CKey> SilentPaymentReceiver::DerivePrivateKey(
    const uint256& shared_secret,
    uint32_t output_index) const
{
    // Private key: spend_key + hash(shared_secret | output_index)
    std::vector<uint8_t> input_data(shared_secret.begin(), shared_secret.end());
    input_data.push_back(output_index & 0xff);

    uint256 tweak = Hash(input_data.begin(), input_data.end());
    return SilentPaymentUtils::TweakPrivateKey(m_spend_key, tweak);
}

// === SilentPaymentSender ===

std::optional<uint256> SilentPaymentSender::ComputeSharedSecret() const
{
    if (m_inputs.empty()) {
        return std::nullopt;
    }

    // Simplified ECDH computation from inputs (full impl: sum of ECDH results)
    // In BIP 352, this involves more complex aggregation
    return std::nullopt;  // Placeholder
}

std::optional<CPubKey> SilentPaymentSender::DeriveOutputKey(uint32_t payment_index) const
{
    auto shared_secret = ComputeSharedSecret();
    if (!shared_secret) return std::nullopt;

    // Derive output key: recipient.spend_key + hash(shared_secret | payment_index)
    std::vector<uint8_t> input_data(shared_secret->begin(), shared_secret->end());
    input_data.push_back(payment_index & 0xff);

    uint256 tweak = Hash(input_data.begin(), input_data.end());
    return SilentPaymentUtils::TweakPublicKey(m_recipient.spend_key, tweak);
}

std::optional<CTxOut> SilentPaymentSender::CreateOutput(uint32_t payment_index, CAmount amount) const
{
    auto output_key = DeriveOutputKey(payment_index);
    if (!output_key) return std::nullopt;

    CTxOut output;
    output.nValue = amount;
    // Create P2TR output from the derived key
    output.scriptPubKey = CScript() << output_key->data();

    return output;
}

bool SilentPaymentSender::VerifyOutput(const CPubKey& output_key, uint32_t payment_index) const
{
    auto derived = DeriveOutputKey(payment_index);
    return derived && *derived == output_key;
}

// === SilentPaymentWallet ===

std::optional<SilentPaymentAddress> SilentPaymentWallet::CreateAddress()
{
    if (!wallet) return std::nullopt;

    auto receiver = SilentPaymentReceiver::GenerateNew();
    receivers.push_back(receiver);

    auto address = receiver.GetAddress();
    address_index[address.ToString()] = receivers.size() - 1;

    return address;
}

std::vector<SilentPaymentAddress> SilentPaymentWallet::ListAddresses() const
{
    std::vector<SilentPaymentAddress> addresses;
    for (const auto& receiver : receivers) {
        addresses.push_back(receiver.GetAddress());
    }
    return addresses;
}

std::vector<SilentPaymentWallet::WalletPayment> SilentPaymentWallet::ScanBlock(
    const CBlock& block,
    int block_height)
{
    std::vector<WalletPayment> payments;

    for (const auto& tx : block.vtx) {
        for (size_t receiver_idx = 0; receiver_idx < receivers.size(); ++receiver_idx) {
            auto detected = receivers[receiver_idx].ScanTransaction(*tx, block_height);

            for (const auto& payment : detected) {
                WalletPayment wp;
                wp.address = receivers[receiver_idx].GetAddress().ToString();
                wp.amount = payment.amount;
                wp.txid = tx->GetHash();
                wp.output_index = payment.output_index;

                payments.push_back(wp);
            }
        }
    }

    return payments;
}

std::optional<CTransactionRef> SilentPaymentWallet::CreateSilentPayment(
    const std::string& recipient_address,
    CAmount amount,
    CAmount fee_rate)
{
    if (!wallet) return std::nullopt;

    // Decode recipient address
    auto recipient = SilentPaymentAddress::FromString(recipient_address);
    if (!recipient) return std::nullopt;

    // Create sender
    SilentPaymentSender sender(*recipient);

    // Build transaction (simplified)
    CMutableTransaction tx;
    tx.version = 2;
    tx.nLockTime = 0;

    // Add output using silent payment derivation
    auto output = sender.CreateOutput(0, amount);
    if (!output) return std::nullopt;

    tx.vout.push_back(*output);

    return MakeTransactionRef(tx);
}

CAmount SilentPaymentWallet::GetSilentPaymentBalance() const
{
    CAmount balance = 0;
    // Would sum up all unspent silent payment outputs
    return balance;
}

bool SilentPaymentWallet::ImportAddress(const SilentPaymentAddress& address)
{
    if (!wallet || !address.IsValid()) {
        return false;
    }

    auto receiver = SilentPaymentReceiver(address.scan_key, address.spend_key);
    receivers.push_back(receiver);
    address_index[address.ToString()] = receivers.size() - 1;

    return true;
}

// === BIP 352 Utility Functions ===

uint256 SilentPaymentUtils::HmacSHA256(const uint256& key, const std::vector<uint8_t>& data)
{
    CHMAC_SHA256 hmac(key.begin(), key.size());
    hmac.Write(data.data(), data.size());
    uint256 result;
    hmac.Finalize(result.begin());
    return result;
}

std::optional<uint256> SilentPaymentUtils::ECDHSecret(const CKey& private_key,
                                                      const CPubKey& public_key)
{
    // ECDH: hash(x-coordinate of (privkey * pubkey))
    if (!private_key.IsValid() || !public_key.IsValid()) {
        return std::nullopt;
    }

    // Compute point = private_key * public_key
    // (In a real impl, use actual ECDH library)
    // For now, return hash of combined data
    std::vector<uint8_t> combined(private_key.begin(), private_key.end());
    combined.insert(combined.end(), public_key.begin(), public_key.end());

    return Hash256(combined);
}

std::optional<CPubKey> SilentPaymentUtils::TweakPublicKey(const CPubKey& parent,
                                                         const uint256& tweak)
{
    // In real implementation, use secp256k1 library for proper point addition
    // For now, placeholder that shows structure
    return parent;
}

std::optional<CKey> SilentPaymentUtils::TweakPrivateKey(const CKey& parent,
                                                       const uint256& tweak)
{
    // Tweak private key: new_key = (old_key + tweak) mod n
    // Requires proper scalar arithmetic from secp256k1
    // Placeholder implementation
    return parent;
}

std::string SilentPaymentUtils::EncodeAddress(const CPubKey& scan_key, const CPubKey& spend_key)
{
    // Encode to bech32m format (placeholder)
    // Full implementation would use proper bech32m encoding
    std::string address = "sp1";  // Silent Payment address prefix
    // Would append encoded key data

    return address;
}

std::optional<std::pair<CPubKey, CPubKey>> SilentPaymentUtils::DecodeAddress(
    const std::string& address)
{
    // Decode from bech32m format (placeholder)
    if (!address.starts_with("sp1")) {
        return std::nullopt;
    }

    // Would decode and return (scan_key, spend_key)
    return std::nullopt;
}

uint256 SilentPaymentUtils::ComputeInputHash(const std::vector<CTxIn>& inputs)
{
    // BIP 352: compute hash of input public keys in a specific order
    std::vector<uint8_t> data;
    for (const auto& input : inputs) {
        // Extract and append public keys in sorted order
        // Placeholder
    }

    return Hash(data.begin(), data.end());
}
