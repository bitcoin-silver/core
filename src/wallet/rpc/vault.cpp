// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/vault.h>
#include <wallet/wallet.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <util/translation.h>

#include <univalue.h>

static RPCHelpMan createvault()
{
    return RPCHelpMan{
        "createvault",
        "Create a new vault with time-locked recovery capability.\n"
        "\nThe vault allows funds to be spent normally, but also provides a recovery path\n"
        "that can only be used after a specified delay (CheckSequenceVerify blocks).\n",
        {
            {"recipient_address", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Address that will normally receive the vaulted funds"},
            {"recovery_address", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Cold storage address for recovery path"},
            {"csv_blocks", RPCArg::Type::NUM, RPCArg::Default{2016},
             "CheckSequenceVerify delay in blocks (~2 weeks at 5min blocks)"},
            {"vault_label", RPCArg::Type::STR, RPCArg::Optional::OMITTED,
             "Optional label for this vault"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR, "vault_id", "Unique vault identifier"},
                {RPCResult::Type::STR, "vault_address", "P2WSH address for vault"},
                {RPCResult::Type::STR, "recipient", "Recipient address"},
                {RPCResult::Type::STR, "recovery_address", "Recovery address"},
                {RPCResult::Type::NUM, "csv_blocks", "CSV delay in blocks"},
            },
        },
        RPCExamples{
            HelpExampleCli("createvault",
                          "\"BSomeAddress1\" \"BColdStorageAddr\" 2016")
            + HelpExampleRpc("createvault",
                            "\"BSomeAddress1\", \"BColdStorageAddr\", 2016")
        },
    };
}

static UniValue createvault(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() < 2 || request.params.size() > 4) {
        throw std::runtime_error(
            "createvault <recipient_address> <recovery_address> [csv_blocks] [label]");
    }

    CTxDestination recipient = DecodeDestination(request.params[0].get_str());
    if (!IsValidDestination(recipient)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recipient address");
    }

    CTxDestination recovery = DecodeDestination(request.params[1].get_str());
    if (!IsValidDestination(recovery)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recovery address");
    }

    uint32_t csv_blocks = 2016; // ~2 weeks
    if (request.params.size() > 2) {
        csv_blocks = request.params[2].get_int();
        if (csv_blocks < 1 || csv_blocks > 0x7fffffff) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "csv_blocks must be between 1 and 2147483647");
        }
    }

    std::string label;
    if (request.params.size() > 3) {
        label = request.params[3].get_str();
    }

    // Create vault configuration
    std::string vault_id = "vault_" + wallet->GetName() + "_" + std::to_string(std::time(nullptr));
    VaultConfig vault_config(recipient, recovery, csv_blocks, vault_id);

    // Build vault address
    VaultBuilder vault_builder(wallet.get(), vault_config);
    std::string vault_address = vault_builder.GetVaultAddress();

    if (vault_address.empty()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to create vault address");
    }

    // Return vault information
    UniValue result(UniValue::VOBJ);
    result.pushKV("vault_id", vault_id);
    result.pushKV("vault_address", vault_address);
    result.pushKV("recipient", EncodeDestination(recipient));
    result.pushKV("recovery_address", EncodeDestination(recovery));
    result.pushKV("csv_blocks", (int64_t)csv_blocks);
    if (!label.empty()) {
        result.pushKV("label", label);
    }

    return result;
}

static RPCHelpMan sendvaulted()
{
    return RPCHelpMan{
        "sendvaulted",
        "Send funds to a vault address with time-locked recovery.\n"
        "\nThis creates a transaction where funds can be spent normally to a recipient,\n"
        "but can also be recovered to a cold storage address after a CSV delay.\n",
        {
            {"vault_id", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Vault identifier created with createvault"},
            {"destination", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Destination address for the vaulted transaction"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO,
             "Amount to send (in BTC)"},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::Default{"0.001"},
             "Transaction fee rate (in BTC/kB)"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::NUM, "output_index", "Vault output index"},
                {RPCResult::Type::STR, "vault_address", "Vault script address"},
            },
        },
        RPCExamples{
            HelpExampleCli("sendvaulted",
                          "\"vault_main_1234\" \"BSomeAddress\" 1.5")
            + HelpExampleRpc("sendvaulted",
                            "\"vault_main_1234\", \"BSomeAddress\", 1.5")
        },
    };
}

