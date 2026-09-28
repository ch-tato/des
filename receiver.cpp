#include <arpa/inet.h>
#include <cstdint>
#include <unistd.h>

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

static void printHexBytes(const vector<uint8_t> &bytes) {
  for (uint8_t b : bytes)
    printf("%02X ", b);
}

int main() {
  cout << "=====================================\n";
  cout << "            DES RECEIVER\n";
  cout << "=====================================\n\n";
}