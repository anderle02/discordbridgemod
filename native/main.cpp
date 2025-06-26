#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>
#include <jni.h>

// Create a flag to stop the application
std::atomic<bool> running = true;

// Signal handler to stop the application
void signalHandler(int signum) {
  running.store(false);
}

int main() {
  std::signal(SIGINT, signalHandler);
  std::cout << "🚀 Initializing Discord SDK...\n";

  // Create our Discord Client
  auto client = std::make_shared<discordpp::Client>();

  // Keep application running to allow SDK to receive events and callbacks
  while (running) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  return 0;
}

static std::shared_ptr<discordpp::Client> client;

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_init(
    JNIEnv* env, jobject obj, jlong applicationId) {

    // Create Discord client (no event loop here)
    client = std::make_shared<discordpp::Client>();

    // You could call client->run() or similar here if the SDK requires,
    // but in this minimal example, just create it and return.
    std::cout << "test";

    return JNI_TRUE;
}