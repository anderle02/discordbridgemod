package dev.anderle.discordbridge;

import com.google.gson.*;
import net.fabricmc.loader.api.FabricLoader;

import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.List;

public class Config {
    public static final Path CONFIG_PATH = FabricLoader.getInstance().getConfigDir().resolve("discordbridge.json");

    private static Config INSTANCE;
    private JsonObject currentSettings;

    /** Get the Config instance. */
    public static Config instance() {
        if(INSTANCE == null) INSTANCE = new Config();
        return INSTANCE;
    }
    /** Get a setting as string. */
    public String getString(String setting) {
        return currentSettings.get(setting).getAsString();
    }
    /** Change a setting to the new value. */
    public void set(String setting, String value) {
        currentSettings.addProperty(setting, value);
        save();
    }

    private Config() {
        List<Setting> settings = List.of(
                new Setting("accessToken", ""),
                new Setting("refreshToken", ""),
                new Setting("expiresAt", "0")
        );

        if(Files.exists(CONFIG_PATH)) {
            try (Reader reader = Files.newBufferedReader(CONFIG_PATH, StandardCharsets.UTF_8)) {
                currentSettings = JsonParser.parseReader(reader).getAsJsonObject();
            } catch (IOException | JsonParseException e) {
                DiscordBridge.LOGGER.error("Error reading config file!", e);
                currentSettings = new JsonObject();
            }
        } else {
            currentSettings = new JsonObject();
        }

        for(Setting setting : settings) {
            if(!currentSettings.has(setting.name)) {
                currentSettings.add(setting.name, new JsonPrimitive(setting.defaultValue));
            }
        }

        save();
    }

    private void save() {
        Gson gson = new GsonBuilder().setPrettyPrinting().create();

        try (Writer out = Files.newBufferedWriter(CONFIG_PATH, StandardCharsets.UTF_8, StandardOpenOption.CREATE, StandardOpenOption.TRUNCATE_EXISTING)) {
            gson.toJson(currentSettings, out);
        } catch (IOException e) {
            DiscordBridge.LOGGER.error("Could not save settings to file!", e);
        }
    }

    private record Setting(String name, String defaultValue) {}
}
