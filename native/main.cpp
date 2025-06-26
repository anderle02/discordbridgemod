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
jlong APPLICATION_ID = 0;
std::atomic<bool> running = true;
static std::shared_ptr<discordpp::Client> client;

// Signal handler to stop the application
void signalHandler(int signum)
{
  running.store(false);
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_init(
    JNIEnv *env, jobject obj, jlong applicationId)
{
  APPLICATION_ID = applicationId;

  std::signal(SIGINT, signalHandler);
  std::cout << "🚀 Initializing Discord SDK with AppID " << APPLICATION_ID << "...\n";

  // Create Discord client (no event loop here)
  client = std::make_shared<discordpp::Client>();

  client->AddLogCallback([](auto message, auto severity)
                         { std::cout << "[" << EnumToString(severity) << "] " << message << std::endl; }, discordpp::LoggingSeverity::Info);

  client->SetStatusChangedCallback([](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail)
                                   {
  std::cout << "🔄 Status changed: " << discordpp::Client::StatusToString(status) << std::endl;

  if (status == discordpp::Client::Status::Ready) {
    std::cout << "✅ Client is ready! You can now call SDK functions.\n";
  } else if (error != discordpp::Client::Error::None) {
    std::cerr << "❌ Connection Error: " << discordpp::Client::ErrorToString(error) << " - Details: " << errorDetail << std::endl;
  } });

  return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_runCallbacks(
    JNIEnv *env, jobject obj)
{
  discordpp::RunCallbacks();

  return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_authorize(
    JNIEnv *env, jobject obj)
{
  // Generate OAuth2 code verifier for authentication
  auto codeVerifier = client->CreateAuthorizationCodeVerifier();

  // Set up authentication arguments
  discordpp::AuthorizationArgs args{};
  args.SetClientId(APPLICATION_ID);
  args.SetScopes(discordpp::Client::GetDefaultPresenceScopes());
  args.SetCodeChallenge(codeVerifier.Challenge());

  // Begin authentication process
  client->Authorize(args, [codeVerifier](auto result, auto code, auto redirectUri)
                    {
  if (!result.Successful()) {
    std::cerr << "❌ Authentication Error: " << result.Error() << std::endl;
    return;
  } else {
    std::cout << "✅ Authorization successful! Getting access token...\n";

    // Exchange auth code for access token
    client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
      [](discordpp::ClientResult result,
      std::string accessToken,
      std::string refreshToken,
      discordpp::AuthorizationTokenType tokenType,
      int32_t expiresIn,
      std::string scope) {
        std::cout << "🔓 Access token received! Establishing connection...\n";
        
        client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,  accessToken, [](discordpp::ClientResult result) {
        if(result.Successful()) {
          std::cout << "🔑 Token updated, connecting to Discord...\n";
          client->Connect();
        }
      });
    });
  } });

  return JNI_TRUE;
}