// Copyright (c) 2022-present The BitcoinSilver developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINSILVER_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H
#define BITCOINSILVER_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H

#include <util/fs.h>

class ArgsManager;

/** @return The fee estimates data file path. */
fs::path FeeestPath(const ArgsManager& argsman);

#endif // BITCOINSILVER_POLICY_FEES_BLOCK_POLICY_ESTIMATOR_ARGS_H
