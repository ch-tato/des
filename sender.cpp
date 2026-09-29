#include <arpa/inet.h>
#include <exception>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "des.hpp"
#include "message_protocol.hpp"
#include "network.hpp"

using namespace std;

int main()
{
    cout << "=====================================\n";
    cout << "            DES CLIENT\n";
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

    // key schedule
    vector<uint8_t> keyBytes(key.begin(), key.end());
    des::Bits keyBits = des::bytesToBits(keyBytes);

    cout << "\n[1] Deriving 16 round keys (K1..K16) from the shared key...\n";
    des::KeySchedule ks = des::generateRoundKeys(keyBits, /*verbose=*/true);

    // message over tcp
    int sock;
    try
    {
        cout << "\n[Setup] Connecting to " << ip << ":" << port << " ...\n";
        sock = net::connectToServer(ip, port);
        cout << "[Setup] Connected. Type messages below - 'exit' or 'quit' ends "
                "the session for both sides.\n";
    }
    catch (const exception &e)
    {
        cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }

    int turnNumber = 0;
    while (true)
    {
        cout << "\n---------------------------------------\n";
        cout << "Enter message (or 'exit'/'quit' to end): ";
        string message;
        getline(cin, message);

        if (chat::isQuitCommand(message))
        {
            chat::sendQuitSignal(sock);
            cout << "\n[Session] You ended the conversation.\n";
            break;
        }

        ++turnNumber;
        chat::sendEncryptedMessage(sock, ks, message, turnNumber);

        cout << "\n[Session] Waiting for receiver's reply...\n";
        try
        {
            chat::ReceivedMessage reply = chat::receiveEncryptedMessage(sock, ks, turnNumber);
            if (reply.isQuit)
            {
                cout << "\n[Session] Receiver ended the conversation.\n";
                break;
            }
            cout << "\n[Session] Receiver replied: \"" << reply.plaintext << "\"\n";
        }
        catch (const exception &e)
        {
            cout << "\n[Session] Connection closed unexpectedly (" << e.what() << ").\n";
            break;
        }
    }

    close(sock);
    cout << "\n=====================================\n";
    cout << "               DONE\n";
    cout << "=====================================\n";
    return 0;
}