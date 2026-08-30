// Copyright (c) 2023 Bitcoin Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "logprintf.h"
#include "nontrivial-threadlocal.h"

#include <clang-tidy/ClangTidyModule.h>
#include <clang-tidy/ClangTidyModuleRegistry.h>

class BitcoinModule final : public clang::tidy::ClangTidyModule
{
public:
    void addCheckFactories(clang::tidy::ClangTidyCheckFactories& CheckFactories) override
    {
        CheckFactories.registerCheck<bitcoinsilver::LogPrintfCheck>("bitcoinsilver-unterminated-logprintf");
        CheckFactories.registerCheck<bitcoinsilver::NonTrivialThreadLocal>("bitcoinsilver-nontrivial-threadlocal");
    }
};

static clang::tidy::ClangTidyModuleRegistry::Add<BitcoinModule>
    X("bitcoinsilver-module", "Adds bitcoinsilver checks.");

volatile int BitcoinModuleAnchorSource = 0;
