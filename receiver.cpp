#include <arpa/inet.h>
#include <cstdint>
#include <exception>
#include <netinet/in.h>
#include <unistd.h>

#include <cctype>
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
    cout << "            DES RECEIVER\n";
    cout << "=====================================\n\n";

    int port;
    cout << "Enter port to listen on: ";
    cin >> port;
    cin.ignore();

    string key;
    while (true)
    {
        cout << "Enter shared DES key: ";
        getline(cin, key);
        if (key.size() == 8)
            break;
        cout << "  -> Key must be exactly 8 characters (64 bits). Try again.\n";
    }

    vector<uint8_t> keyBytes(key.begin(), key.end());
    des::Bits keyBits = des::bytesToBits(keyBytes);

    cout << "\n[1] Deriving 16 round keys (K1..K16) from the shared key...\n";
    des::KeySchedule ks = des::generateRoundKeys(keyBits, /*verbose=*/true);

    try
    {
        cout << "\n[2] Listening on port " << port << " ...\n";
        int serverSock = net::createServerSocket(port);
        string clientIP;
        int clientSock = net::acceptClient(serverSock, clientIP);
        cout << "    Connection established from " << clientIP << "\n";

        uint32_t nBlocksNet;
        net::recvAll(clientSock, &nBlocksNet, sizeof(nBlocksNet));
        int numBlocks = static_cast<int>(ntohl(nBlocksNet));

        vector<uint8_t> iv(des_util::BLOCK_SIZE);
        net::recvAll(clientSock, iv.data(), iv.size());

        vector<uint8_t> ciphertext(numBlocks * des_util::BLOCK_SIZE);
        net::recvAll(clientSock, ciphertext.data(), ciphertext.size());

        cout << "\n[3] Received IV: ";
        printHexBytes(iv);
        cout << "\n";
        cout << "    Received ciphertext (" << ciphertext.size() << " bytes): ";
        for (uint8_t b : ciphertext)
            printf("%02X", b);
        cout << "\n";

        // cbc decryption
        cout << "\n[4] Decrypting (DES-CBC), round-by-round trace:\n";
        vector<uint8_t> plainPadded;
        vector<uint8_t> prevBlock = iv;

        for (int i = 0; i < numBlocks; i++)
        {
            vector<uint8_t> cblock(ciphertext.begin() + i * 8, ciphertext.begin() + i * 8 + 8);

            cout << "\n  --- Block " << (i + 1) << "/" << numBlocks << " ---\n";
            cout << "    Cipher block         : ";
            printHexBytes(cblock);
            cout << "\n";

            des::Bits cipherBits = des::bytesToBits(cblock);
            des::Bits plainBits =
                des::decryptBlock(cipherBits, ks, /*verbose=*/true, "[Block " + to_string(i + 1) + "]");
            vector<uint8_t> desOutput = des::bitsToBytes(plainBits);
            vector<uint8_t> plainBlock = des_util::xorBytes(desOutput, prevBlock);

            cout << "    DES output           : ";
            printHexBytes(desOutput);
            cout << "\n";
            cout << "    XOR with prev cipher : ";
            printHexBytes(prevBlock);
            cout << "\n";
            cout << "    -> Plaintext block   : ";
            printHexBytes(plainBlock);
            cout << " (\"";
            for (uint8_t b : plainBlock)
                cout << (isprint(b) ? static_cast<char>(b) : '.');
            cout << "\")\n";

            plainPadded.insert(plainPadded.end(), plainBlock.begin(), plainBlock.end());
            prevBlock = cblock;
        }

        vector<uint8_t> plain = des_util::pkcs7Unpad(plainPadded);
        string message(plain.begin(), plain.end());

        cout << "\n=====================================\n";
        cout << "    Decrypted message: \"" << message << "\"\n";
        cout << "=====================================\n";

        close(clientSock);
        close(serverSock);
    }
    catch (const exception &e)
    {
        cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    return 0;
}