// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINSILVER_WALLET_VAULT_H
#define BITCOINSILVER_WALLET_VAULT_H

#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <uint256.h>

#include <optional>
#include <string>
#include <vector>

class CWallet;

/**
 * Vault Configuration
 *
 * Represents a vault descriptor with a primary spending path and a time-locked recovery path.
 * Script pattern (using Taproot or P2WSH):
 *
 *   OP_IF
 *     <recipient_pubkey> OP_CHECKSIG    # Normal spend path
 *   OP_ELSE
 *     <csv_blocks> OP_CHECKSEQUENCEVERIFY OP_DROP
 *     <recovery_pubkey> OP_CHECKSIG     # Recovery path (after CSV delay)
 *   OP_ENDIF
 */
struct VaultConfig {
    CTxDestination recipient_address;      // Primary recipient
    CTxDestination recovery_address;        // Cold storage recovery address
    uint32_t csv_blocks;                    // CheckSequenceVerify delay (in blocks)
    std::string vault_id;                   // Unique identifier for this vault
    bool enabled = true;

    VaultConfig() = default;
    VaultConfig(const CTxDestination& recipient,
                const CTxDestination& recovery,
                uint32_t csv_delay,
                const std::string& id)
        : recipient_address(recipient),
          recovery_address(recovery),
          csv_blocks(csv_delay),
          vault_id(id) {}
};

/**
 * Vault Transaction Builder
 *
 * Handles construction of transactions with vault protection.
 * Supports both normal spending and recovery paths.
 */
class VaultBuilder {
private:
    CWallet* wallet;
    VaultConfig vault_config;

    /**
     * Build the vault script with IF/ELSE structure
     * @param recipient_pubkey Public key for normal spending
     * @param recovery_pubkey Public key for recovery path
     * @param csv_blocks Sequence verification delay
     * @return CScript containing the vault logic
     */
    CScript BuildVaultScript(const CPubKey& recipient_pubkey,
                            const CPubKey& recovery_pubkey,
                            uint32_t csv_blocks) const;

public:
    VaultBuilder(CWallet* w, const VaultConfig& config)
        : wallet(w), vault_config(config) {}

    /**
     * Create a vaulted transaction with time-locked recovery
     * @param destination Target address for funds
     * @param amount Amount to send
     * @param use_recovery_path If true, uses recovery path (requires CSV expiry)
     * @return CMutableTransaction with vault protection
     */
    std::optional<CMutableTransaction> CreateVaultedTransaction(
        const CTxDestination& destination,
        CAmount amount,
        bool use_recovery_path = false);

    /**
     * Add vault spending input to existing transaction
     * @param tx Transaction to modify
     * @param prev_out Previous vault output to spend from
     * @param use_recovery_path Whether to use recovery path
     * @return true if successful
     */
    bool AddVaultInput(CMutableTransaction& tx,
                      const COutPoint& prev_out,
                      bool use_recovery_path = false);

    /**
     * Create a recovery transaction after CSV expires
     * @param vault_txid Transaction ID of the vaulted transaction
     * @param vault_output_index Output index of vault UTXO
     * @param recovery_destination Recovery address (typically cold storage)
     * @param fee_rate Fee rate in sat/B
     * @return CMutableTransaction for recovery
     */
    std::optional<CMutableTransaction> CreateRecoveryTransaction(
        const uint256& vault_txid,
        uint32_t vault_output_index,
        const CTxDestination& recovery_destination,
        CAmount fee_rate);

    /**
     * Get vault script PubKey
     * @return CScript containing the vault address script
     */
    CScript GetVaultScript() const;

    /**
     * Get the vault address from config
     * @return Address string suitable for receiving funds
     */
    std::string GetVaultAddress() const;
};

/**
 * Recovery Manager
 *
 * Tracks pending recovery transactions and manages vault expiry times
 */
class RecoveryManager {
private:
    struct PendingRecovery {
        uint256 vault_txid;
        uint32_t output_index;
        uint32_t expiry_blocks;
        CTxDestination recovery_addr;
        CAmount amount;
    };

    std::vector<PendingRecovery> pending_recoveries;
    CWallet* wallet;

public:
    RecoveryManager(CWallet* w) : wallet(w) {}

    /**
     * Track a new vault for recovery purposes
     * @param vault_tx Vaulted transaction reference
     * @param recovery_addr Recovery address
     * @param expiry CSV blocks until recoverable
     */
    void TrackVault(const uint256& vault_txid,
                   uint32_t output_index,
                   const CTxDestination& recovery_addr,
                   uint32_t expiry,
                   CAmount amount);

    /**
     * Get list of currently recoverable vaults (at current height)
     * @param current_height Current block height
     * @return List of recoverable vault references
     */
    std::vector<PendingRecovery> GetRecoverableVaults(int current_height) const;

    /**
     * Mark vault as recovered
     * @param vault_txid Transaction ID of vault
     */
    void MarkRecovered(const uint256& vault_txid);

    /**
     * Get time until vault is recoverable
     * @param vault_txid Transaction ID of vault
     * @return Blocks remaining until recovery is possible
     */
    std::optional<uint32_t> GetRecoveryTimeRemaining(const uint256& vault_txid) const;
};

#endif // BITCOINSILVER_WALLET_VAULT_H
