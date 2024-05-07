SET @Entry = 190010;
DELETE FROM `creature_template` WHERE `entry` = @Entry;
DELETE FROM `locales_creature` WHERE entry = @Entry;
DELETE FROM `creature` WHERE `id` = @Entry;