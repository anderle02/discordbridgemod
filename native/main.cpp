#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <functional>
#include <csignal>
#include <jni.h>

jlong APPLICATION_ID = 0;
std::string STATUS;

static std::shared_ptr<discordpp::Client> client;

std::string jstringToStdString(JNIEnv* env, jstring jStr) {
    if (!jStr) return "";

    const char* chars = env->GetStringUTFChars(jStr, nullptr);
    std::string result(chars);
    env->ReleaseStringUTFChars(jStr, chars);

    return result;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_sendMessage(JNIEnv *env, jobject obj, jstring message)
{
  uint64_t recipientId = 947173506045591593; // The recipient's Discord ID

  client->SendUserMessage(recipientId, jstringToStdString(env, message), [](auto result, uint64_t messageId)
                          {
  if (result.Successful()) {
    STATUS = "✅ Message sent successfully";
  } else {
    STATUS = "❌ Failed to send message: " + result.Error();
  } });

  return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_init(JNIEnv *env, jobject obj, jlong applicationId)
{
  APPLICATION_ID = applicationId;

  STATUS = "🚀 Initializing Discord SDK with AppID " + std::to_string(APPLICATION_ID) + "...";

  // Create Discord client (no event loop here)
  client = std::make_shared<discordpp::Client>();

  client->AddLogCallback([](auto message, auto severity)
                         { std::cout << "[" << EnumToString(severity) << "] " << message << std::endl; }, discordpp::LoggingSeverity::Info);

  client->SetStatusChangedCallback([](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail)
                                   {
  STATUS = "🔄 Status changed: " + discordpp::Client::StatusToString(status);

  if (status == discordpp::Client::Status::Ready) {
    STATUS = "✅ Client is ready! You can now call SDK functions.";
  } else if (error != discordpp::Client::Error::None) {
    STATUS = "❌ Connection Error: " + discordpp::Client::ErrorToString(error) + " - Details: " + std::to_string(errorDetail);
  } });

  return JNI_TRUE;
}

extern "C" JNIEXPORT jstring JNICALL Java_dev_anderle_discordbridge_DiscordSDK_runCallbacks(JNIEnv *env, jobject obj)
{
  discordpp::RunCallbacks();
  return env->NewStringUTF(STATUS.c_str());
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_authorize(JNIEnv *env, jobject obj)
{
  // Generate OAuth2 code verifier for authentication
  auto codeVerifier = client->CreateAuthorizationCodeVerifier();

  // Set up authentication arguments
  discordpp::AuthorizationArgs args{};
  args.SetClientId(APPLICATION_ID);
  args.SetScopes(discordpp::Client::GetDefaultCommunicationScopes());
  args.SetCodeChallenge(codeVerifier.Challenge());

  // Begin authentication process
  client->Authorize(args, [codeVerifier](auto result, auto code, auto redirectUri)
                    {
  if (!result.Successful()) {
    STATUS = "❌ Authentication Error: " + result.Error();
    return;
  } else {
    STATUS = "✅ Authorization successful! Getting access token...";

    // Exchange auth code for access token
    client->GetToken(APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
      [](discordpp::ClientResult result,
      std::string accessToken,
      std::string refreshToken,
      discordpp::AuthorizationTokenType tokenType,
      int32_t expiresIn,
      std::string scope) {
        STATUS = "🔓 Access token received! Establishing connection...";
        
        client->UpdateToken(discordpp::AuthorizationTokenType::Bearer,  accessToken, [](discordpp::ClientResult result) {
        if(result.Successful()) {
          STATUS = "🔑 Token updated, connecting to Discord...";
          client->Connect();
        }
      });
    });
  } });

  return JNI_TRUE;
}