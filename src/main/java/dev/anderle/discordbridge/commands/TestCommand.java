package dev.anderle.discordbridge.commands;

import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonParser;
import com.mojang.brigadier.CommandDispatcher;
import com.mojang.brigadier.StringReader;
import com.mojang.brigadier.arguments.ArgumentType;
import com.mojang.brigadier.arguments.StringArgumentType;
import com.mojang.brigadier.context.CommandContext;
import com.mojang.brigadier.exceptions.CommandSyntaxException;
import com.mojang.brigadier.suggestion.Suggestions;
import com.mojang.brigadier.suggestion.SuggestionsBuilder;
import dev.anderle.discordbridge.DiscordBridge;
import dev.anderle.discordbridge.DiscordSDK;
import net.fabricmc.fabric.api.client.command.v2.ClientCommandManager;
import net.fabricmc.fabric.api.client.command.v2.FabricClientCommandSource;
import net.minecraft.network.chat.Component;

import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public class TestCommand {
    /** Cache guilds and channel data to prevent Discord rate limits. 1 minute is probably a reasonable time. */
    public static final Cache<String, JsonArray> CACHE = CacheBuilder.newBuilder()
            .expireAfterWrite(1, TimeUnit.MINUTES)
            .maximumSize(200)
            .build();

    private static boolean isWaitingForResponse = false; // To avoid multiple requests in a row when cache needs to be refreshed and user quickly types.

    public TestCommand(CommandDispatcher<FabricClientCommandSource> dispatcher) {
        dispatcher.register(ClientCommandManager.literal("discord")
        .then(ClientCommandManager.argument("mode", StringArgumentType.word())
        .then(ClientCommandManager.argument("guild", new GuildArgument())
        .then(ClientCommandManager.argument("channel", new ChannelArgument())
        .executes((ctx) -> {
            String mode = ctx.getArgument("mode", String.class);
            String guild = ctx.getArgument("guild", String.class);
            String channel = ctx.getArgument("channel", String.class);

            ctx.getSource().sendFeedback(Component.literal("You chose: " + mode + " " + guild + " " + channel));
            return 1;
        })))));
    }

    public static class GuildArgument implements ArgumentType<String> {

        @Override
        public String parse(StringReader reader) throws CommandSyntaxException {
            return reader.readQuotedString();
        }

        @Override
        public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> context, SuggestionsBuilder builder) {
            String input = builder.getRemainingLowerCase().replaceAll("\"", "");
            JsonArray guilds = CACHE.getIfPresent("GUILDS");
            if (guilds != null) {
                guilds.asList().stream()
                        .map((guild) -> guild.getAsJsonObject().get("name").getAsString())
                        .filter((name) -> name.toLowerCase().startsWith(input))
                        .forEach((name) -> builder.suggest("\"" + name + "\""));
            } else if(!isWaitingForResponse) {
                refreshGuildsCache();
            }
            return builder.buildFuture();
        }
    }

    private static void refreshGuildsCache() {
        isWaitingForResponse = true;
        DiscordBridge.discordClient.getUserGuilds(new DiscordSDK.NativeCallback() {
            public void onSuccess(String data) {
                CACHE.put("GUILDS", JsonParser.parseString(data).getAsJsonArray());
                isWaitingForResponse = false;
            }
            public void onError(String message) {
                DiscordBridge.LOGGER.error("Failed to list user guilds: {}", message);
                isWaitingForResponse = false;
            }
        });
    }

    public static class ChannelArgument implements ArgumentType<String> {

        @Override
        public String parse(StringReader reader) throws CommandSyntaxException {
            return reader.readUnquotedString();
        }

        @Override
        public <S> CompletableFuture<Suggestions> listSuggestions(CommandContext<S> context, SuggestionsBuilder builder) {
            if(isWaitingForResponse) return builder.buildFuture();

            JsonArray guilds = CACHE.getIfPresent("GUILDS");
            if (guilds == null) {
                refreshGuildsCache();
                return builder.buildFuture();
            }

            String guild = context.getArgument("guild", String.class);
            long guildId = guilds.asList().stream()
                    .filter((g) -> g.getAsJsonObject().get("name").getAsString().equals(guild))
                    .map(g -> g.getAsJsonObject().get("id").getAsLong())
                    .findFirst().orElse(0L);

            if(guildId == 0L) return builder.buildFuture();

            JsonArray channels = CACHE.getIfPresent(String.valueOf(guildId));
            if(channels == null) {
                refreshChannelsCache(guildId);
                return builder.buildFuture();
            }

            String input = builder.getRemainingLowerCase();
            channels.asList().stream()
                    .map(channel -> channel.getAsJsonObject().get("name").getAsString())
                    .filter((name) -> name.toLowerCase().startsWith(input))
                    .forEach(builder::suggest);

            return builder.buildFuture();
        }
    }

    private static void refreshChannelsCache(long guildId) {
        isWaitingForResponse = true;
        DiscordBridge.discordClient.getGuildChannels(guildId, new DiscordSDK.NativeCallback() {
            public void onSuccess(String data) {
                System.out.println(data);
                CACHE.put(String.valueOf(guildId), JsonParser.parseString(data).getAsJsonArray());
                isWaitingForResponse = false;
            }
            public void onError(String message) {
                DiscordBridge.LOGGER.error("Failed to list guild channels of guild with Id {}: {}", guildId, message);
                isWaitingForResponse = false;
            }
        });
    }

}
