#define DISCORDPP_IMPLEMENTATION
#include "discordpp.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <jni.h>
#include <string>
#include <thread>

static std::shared_ptr<discordpp::Client> client; // Discord Client.
static jlong APPLICATION_ID = 0;				  // Application ID.
static jboolean READY = false;					  // Client State.

/* Convert jstring to std::string. */
static std::string jstringToStdString(JNIEnv *env, jstring jStr);
/* Call the callback and then delete its reference. */
static void runJNICallback(jobject gCallback, bool success, const std::string &msg);
static std::string removeSpecialChars(const std::string &input);

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

	std::string localappdata = std::getenv("LOCALAPPDATA");
	client->RegisterLaunchCommand(APPLICATION_ID, "\"" + localappdata + "\\Programs\\PrismLauncher\\prismlauncher.exe\" --launch \"Skyblock Foraging\" --server \"mc.hypixel.net\"");

	return JNI_TRUE;
}

/* Make discord run all its tasks. */
extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_runCallbacks(JNIEnv *env, jobject obj) {
	discordpp::RunCallbacks();
	// TODO: Report status changes here
	return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_createOrJoinLobby(JNIEnv *env, jobject, jstring secret, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);
	client->CreateOrJoinLobby(jstringToStdString(env, secret), [gCallback](discordpp::ClientResult result, uint64_t lobbyId) {
		if (!result.Successful()) return runJNICallback(gCallback, false, result.Error());

		runJNICallback(gCallback, true, std::to_string(lobbyId));
	});
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_getUserGuilds(JNIEnv *env, jobject, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);

	client->GetUserGuilds([gCallback](discordpp::ClientResult result, std::vector<discordpp::GuildMinimal> guilds) {
		if (!result.Successful()) return runJNICallback(gCallback, false, result.Error());

		std::string data = "[";
		for (const discordpp::GuildMinimal guild : guilds) {
			data.append("{\"id\":" + std::to_string(guild.Id()) + ",\"name\":\"" + guild.Name() + "\"},");
		}
		data.pop_back();
		data.append("]");

		runJNICallback(gCallback, true, data);
	});

	return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_getGuildChannels(JNIEnv *env, jobject, jlong guildId, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);

	client->GetGuildChannels(guildId, [gCallback](discordpp::ClientResult result, std::vector<discordpp::GuildChannel> guildChannels) {
		if (!result.Successful()) return runJNICallback(gCallback, false, result.Error());

		std::string data = "[";
		for (const discordpp::GuildChannel channel : guildChannels) {
			data.append("{\"id\":" + std::to_string(channel.Id()) + ",\"name\":\"" + channel.Name() + "\",\"isLinkable\":" + std::to_string(channel.IsLinkable()) + "},");
		}
		data.pop_back();
		data.append("]");

		runJNICallback(gCallback, true, data);
	});

	return JNI_TRUE;
}

/* Update the user's rich presence. */
extern "C" JNIEXPORT jboolean JNICALL Java_dev_anderle_discordbridge_DiscordSDK_updateRichPresence(JNIEnv *env, jobject, jstring details, jstring state, jobject callback) {
	jobject gCallback = env->NewGlobalRef(callback);

	discordpp::Activity activity;
	activity.SetType(discordpp::ActivityTypes::Playing);
	activity.SetDetails(jstringToStdString(env, details));
	activity.SetState(jstringToStdString(env, state));

	// Set the party information
	// Create discordpp::ActivityParty
	discordpp::ActivityParty party;
	party.SetId("party1234");
	// current party size
	party.SetCurrentSize(1);
	// max party size
	party.SetMaxSize(5);
	activity.SetParty(party);

	// Create ActivitySecrets
	discordpp::ActivitySecrets secrets;
	secrets.SetJoin("joinsecret1234");
	activity.SetSecrets(secrets);

	// Set supported platforms that can join the game
	// See discordpp::ActivityGamePlatforms for available platforms
	activity.SetSupportedPlatforms(discordpp::ActivityGamePlatforms::Desktop);

	client->UpdateRichPresence(activity, [gCallback](discordpp::ClientResult result) {
		if (result.Successful())
			runJNICallback(gCallback, true, "");
		else
			runJNICallback(gCallback, false, result.Error());
	});

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

		client->Connect();

		auto start = std::chrono::steady_clock::now();

		std::thread([gCallback, start]() {
			while (!READY) { // Wait until the client is ready.
				if (std::chrono::steady_clock::now() - start > std::chrono::seconds(5)) {
					return runJNICallback(gCallback, false, "Timeout waiting for client ready. Check https://discordstatus.com/ for issues.");
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
			runJNICallback(gCallback, true, "");
		}).detach();
	});

	return JNI_TRUE;
}

static void runJNICallback(jobject gCallback, bool success, const std::string &msg) {
	JNIEnv *env = nullptr;
	bool detach = g_jvm->GetEnv((void **)&env, JNI_VERSION_1_8) != JNI_OK;
	if (detach) g_jvm->AttachCurrentThread((void **)&env, nullptr);

	jclass cls = env->GetObjectClass(gCallback);
	jmethodID mid = env->GetMethodID(cls, success ? "onSuccess" : "onError", "(Ljava/lang/String;)V");
	jstring jMsg = env->NewStringUTF(removeSpecialChars(msg).c_str());

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

static std::string removeSpecialChars(const std::string &input) {
	std::string output;
	for (unsigned char c : input) {
		// Keep only printable ASCII (space 0x20 to ~ 0x7E)
		if (c >= 0x20 && c <= 0x7E) {
			output += c;
		}
		// else skip the character
	}
	return output;
}