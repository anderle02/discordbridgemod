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

    public void updateRichPresence() {
        updateRichPresence("details", "state", new NativeCallback() {
            public void onSuccess(String data) {
                DiscordBridge.LOGGER.info("Successfully updated Discord RPC.");
            }
            public void onError(String message) {
                DiscordBridge.LOGGER.error("Failed to set Discord RPC: {}", message);
            }
        });
    }

    /** Opens discord for authorization or refreshes the access token if needed. Then logs into Discord. */
    public void startLoginFlow() {
        if(Config.instance().getString("accessToken").isEmpty()) {
            authorize(new NativeCallback() {
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
            refreshToken(Config.instance().getString("refreshToken"), new NativeCallback() {
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
        login(Config.instance().getString("accessToken"), new NativeCallback() {
            public void onSuccess(String data) {
                DiscordBridge.LOGGER.info("Successfully logged in with Discord.");
                //updateRichPresence();
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
    private native boolean authorize(NativeCallback callback);
    private native boolean refreshToken(String refreshToken, NativeCallback callback);
    private native boolean login(String accessToken, NativeCallback callback);
    private native boolean updateRichPresence(String details, String state, NativeCallback callback);
    public native boolean getUserGuilds(NativeCallback callback);
    public native boolean getGuildChannels(long guildId, NativeCallback callback);

    @SuppressWarnings("unused")
    public interface NativeCallback {
        void onSuccess(String data);
        void onError(String message);
    }
}