// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOINSILVER_COMMON_RUN_COMMAND_H
#define BITCOINSILVER_COMMON_RUN_COMMAND_H

#include <string>
#include <vector>

class UniValue;

/**
 * Execute a command which returns JSON, and parse the result.
 *
 * @param cmd_args The command and arguments
 * @param str_std_in string to pass to stdin
 * @return parsed JSON
 */
UniValue RunCommandParseJSON(const std::vector<std::string>& cmd_args, const std::string& str_std_in = "");

#endif // BITCOINSILVER_COMMON_RUN_COMMAND_H
