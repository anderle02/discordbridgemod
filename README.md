# Discord Bridge Mod
This Minecraft Fabric 1.12.5 mod connects Minecraft with your Discord account,
using [Discord's Social SDK](https://discord.com/developers/docs/discord-social-sdk/overview).

## Features
This Mod has absolutely no features right now and is WIP.
### Planned Features
- Authentication Process
- Rich Presence
- Bridge Chat

## Why?
I wanted to try out the social SDK and feel like it could be nice since guilds wouldn't need to setup bridge bots anymore.
And it just looks a lot nicer if the message comes from yourself instead of a bot.

## Contributing
### Setup
- Clone the repository and open the project with IntelliJ.
- Recommended extensions: Minecraft Development
- Install a C/C++ compiler if you don't have one.
- Install CMake.
- Install the Discord Social SDK (standalone version). [Follow this guide](https://discord.com/developers/docs/discord-social-sdk/getting-started/using-c++) until step 4 to create your app and download the archive.
- Unzip that archive into `./discord_social_sdk` within this project.

After everything is installed, tell IntelliJ to rebuild the JNI library when running the Minecraft Client run configuration.
Add the following to your `.idea/runConfigurations/Minecraft_Client.xml` file.
```xml
<method v="2">
  <option name="Gradle.BeforeRunTask" enabled="false" tasks="buildNative" externalProjectPath="$PROJECT_DIR$/native" vmOptions="" scriptParameters="" />
</method>
```
Run `Minecraft Client` to start coding.

To edit the native code I'd recommend VS Code or some other IDE, IntelliJ doesn't really support C/C++.
### License
(coming soon)