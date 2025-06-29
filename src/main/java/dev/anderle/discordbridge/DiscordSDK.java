package dev.anderle.discordbridge;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;

import java.util.function.Consumer;

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

    public void authorizeDiscordAccount(Consumer<String> onSuccess, Consumer<String> onError) {
        authorize(new AuthCallback() {
            public void onSuccess(String data) { onSuccess.accept(data); }
            public void onError(String message) { onError.accept(message); }
        });
    }

    private native boolean init(long applicationId);
    private native boolean runCallbacks();
    private native boolean authorize(AuthCallback callback);

    @SuppressWarnings("unused")
    public interface AuthCallback {
        void onSuccess(String data);
        void onError(String message);
    }
}