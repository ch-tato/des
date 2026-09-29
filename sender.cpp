#include <arpa/inet.h>
#include <cstdint>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "crypto_utils.hpp"
#include "des.hpp"

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

    // send over tcp
}