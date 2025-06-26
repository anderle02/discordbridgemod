package dev.anderle.discordbridge.commands;

import com.mojang.brigadier.CommandDispatcher;
import dev.anderle.discordbridge.DiscordBridge;
import net.fabricmc.fabric.api.client.command.v2.ClientCommandManager;
import net.fabricmc.fabric.api.client.command.v2.FabricClientCommandSource;
import net.minecraft.network.chat.Component;

public class SetupCommand {
    public SetupCommand(CommandDispatcher<FabricClientCommandSource> dispatcher) {
        dispatcher.register(ClientCommandManager.literal("discordbridge").executes(ctx -> {
            ctx.getSource().sendFeedback(Component.literal("Please visit your Browser or Discord App and authorize yourself, to use the mod."));
            DiscordBridge.discordClient.authorize();
            return 1;
        }));
    }
}
