#include <arpa/inet.h>
#include <exception>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "crypto_utils.hpp"
#include "des.hpp"
#include "network.hpp"

using namespace std;

static void printHexBytes(const vector<uint8_t> &bytes)
{
    for (uint8_t b : bytes)
        printf("%02X ", b);
}

int main()
{
    cout << "=====================================\n";
    cout << "            DES SENDER\n";
    cout << "=====================================\n\n";

    // ip and port
    string ip;
    cout << "Enter receiver's IP address: ";
    getline(cin, ip);

    int port;
    cout << "Enter receiver's port: ";
    cin >> port;
    cin.ignore();

    // key
    string key;
    while (true)
    {
        cout << "Enter shared DES key: ";
        getline(cin, key);
        if (key.size() == 8)
            break;
        cout << "  -> Key must be exactly 8 characters. Try again.\n";
    }

    // message
    string message;
    cout << "Enter plaintext message to send: ";
    getline(cin, message);

    // key schedule
    vector<uint8_t> keyBytes(key.begin(), key.end());
    des::Bits keyBits = des::bytesToBits(keyBytes);

    cout << "\n[1] Deriving 16 round keys (K1..K16) from the shared key...\n";
    des::KeySchedule ks = des::generateRoundKeys(keyBits, /*verbose=*/true);

    // padding
    vector<uint8_t> plainBytes(message.begin(), message.end());
    vector<uint8_t> padded = des_util::pkcs7Pad(plainBytes);
    int numBlocks = static_cast<int>(padded.size() / des_util::BLOCK_SIZE);

    cout << "\n[2] Plaintext: \"" << message << "\" (" << plainBytes.size() << " bytes)\n";
    cout << "    PKCS#7 padded to " << padded.size() << " bytes -> " << numBlocks << " block(s) of 8 bytes\n";

    vector<uint8_t> iv = des_util::generateRandomIV();
    cout << "    Random IV generated: ";
    printHexBytes(iv);
    cout << "\n";

    // cbc encryption
    cout << "\n[3] Encrypting (DES-CBC), round-by-round trace:\n";
    vector<uint8_t> ciphertext;
    vector<uint8_t> prevBlock = iv;

    for (int i = 0; i < numBlocks; i++)
    {
        vector<uint8_t> block(padded.begin() + i * 8, padded.begin() + i * 8 + 8);
        vector<uint8_t> xoredIn = des_util::xorBytes(block, prevBlock);

        cout << "\n  --- Block " << (i + 1) << "/" << numBlocks << " ---\n";
        cout << "    Plaintext block      : ";
        printHexBytes(block);
        cout << "\n";
        cout << "    XOR with prev cipher : ";
        printHexBytes(prevBlock);
        cout << "\n";
        cout << "    -> Input to DES      : ";
        printHexBytes(xoredIn);
        cout << "\n";

        des::Bits inputBits = des::bytesToBits(xoredIn);
        des::Bits cipherBits = des::encryptBlock(inputBits, ks, /*verbose=*/true, "[Block " + to_string(i + 1) + "]");
        vector<uint8_t> cipherBlock = des::bitsToBytes(cipherBits);

        cout << "    Cipher block         : ";
        printHexBytes(cipherBlock);
        cout << "\n";

        ciphertext.insert(ciphertext.end(), cipherBlock.begin(), cipherBlock.end());
        prevBlock = cipherBlock;
    }

    cout << "\n[4] Final ciphertext (" << ciphertext.size() << " bytes): ";
    for (uint8_t b : ciphertext)
        printf("%02X", b);
    cout << "\n";

    // send over tcp
    try
    {
        cout << "\n[5] Connecting to " << ip << ":" << port << " ...\n";
        int sock = net::connectToServer(ip, port);

        uint32_t nBlocksNet = htonl(static_cast<uint32_t>(numBlocks));
        net::sendAll(sock, &nBlocksNet, sizeof(nBlocksNet));
        net::sendAll(sock, iv.data(), iv.size());
        net::sendAll(sock, ciphertext.data(), ciphertext.size());

        cout << "    Sent " << numBlocks << " block(s) + IV to receiver.\n";
        close(sock);
    }
    catch (const exception &e)
    {
        cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    cout << "\n=====================================\n";
    cout << "               DONE\n";
    cout << "=====================================\n";
    return 0;
}