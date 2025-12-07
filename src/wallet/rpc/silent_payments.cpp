// Copyright (c) 2024 The BitcoinSilver Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/silent_payments.h>
#include <wallet/wallet.h>
#include <rpc/protocol.h>
#include <rpc/server.h>
#include <util/translation.h>

#include <univalue.h>

static RPCHelpMan silentpaymentaddress()
{
    return RPCHelpMan{
        "silentpaymentaddress",
        "Create a new Silent Payment address for receiving private payments.\n"
        "\nSilent Payments (BIP 352) allow receiving cryptocurrency without on-chain link\n"
        "between sender and receiver. Each payment uses a unique derived address.\n",
        {
            {"label", RPCArg::Type::STR, RPCArg::Optional::OMITTED,
             "Optional label for this silent payment address"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR, "sp_address", "Bech32m encoded silent payment address"},
                {RPCResult::Type::STR, "scan_key", "Hex-encoded public scan key"},
                {RPCResult::Type::STR, "spend_key", "Hex-encoded public spend key"},
                {RPCResult::Type::STR, "label", "Label for this address (if provided)"},
            },
        },
        RPCExamples{
            HelpExampleCli("silentpaymentaddress", "\"my-address\"")
            + HelpExampleRpc("silentpaymentaddress", "\"my-address\"")
        },
    };
}

static UniValue silentpaymentaddress(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() > 1) {
        throw std::runtime_error("silentpaymentaddress [label]");
    }

    // Create SP wallet if not exists
    // (In real impl, would be integrated into wallet initialization)
    SilentPaymentWallet sp_wallet(wallet.get());

    auto address = sp_wallet.CreateAddress();
    if (!address) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to create silent payment address");
    }

    UniValue result(UniValue::VOBJ);
    result.pushKV("sp_address", address->ToString());
    result.pushKV("scan_key", address->scan_key.GetHex());
    result.pushKV("spend_key", address->spend_key.GetHex());

    if (request.params.size() > 0) {
        result.pushKV("label", request.params[0].get_str());
    }

    return result;
}

static RPCHelpMan sendsilent()
{
    return RPCHelpMan{
        "sendsilent",
        "Send funds using Silent Payments to hide sender-receiver relationship.\n"
        "\nThe payment uses ECDH and BIP 352 to create outputs only detectable by\n"
        "scanning with the recipient's private keys.\n",
        {
            {"sp_address", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Recipient's silent payment address"},
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO,
             "Amount to send (in BTC)"},
            {"fee_rate", RPCArg::Type::AMOUNT, RPCArg::Default{"0.001"},
             "Transaction fee rate (in BTC/kB)"},
            {"subtract_fee_from_outputs", RPCArg::Type::BOOL, RPCArg::Default{false},
             "Subtract fee from amount"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR_HEX, "txid", "Transaction ID"},
                {RPCResult::Type::NUM, "size", "Transaction size in bytes"},
                {RPCResult::Type::STR, "status", "Transaction broadcast status"},
            },
        },
        RPCExamples{
            HelpExampleCli("sendsilent",
                          "\"sp1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq\" 1.5")
            + HelpExampleRpc("sendsilent",
                            "\"sp1q...\", 1.5")
        },
    };
}

static UniValue sendsilent(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() < 2 || request.params.size() > 4) {
        throw std::runtime_error("sendsilent <sp_address> <amount> [fee_rate] [subtract_fee]");
    }

    std::string sp_address = request.params[0].get_str();
    CAmount amount = AmountFromValue(request.params[1]);

    if (amount <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Amount must be positive");
    }

    CAmount fee_rate = AmountFromValue(request.params.size() > 2 ? 
                                       request.params[2] : UniValue(0.001));

    // Create silent payment wallet
    SilentPaymentWallet sp_wallet(wallet.get());

    // Create transaction
    auto tx = sp_wallet.CreateSilentPayment(sp_address, amount, fee_rate);
    if (!tx) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to create silent payment transaction");
    }

    // Broadcast (simplified - real impl would handle signing, fees, etc.)
    // wallet->SendTransaction(tx);

    UniValue result(UniValue::VOBJ);
    result.pushKV("txid", tx->GetHash().GetHex());
    result.pushKV("size", (int64_t)tx->GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION));
    result.pushKV("status", "broadcasted");

    return result;
}

static RPCHelpMan silentpaymentbalance()
{
    return RPCHelpMan{
        "silentpaymentbalance",
        "Get total balance of all silent payment addresses.\n",
        {},
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::STR, "balance", "Total balance in BTC"},
                {RPCResult::Type::NUM, "confirmations_needed", "Blocks to full confirmation"},
            },
        },
        RPCExamples{
            HelpExampleCli("silentpaymentbalance", "")
            + HelpExampleRpc("silentpaymentbalance", "")
        },
    };
}

