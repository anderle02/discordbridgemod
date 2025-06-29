package dev.anderle.discordbridge;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;

@Environment(EnvType.CLIENT)
public final class DiscordSDK {
    private int tickCounter;

    static {
        System.loadLibrary("discord_partner_sdk");
        System.loadLibrary("discord_bridge");
    }

    public DiscordSDK(long applicationId) {
        init(applicationId);

        ClientTickEvents.END_CLIENT_TICK.register(client -> {
            tickCounter++;
            if (tickCounter >= 10) {
                tickCounter = 0;
                runCallbacks();
            }
        });
    }

    /** Opens discord for authorization or refreshes the access token if needed. Then logs into Discord. */
    public void startLoginFlow() {
        if(Config.instance().getString("accessToken").isEmpty()) {
            authorize(new AuthCallback() {
                public void onSuccess(String data) {
                    storeCredentials(JsonParser.parseString(data).getAsJsonObject());
                    DiscordBridge.LOGGER.info("Successfully authorized with Discord. Logging in...");
                    login();
                }
                public void onError(String message) {
                    DiscordBridge.LOGGER.error("Failed to authorize Discord account: {}", message);
                }
            });
        } else if(isTokenExpired(Long.parseLong(Config.instance().getString("expiresAt")))) {
            refreshToken(Config.instance().getString("refreshToken"), new AuthCallback() {
                public void onSuccess(String data) {
                    storeCredentials(JsonParser.parseString(data).getAsJsonObject());
                    DiscordBridge.LOGGER.info("Successfully refreshed Discord access token. Logging in...");
                    login();
                }
                public void onError(String message) {
                    DiscordBridge.LOGGER.error("Failed to refresh Discord access token: {}", message);
                }
            });
        } else {
            login();
        }
    }

    private void login() {
        login(Config.instance().getString("accessToken"), new AuthCallback() {
            public void onSuccess(String data) {
                DiscordBridge.LOGGER.info("Successfully logged in with Discord.");
            }
            public void onError(String message) {
                DiscordBridge.LOGGER.error("Failed to log in with Discord: {}", message);
            }
        });
    }

    private boolean isTokenExpired(long expiresAt) {
        return System.currentTimeMillis() >= expiresAt * 1000;
    }

    private void storeCredentials(JsonObject credentials) {
        Config.instance().set("accessToken", credentials.get("accessToken").getAsString());
        Config.instance().set("refreshToken", credentials.get("refreshToken").getAsString());
        Config.instance().set("expiresAt", credentials.get("expiresAt").getAsString());
    }

    private native boolean init(long applicationId);
    private native boolean runCallbacks();
    private native boolean authorize(AuthCallback callback);
    private native boolean refreshToken(String refreshToken, AuthCallback callback);
    private native boolean login(String accessToken, AuthCallback callback);

    @SuppressWarnings("unused")
    public interface AuthCallback {
        void onSuccess(String data);
        void onError(String message);
    }
}