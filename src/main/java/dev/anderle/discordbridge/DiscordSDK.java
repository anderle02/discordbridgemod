package dev.anderle.discordbridge;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;
import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import net.minecraft.network.chat.Component;

@Environment(EnvType.CLIENT)
public final class DiscordSDK {
    private int tickCounter;
    private String status = "";

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
                String oldStatus = status;
                status = runCallbacks();
                if(!oldStatus.equals(status)) {
                    client.gui.getChat().addMessage(Component.literal(status));
                }
            }
        });
    }

    public native boolean init(long applicationId);
    public native String runCallbacks();
    public native boolean authorize();
    public native boolean sendMessage(String message);
}