# Bitcoin Silver (BTCS)

Bitcoin Silver is a fork of Bitcoin designed to provide a faster network and continuous development. This README provides an overview of the project, installation instructions, and details on how to contribute.

## Overview

Bitcoin Silver aims to enhance the original Bitcoin network by:
- **Faster Transactions**: Improving transaction speeds to ensure a more efficient and scalable network.
- **Continuous Development**: Regular updates and community-driven enhancements to keep the network secure and up-to-date.

## Features

- **Speed**: Reduced block generation time to increase transaction throughput.
- **Security**: Enhanced security protocols to protect against attacks.
- **Scalability**: Optimized to handle a larger number of transactions per second.
- **Community-Driven**: Open-source project with contributions from developers worldwide.

## Specification

<table>
<tr><td>Coin Name</td><td>Bitcoin Silver</td></tr>
<tr><td>Ticker</td><td>BTCS</td></tr>
<tr><td>Type</td><td>PoW</td></tr>
<tr><td>Maturity</td><td>200</td></tr>
<tr><td>Max supply</td><td>21 470 000</td></tr>
<tr><td>P2P port</td><td>10566</td></tr>
<tr><td>RPC port</td><td>10567</td></tr>
<tr><td>Prefix</td><td>"B" / "bs1"</td></tr>
<tr><td>Halving</td><td>210 000</td></tr>
<tr><td>Block time</td><td>300 seconds</td></tr>
<tr><td>Diff time</td><td>~96 blocks</td></tr>
<tr><td>Block reward</td><td>50</td></tr>
<tr><td>Initial Security Fund</td><td>470 000 (~2.5%)</td></tr>
<tr><td>Algo</td><td>SHA256</td></tr>
</table>

The initial allocation of 470,000 Bitcoin Silver (ISF) was distributed among core members, averaging 50,000 per person, and backed by USDT deposits used for listing fees and initial cloud services. This was not a pre-mine for personal profit but rather an investment in foundational contributors to incentivize development, ensure long-term commitment, and support the project's growth.

## Getting Started

### Prerequisites

Before you begin, ensure you have met the following requirements:
- **Operating System**: Linux, macOS, or Windows.
- **Dependencies**: Ensure you have `git`, `cmake`, and `gcc` or equivalent installed.

### Installation

1. **Clone the Repository**
```bash
   git clone https://github.com/bitcoin-silver/core.git
   cd core
```

2. **Build the Source Code**

**Linux:**
```bash
cmake -B build -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DENABLE_WALLET=ON -DENABLE_IPC=OFF
cmake --build build -j$(nproc)
```
> **Note:** `-DENABLE_IPC=OFF` is needed unless you have Cap'n Proto installed, since `ENABLE_IPC` defaults to `ON` and configure will otherwise fail with "Cap'n Proto is required".

> **Note:** On newer Linux distros (Ubuntu 22.04+, Ubuntu 25.10, etc.) BerkeleyDB 4.8 is no longer available. Install BerkeleyDB dev libraries and add `-DWITH_BDB=ON`:
> ```bash
> sudo apt install libdb-dev libdb++-dev
> cmake -B build -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF -DWITH_BDB=ON -DENABLE_IPC=OFF
> cmake --build build -j$(nproc)
> ```

To also build the GUI (`bitcoinsilver-qt`), add `-DBUILD_GUI=ON` (requires Qt6 development packages installed). Other optional CMake arguments: `-DENABLE_WALLET=OFF`.

**Windows (cross-compile):**
```bash
cd depends
make HOST=x86_64-w64-mingw32
cd ..
cmake -B build --toolchain depends/x86_64-w64-mingw32/toolchain.cmake -DBUILD_GUI=ON -DBUILD_TESTS=OFF -DBUILD_BENCH=OFF
cmake --build build -j$(nproc)
```

3. **Run the Node**
```bash
./build/bin/bitcoinsilverd
```

The GUI wallet, if built, is at `./build/bin/bitcoinsilver-qt`.

## Usage

### Wallet Setup

1.  **Generate a New Wallet**
```bash
./build/bin/bitcoinsilver-cli createwallet "mywallet"
```

-   **Get Wallet Address**
```bash
./build/bin/bitcoinsilver-cli getnewaddress
```
### Mining

Start mining with the following command:
```bash
./build/bin/bitcoinsilver-cli generatetoaddress n "mywalletaddress"
```
Replace `n` with the number of blocks you want to mine and `"mywalletaddress"` with your actual wallet address.

## Contributing

We welcome contributions from the community! To get started:

1.  Fork the repository on GitHub.
2.  Create a new branch (`git checkout -b feature/YourFeature`).
3.  Make your changes and commit them (`git commit -am 'Add new feature'`).
4.  Push to the branch (`git push origin feature/YourFeature`).
5.  Create a new Pull Request.

### Reporting Issues

If you encounter any issues, please report them on our [GitHub Issues](https://github.com/bitcoin-silver/core/issues) page.
