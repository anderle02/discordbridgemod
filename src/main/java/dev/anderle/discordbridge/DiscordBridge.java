package dev.anderle.discordbridge;

import dev.anderle.discordbridge.commands.SetupCommand;
import net.fabricmc.api.ModInitializer;

import net.fabricmc.fabric.api.client.command.v2.ClientCommandRegistrationCallback;
import net.fabricmc.fabric.api.client.message.v1.ClientSendMessageEvents;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DiscordBridge implements ModInitializer {
	public static final String MOD_ID = "discordbridge";
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
	private static final long APP_ID = 1387765262815727707L;

	public static final DiscordSDK discordClient = new DiscordSDK(APP_ID);

	@Override
	public void onInitialize() {
		ClientCommandRegistrationCallback.EVENT.register((dispatcher, dedicated) -> new SetupCommand(dispatcher));

		ClientSendMessageEvents.CHAT.register(message -> {
			if (!message.startsWith("/")) { // Ignore commands
				System.out.println("You sent: " + message);
				discordClient.sendMessage(message);
			}
		});

		LOGGER.info("DiscordBridge Initialized!");
	}
}