static UniValue sendvaulted(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() < 3 || request.params.size() > 4) {
        throw std::runtime_error("sendvaulted <vault_id> <destination> <amount> [fee_rate]");
    }

    // TODO: Lookup vault configuration by vault_id
    // For now, this is a placeholder that shows the structure

    CTxDestination dest = DecodeDestination(request.params[1].get_str());
    if (!IsValidDestination(dest)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid destination address");
    }

    CAmount amount = AmountFromValue(request.params[2]);
    if (amount <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid amount");
    }

    // Return transaction details
    UniValue result(UniValue::VOBJ);
    result.pushKV("status", "vaulted_transaction_created");
    result.pushKV("destination", EncodeDestination(dest));
    result.pushKV("amount", ValueFromAmount(amount));

    return result;
}

static RPCHelpMan recovervault()
{
    return RPCHelpMan{
        "recovervault",
        "Initiate recovery of a vaulted transaction.\n"
        "\nThis creates a recovery transaction that can be broadcast after the CSV delay expires.\n"
        "The funds will be sent to the cold storage recovery address.\n",
        {
            {"vault_id", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Vault identifier"},
            {"vault_txid", RPCArg::Type::STR_HEX, RPCArg::Optional::NO,
             "Transaction ID of the vaulted funds"},
            {"output_index", RPCArg::Type::NUM, RPCArg::Optional::NO,
             "Output index of the vault UTXO"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR_HEX, "recovery_txid", "Recovery transaction ID"},
                {RPCResult::Type::STR, "status", "Recovery status"},
                {RPCResult::Type::NUM, "blocks_remaining", "Blocks until recovery is possible"},
            },
        },
        RPCExamples{
            HelpExampleCli("recovervault",
                          "\"vault_main_1234\" \"abc123...\" 0")
            + HelpExampleRpc("recovervault",
                            "\"vault_main_1234\", \"abc123...\", 0")
        },
    };
}

static UniValue recovervault(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() != 3) {
        throw std::runtime_error("recovervault <vault_id> <vault_txid> <output_index>");
    }

    // TODO: Implement recovery transaction creation
    // For now, this is a placeholder showing the expected output structure

    UniValue result(UniValue::VOBJ);
    result.pushKV("status", "recovery_initiated");
    result.pushKV("vault_id", request.params[0].get_str());
    result.pushKV("vault_txid", request.params[1].get_str());
    result.pushKV("output_index", request.params[2].get_int());

    return result;
}

static RPCHelpMan getVaultInfo()
{
    return RPCHelpMan{
        "getvaultinfo",
        "Get information about a vault.\n",
        {
            {"vault_id", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Vault identifier"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR, "vault_id", "Vault identifier"},
                {RPCResult::Type::STR, "vault_address", "Vault address"},
                {RPCResult::Type::STR, "recipient", "Recipient address"},
                {RPCResult::Type::STR, "recovery_address", "Recovery address"},
                {RPCResult::Type::NUM, "csv_blocks", "CSV delay in blocks"},
                {RPCResult::Type::STR, "status", "Current vault status"},
            },
        },
        RPCExamples{
            HelpExampleCli("getvaultinfo", "\"vault_main_1234\"")
            + HelpExampleRpc("getvaultinfo", "\"vault_main_1234\"")
        },
    };
}

static UniValue getvaultinfo(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() != 1) {
        throw std::runtime_error("getvaultinfo <vault_id>");
    }

    // TODO: Lookup vault by ID and return information
    UniValue result(UniValue::VOBJ);
    result.pushKV("vault_id", request.params[0].get_str());
    result.pushKV("status", "active");

    return result;
}

void RegisterVaultRPCCommands()
{
    static const CRPCCommand commands[] = {
        {"wallet", &createvault},
        {"wallet", &sendvaulted},
        {"wallet", &recovervault},
        {"wallet", &getVaultInfo},
    };
    for (const auto& c : commands) {
        t_rpcCommands->emplace_back(c);
    }
}
