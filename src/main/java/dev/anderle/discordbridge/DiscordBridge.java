package dev.anderle.discordbridge;

import net.fabricmc.api.ModInitializer;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DiscordBridge implements ModInitializer {
	public static final String MOD_ID = "discordbridge";
	public static final Logger LOGGER = LoggerFactory.getLogger(MOD_ID);
	private static final long APP_ID = 1387765262815727707L;

	@Override
	public void onInitialize() {
		LOGGER.info("Hello Fabric world!");

		new DiscordSDK().init(APP_ID);
	}
}