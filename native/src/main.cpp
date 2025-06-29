#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <chrono>
#include <iostream>
#include <jni.h>
#include <string>

static std::shared_ptr<discordpp::Client> client; // Discord Client.
static jlong APPLICATION_ID = 0;				  // Application ID.
static jboolean READY = false;					  // Client State.

/* Convert jstring to std::string. */
static std::string jstringToStdString(JNIEnv *env, jstring jStr);
/* Call the callback and then delete its reference. */
static void runJNICallback(jobject gCallback, bool success, const std::string &msg);

/* Save the jvm as global reference on load. */
static JavaVM *g_jvm = nullptr;
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
	g_jvm = vm;
	return JNI_VERSION_1_8;
}

/* Initialize the social SDK and register event listeners. */
extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_init(JNIEnv *env, jobject obj, jlong applicationId) {
	APPLICATION_ID = applicationId;

	client = std::make_shared<discordpp::Client>();

	client->AddLogCallback([](auto message, auto severity) { std::cout << "[" << EnumToString(severity) << "] " << message << std::endl; }, discordpp::LoggingSeverity::Info);

	client->SetStatusChangedCallback([](discordpp::Client::Status status, discordpp::Client::Error error, int32_t errorDetail) { READY = status == discordpp::Client::Status::Ready; });

	return JNI_TRUE;
}

/* Make discord run all its tasks. */
extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_runCallbacks(JNIEnv *env, jobject obj) {
	discordpp::RunCallbacks();
	// TODO: Report status changes here
	return JNI_TRUE;
}

/* Discord authorization flow. */
extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_authorize(JNIEnv *env, jobject, jobject callback) {

	jobject gCallback = env->NewGlobalRef(callback);

	auto codeVerifier = client->CreateAuthorizationCodeVerifier();

	discordpp::AuthorizationArgs args{};
	args.SetClientId(APPLICATION_ID);
	args.SetScopes(discordpp::Client::GetDefaultCommunicationScopes());
	args.SetCodeChallenge(codeVerifier.Challenge());

	client->Authorize(args, [gCallback, codeVerifier](auto res1, auto code, auto redirectUri) {
		if (!res1.Successful()) return runJNICallback(gCallback, false, res1.Error());

		client->GetToken(
			APPLICATION_ID, code, codeVerifier.Verifier(), redirectUri,
			[gCallback](discordpp::ClientResult res2, std::string accessToken, std::string refreshToken, discordpp::AuthorizationTokenType tokenType, int32_t expiresIn, std::string scope) {
				if (!res2.Successful()) return runJNICallback(gCallback, false, res2.Error());

				auto expiresAt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + expiresIn;
				std::string credentials = "{\"accessToken\":\"" + accessToken + "\",\"refreshToken\":\"" + refreshToken + "\",\"expiresAt\":" + std::to_string(expiresAt) + "}";

				runJNICallback(gCallback, true, credentials);
			});
	});
	return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_refreshToken(JNIEnv *env, jobject, jstring refreshToken, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);

	client->RefreshToken(
		APPLICATION_ID, jstringToStdString(env, refreshToken),
		[gCallback](discordpp::ClientResult res1, std::string accessToken, std::string refreshToken, discordpp::AuthorizationTokenType tokenType, int32_t expiresIn, std::string scope) {
			if (!res1.Successful()) return runJNICallback(gCallback, false, res1.Error());

			auto expiresAt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + expiresIn;
			std::string credentials = "{\"accessToken\":\"" + accessToken + "\",\"refreshToken\":\"" + refreshToken + "\",\"expiresAt\":" + std::to_string(expiresAt) + "}";

			runJNICallback(gCallback, true, credentials);
		});

	return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_login(JNIEnv *env, jobject, jstring accessToken, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);

	client->UpdateToken(discordpp::AuthorizationTokenType::Bearer, jstringToStdString(env, accessToken), [gCallback](discordpp::ClientResult result) {
		if (!result.Successful()) return runJNICallback(gCallback, false, result.Error());

		runJNICallback(gCallback, true, "");
		client->Connect();
	});

	return JNI_TRUE;
}

static void runJNICallback(jobject gCallback, bool success, const std::string &msg) {
	JNIEnv *env = nullptr;
	bool detach = g_jvm->GetEnv((void **)&env, JNI_VERSION_1_8) != JNI_OK;
	if (detach) g_jvm->AttachCurrentThread((void **)&env, nullptr);

	jclass cls = env->GetObjectClass(gCallback);
	jmethodID mid = env->GetMethodID(cls, success ? "onSuccess" : "onError", "(Ljava/lang/String;)V");
	jstring jMsg = env->NewStringUTF(msg.c_str());

	env->CallVoidMethod(gCallback, mid, jMsg);
	env->DeleteLocalRef(jMsg);
	env->DeleteGlobalRef(gCallback);

	if (detach) g_jvm->DetachCurrentThread();
}

static std::string jstringToStdString(JNIEnv *env, jstring jStr) {
	if (!jStr) return "";

	const char *chars = env->GetStringUTFChars(jStr, nullptr);
	std::string result(chars);
	env->ReleaseStringUTFChars(jStr, chars);

	return result;
}