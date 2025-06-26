package dev.anderle.discordbridge;

import net.fabricmc.api.ModInitializer;

import net.fabricmc.fabric.api.client.event.lifecycle.v1.ClientTickEvents;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DiscordBridge implements ModInitializer {
	public static final String MOD_ID = "discordbridge";
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
	private static final long APP_ID = 1387765262815727707L;

	private int tickCounter;

	@Override
	public void onInitialize() {
		LOGGER.info("Hello Fabric world!");

		DiscordSDK discordClient = new DiscordSDK();
		discordClient.init(APP_ID);

		ClientTickEvents.END_CLIENT_TICK.register(client -> {
			tickCounter++;
			if (tickCounter >= 10) {
				tickCounter = 0;
				LOGGER.info("Running Callbacks...");
				discordClient.runCallbacks();

			}
		});

		discordClient.authorize();
	}
}