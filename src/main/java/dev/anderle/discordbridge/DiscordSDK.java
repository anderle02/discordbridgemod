package dev.anderle.discordbridge;

import net.fabricmc.api.EnvType;
import net.fabricmc.api.Environment;

@Environment(EnvType.CLIENT)
public final class DiscordSDK {
    static {
        System.loadLibrary("discord_partner_sdk");
        System.loadLibrary("discord_bridge");
    }

    public native boolean init(long applicationId);
}