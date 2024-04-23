# Transmog
Module to allow changing your equipped item appearances with any items you have ever obtained during your characters lifetime. This tries to simulate the transmog feature introduced in Cataclysm.

The transmog npc will be located in Stormwind Mage Tower and in Orgrimmar Valley of Spirits.

# Available Cores
Classic, TBC and WoTLK

# How to install
1. Follow the instructions in https://github.com/davidonete/cmangos-modules?tab=readme-ov-file#how-to-install
2. Enable the `BUILD_MODULE_TRANSMOG` flag in cmake and run cmake. The module should be installed in `src/modules/transmog`
3. Copy the configuration file from `src/modules/transmog/src/transmog.conf.dist.in` and place it where your mangosd executable is. Also rename it to `transmog.conf`.
4. Remember to edit the config file and modify the options you want to use.
5. You will also have to install the database changes located in the `src/modules/transmog/sql/install` folder, each folder inside represents where you should execute the queries. E.g. The queries inside of `src/modules/transmog/sql/install/world` will need to be executed in the world/mangosd database, the ones in `src/modules/transmog/sql/install/characters` in the characters database, etc...
6. Lastly in order to use the system you will need to install the addon to your client. Pick one of the addon versions based on your client version from the `addons` folder.

# How to uninstall
To remove transmog from your server you have multiple options, the first and easiest is to disable it from the hardcore.conf file. The second option is to completely remove it from the server and db:
1. Remove the `BUILD_MODULE_TRANSMOG` flag from your cmake configuration and recompile the game
2. Execute the sql queries located in the `src/modules/transmog/sql/uninstall` folder. Each folder inside represents where you should execute the queries. E.g. The queries inside of `src/modules/transmog/sql/uninstall/world` will need to be executed in the world/mangosd database, the ones in `src/modules/transmog/sql/uninstall/characters` in the characters database, etc...
