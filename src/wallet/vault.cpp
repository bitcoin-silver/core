// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/vault.h>

#include <key.h>
#include <script/interpreter.h>
#include <script/script.h>
#include <script/standard.h>
#include <wallet/wallet.h>
#include <util/strencodings.h>

#include <algorithm>

CScript VaultBuilder::BuildVaultScript(const CPubKey& recipient_pubkey,
                                       const CPubKey& recovery_pubkey,
                                       uint32_t csv_blocks) const
{
    // Vault script using Taproot-compatible structure
    // OP_IF
    //   <recipient> OP_CHECKSIG
    // OP_ELSE
    //   <csv_delay> OP_CHECKSEQUENCEVERIFY OP_DROP
    //   <recovery> OP_CHECKSIG
    // OP_ENDIF

    CScript script;
    script << OP_IF;
    script << ToByteVector(recipient_pubkey) << OP_CHECKSIG;
    script << OP_ELSE;
    script << CScriptNum(csv_blocks) << OP_CHECKSEQUENCEVERIFY << OP_DROP;
    script << ToByteVector(recovery_pubkey) << OP_CHECKSIG;
    script << OP_ENDIF;

    return script;
}

std::optional<CMutableTransaction> VaultBuilder::CreateVaultedTransaction(
    const CTxDestination& destination,
    CAmount amount,
    bool use_recovery_path)
{
    if (!wallet) return std::nullopt;

    CMutableTransaction tx;
    tx.version = 2;
    tx.nLockTime = 0;

    // Get pubkeys from wallet
    CKey recipient_key, recovery_key;
    CPubKey recipient_pubkey, recovery_pubkey;

    // For now, use simplified key derivation - in production, would use proper derivation
    if (!wallet->GetKey(vault_config.recipient_address, recipient_key)) {
        return std::nullopt;
    }
    recipient_pubkey = recipient_key.GetPubKey();

    if (!wallet->GetKey(vault_config.recovery_address, recovery_key)) {
        return std::nullopt;
    }
    recovery_pubkey = recovery_key.GetPubKey();

    // Build vault script
    CScript vault_script = BuildVaultScript(recipient_pubkey, recovery_pubkey, vault_config.csv_blocks);

    // Create output with vault script
    CTxOut vault_output;
    vault_output.nValue = amount;
    vault_output.scriptPubKey = GetScriptForDestination(destination);

    tx.vout.push_back(vault_output);

    return tx;
}

bool VaultBuilder::AddVaultInput(CMutableTransaction& tx,
                                 const COutPoint& prev_out,
                                 bool use_recovery_path)
{
    if (!wallet) return false;

    CTxIn vault_input;
    vault_input.prevout = prev_out;

    if (use_recovery_path) {
        // Recovery path: set nSequence to 1 to trigger CSV verification
        vault_input.nSequence = vault_config.csv_blocks;
    } else {
        // Normal spending path: disable sequence verification
        vault_input.nSequence = 0xfffffffe;
    }

    tx.vin.push_back(vault_input);
    return true;
}

std::optional<CMutableTransaction> VaultBuilder::CreateRecoveryTransaction(
    const uint256& vault_txid,
    uint32_t vault_output_index,
    const CTxDestination& recovery_destination,
    CAmount fee_rate)
{
    if (!wallet) return std::nullopt;

    CMutableTransaction recovery_tx;
    recovery_tx.version = 2;
    recovery_tx.nLockTime = 0;

    // Add input from vaulted UTXO with CSV constraint
    COutPoint vault_outpoint(vault_txid, vault_output_index);
    if (!AddVaultInput(recovery_tx, vault_outpoint, true)) {
        return std::nullopt;
    }

    // Add output to recovery address
    CTxOut recovery_output;
    recovery_output.scriptPubKey = GetScriptForDestination(recovery_destination);

    // Calculate amount (input amount - fees, simplified)
    recovery_output.nValue = 0; // Would be set to actual UTXO value - fees

    recovery_tx.vout.push_back(recovery_output);

    return recovery_tx;
}

CScript VaultBuilder::GetVaultScript() const
{
    CKey recipient_key, recovery_key;
    if (!wallet || !wallet->GetKey(vault_config.recipient_address, recipient_key) ||
        !wallet->GetKey(vault_config.recovery_address, recovery_key)) {
        return CScript();
    }

    return BuildVaultScript(recipient_key.GetPubKey(), recovery_key.GetPubKey(), vault_config.csv_blocks);
}

std::string VaultBuilder::GetVaultAddress() const
{
    // Get the vault script
    CScript script = GetVaultScript();
    if (script.empty()) return "";

    // Convert to address based on script type
    // For P2WSH (Witness Script Hash) addresses
    uint256 script_hash = Hash160(script.begin(), script.end());
    return EncodeDestination(WitnessV0KeyHash(script_hash));
}

// RecoveryManager implementation

void RecoveryManager::TrackVault(const uint256& vault_txid,
                               uint32_t output_index,
                               const CTxDestination& recovery_addr,
                               uint32_t expiry,
                               CAmount amount)
{
    PendingRecovery pending;
    pending.vault_txid = vault_txid;
    pending.output_index = output_index;
    pending.recovery_addr = recovery_addr;
    pending.expiry_blocks = expiry;
    pending.amount = amount;

    pending_recoveries.push_back(pending);
}

std::vector<RecoveryManager::PendingRecovery> RecoveryManager::GetRecoverableVaults(
    int current_height) const
{
    std::vector<PendingRecovery> recoverable;

    for (const auto& pending : pending_recoveries) {
        if (current_height >= static_cast<int>(pending.expiry_blocks)) {
            recoverable.push_back(pending);
        }
    }

    return recoverable;
}

void RecoveryManager::MarkRecovered(const uint256& vault_txid)
{
    auto it = std::remove_if(pending_recoveries.begin(), pending_recoveries.end(),
                            [vault_txid](const PendingRecovery& p) {
                                return p.vault_txid == vault_txid;
                            });
    pending_recoveries.erase(it, pending_recoveries.end());
}

std::optional<uint32_t> RecoveryManager::GetRecoveryTimeRemaining(const uint256& vault_txid) const
{
    auto it = std::find_if(pending_recoveries.begin(), pending_recoveries.end(),
                          [vault_txid](const PendingRecovery& p) {
                              return p.vault_txid == vault_txid;
                          });

    if (it == pending_recoveries.end()) {
        return std::nullopt;
    }

    // Would need current chain height to calculate actual time remaining
    return it->expiry_blocks;
}