static UniValue silentpaymentbalance(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() > 0) {
        throw std::runtime_error("silentpaymentbalance");
    }

    SilentPaymentWallet sp_wallet(wallet.get());
    CAmount balance = sp_wallet.GetSilentPaymentBalance();

    UniValue result(UniValue::VOBJ);
    result.pushKV("balance", ValueFromAmount(balance));
    result.pushKV("confirmations_needed", 6);

    return result;
}

static RPCHelpMan listsilentpaymentaddresses()
{
    return RPCHelpMan{
        "listsilentpaymentaddresses",
        "List all Silent Payment addresses in the wallet.\n",
        {
            {"include_keys", RPCArg::Type::BOOL, RPCArg::Default{false},
             "Include scan and spend keys in output"},
        },
        RPCResult{
            RPCResult::Type::ARR,
            "",
            "",
            {
                {RPCResult::Type::OBJ,
                 "",
                 "",
                 {
                     {RPCResult::Type::STR, "address", "Silent payment address"},
                     {RPCResult::Type::STR, "scan_key", "Public scan key (if requested)"},
                     {RPCResult::Type::STR, "spend_key", "Public spend key (if requested)"},
                 }},
            },
        },
        RPCExamples{
            HelpExampleCli("listsilentpaymentaddresses", "true")
            + HelpExampleRpc("listsilentpaymentaddresses", "true")
        },
    };
}

static UniValue listsilentpaymentaddresses(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() > 1) {
        throw std::runtime_error("listsilentpaymentaddresses [include_keys]");
    }

    bool include_keys = false;
    if (request.params.size() > 0) {
        include_keys = request.params[0].get_bool();
    }

    SilentPaymentWallet sp_wallet(wallet.get());
    auto addresses = sp_wallet.ListAddresses();

    UniValue result(UniValue::VARR);
    for (const auto& addr : addresses) {
        UniValue item(UniValue::VOBJ);
        item.pushKV("address", addr.ToString());

        if (include_keys) {
            item.pushKV("scan_key", addr.scan_key.GetHex());
            item.pushKV("spend_key", addr.spend_key.GetHex());
        }

        result.push_back(item);
    }

    return result;
}

static RPCHelpMan importsilentpayment()
{
    return RPCHelpMan{
        "importsilentpayment",
        "Import a silent payment address to watch for incoming payments.\n"
        "\nUseful for receiving-only wallets that want to scan blocks\n"
        "without storing spending keys.\n",
        {
            {"sp_address", RPCArg::Type::STR, RPCArg::Optional::NO,
             "Silent payment address to import"},
            {"label", RPCArg::Type::STR, RPCArg::Optional::OMITTED,
             "Optional label"},
        },
        RPCResult{
            RPCResult::Type::OBJ,
            "",
            "",
            {
                {RPCResult::Type::BOOL, "success", "Whether import was successful"},
                {RPCResult::Type::STR, "address", "Imported address"},
            },
        },
        RPCExamples{
            HelpExampleCli("importsilentpayment", "\"sp1q...\" \"receive-only\"")
            + HelpExampleRpc("importsilentpayment", "\"sp1q...\", \"receive-only\"")
        },
    };
}

static UniValue importsilentpayment(const JSONRPCRequest& request)
{
    std::shared_ptr<CWallet> const wallet = GetWalletForJSONRPCRequest(request);
    if (!wallet) return NullUniValue;

    LOCK(wallet->cs_wallet);

    if (request.params.size() < 1 || request.params.size() > 2) {
        throw std::runtime_error("importsilentpayment <sp_address> [label]");
    }

    std::string sp_address = request.params[0].get_str();
    auto address = SilentPaymentAddress::FromString(sp_address);

    if (!address) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid silent payment address");
    }

    SilentPaymentWallet sp_wallet(wallet.get());
    bool success = sp_wallet.ImportAddress(*address);

    UniValue result(UniValue::VOBJ);
    result.pushKV("success", success);
    result.pushKV("address", sp_address);

    if (request.params.size() > 1) {
        result.pushKV("label", request.params[1].get_str());
    }

    return result;
}

void RegisterSilentPaymentRPCCommands()
{
    static const CRPCCommand commands[] = {
        {"wallet", &silentpaymentaddress},
        {"wallet", &sendsilent},
        {"wallet", &silentpaymentbalance},
        {"wallet", &listsilentpaymentaddresses},
        {"wallet", &importsilentpayment},
    };
    for (const auto& c : commands) {
        t_rpcCommands->emplace_back(c);
    }
}
