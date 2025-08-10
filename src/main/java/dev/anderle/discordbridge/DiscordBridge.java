package dev.anderle.discordbridge;

import dev.anderle.discordbridge.commands.TestCommand;
import net.fabricmc.api.ModInitializer;

import net.fabricmc.fabric.api.client.command.v2.ClientCommandRegistrationCallback;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DiscordBridge implements ModInitializer {
	public static final String MOD_ID = "discordbridge";
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
	private static final long APP_ID = 1387765262815727707L;

	public static final DiscordSDK discordClient = new DiscordSDK(APP_ID);

	@Override
	public void onInitialize() {
		ClientCommandRegistrationCallback.EVENT.register((dispatcher, registryAccess) -> {
			new TestCommand(dispatcher);
		});

		LOGGER.info("DiscordBridge Initialized!");
		discordClient.startLoginFlow();
	}
}