SET @Entry = 190010;
DELETE FROM `creature_template` WHERE `entry` = @Entry;
DELETE FROM `creature` WHERE `id` = @Entry;

SET @TEXT_ID := 601083;
DELETE FROM `npc_text` WHERE `ID` BETWEEN @TEXT_ID AND @TEXT_ID+5;

SET @STRING_ENTRY := 11100;
DELETE FROM `mangos_string` WHERE `entry` BETWEEN  @STRING_ENTRY+0 AND @STRING_ENTRY+29;

DELETE FROM `command` WHERE `name` IN ('transmog', 'transmog add', 'transmog add set');