Configuration Files
===================

.. role:: raw-html(raw)
    :format: html

There are three configuration files to worry about:

- *lobby.xml* is used for the
  lobby server

- *world.xml* is used for
  the world server

- *channel.xml* is used for
  the channel server

Note that these files can have a different name. By default, the
server will search for this configuration file in the following
location:

- *Windows* - the file
  *lobby.xml* or *world.xml* or *channel.xml*
  (depending on which server) in the current working directory

- *Linux* - the *lobby.xml* or *world.xml* or *channel.xml*
  (depending on which server) in */etc/comp_hack*

If you don’t want to use the default name and location, you can
specify the configuration file path on the command line:

.. code-block:: bash

    comp_channel /path/to/my/config.xml

All three configuration files share some settings in common.
Those settings are documented here.

As with any xml file, it should start with this line:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>

All settings are enclosed between the *<object>*
and *</object>* tags. This *<object>* section is wrapped by the
tags *<objgen>* and *</objgen>*. Inside the *<object>* section is
a series of *<member>* tags for each setting. Here is an example
of how this should look:

.. code-block:: xml

    <?xml version="1.0" encoding="UTF-8"?>
    <objgen>
        <object>
            <member name="DatabaseType">SQLITE3</member>
        </object>
    </objgen>

Some settings may be a group of other settings. In this case the
*<member>* tag encloses an *<object>* with more settings. Here is
an example:

.. code-block:: xml

    <member name="SQLite3Config">
        <object>
            <member name="FileDirectory">/var/lib/comp_hack/database</member>
        </object>
    </member>

Each *<member>* tag has a *name* attribute that specifies which
configuration option is being changed. What follows is a
description of the configuration options. These sections cover the
common configuration shared by all three servers, the database
configuration shared by the lobby and world servers or world and
channel servers, and any configuration specific to a server.

Common Server Configuration
---------------------------

These settings apply to all three server configurations.

DiffieHellmanKeyPair
^^^^^^^^^^^^^^^^^^^^

**Type:** string
:raw-html:`<br />`
**Default:** *blank*

This is the public key pair used for encrypting new client
connections. Leave this option out and the server will generate
a new pair for your server to add to the configuration.

Example
"""""""

.. code-block:: xml

    <member name="DiffieHellmanKeyPair">940134C09FB9BABE187BCE1030E63...</member>

Port
^^^^

**Type:** integer

**Default:** 10666 (lobby), 18666 (world) or 14666 (channel)

Main port for the server to listen for connections on. For the
lobby and channel these should be open to the internet for
clients. For the world server this port should be blocked by
your firewall. For the lobby this is usually 10666. For the
channel this is usually 14666, 15666, etc. For the world this
is usually 18666.

Example
"""""""

.. code-block:: xml

    <member name="Port">10666</member>

DatabaseType
^^^^^^^^^^^^

**Type:** enumeration

**Default:** MARIADB

The type of database being used. Must be either *MARIADB* or
*SQLITE3*. *MARIADB* is strongly recommended for production
servers and testing.

Example
"""""""

.. code-block:: xml

    <member name="DatabaseType">MARIADB</member>

MultithreadMode
^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Just leave it on unless your computer is an 8086.

Example
"""""""

.. code-block:: xml

    <member name="MultithreadMode">true</member>

DataStore
^^^^^^^^^

**Type:** string

**Default:** *blank*

Location of the game data directory.

Example
"""""""

.. code-block:: xml

    <member name="DataStore">/var/lib/comp_hack</member>

DataStoreSync
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

This feature is not implemented yet but it would sync the data
store between servers (if they were on different systems and
need this).

Example
"""""""

.. code-block:: xml

    <member name="DataStoreSync">true</member>

LogFile
^^^^^^^

**Type:** string

**Default:** *blank*

Path to the log file.

Example
"""""""

.. code-block:: xml

    <member name="LogFile">/var/log/comp_hack/lobby.log</member>

LogFileTimestamp
^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Indicates if the log file should include timestamps.

Example
"""""""

.. code-block:: xml

    <member name="LogFileTimestamp">true</member>

LogFileAppend
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Appends to an existing log file if it exists. If disabled, the
log file is replaced.

Example
"""""""

.. code-block:: xml

    <member name="LogFileAppend">true</member>

LogLevels
^^^^^^^^^

**Type:** map

**Default:** WARNING

Map of log categories to log level. Each category can be set to
LEVEL_DEBUG, LEVEL_INFO, LEVEL_WARNING, LEVEL_ERROR or
LEVEL_CRITICAL. Messages at the set level or more critical
level(s) will be logged. For example, a level of LEVEL_WARNING
will log messages at the LEVEL_ERROR and LEVEL_CRITICAL level as
well.

Here is the list of categories:

- AccountManager

- ActionManager

- AIManager

- Barter

- Bazaar

- CharacterManager

- ChatManager

- Clan

- Connection

- Crypto

- Database

- DataStore

- DataSyncManager

- DefinitionManager

- Demon

- EventManager

- Friend

- FusionManager

- General

- Invalid

- Item

- MatchManager

- Packet

- Party

- ScriptEngine

- Server

- ServerConstants

- ServerDataManager

- SkillManager

- Team

- TokuseiManager

- Trade

- WebAPI

- ZoneManager

Example
"""""""

.. code-block:: xml

    <member name="LogLevels">
        <pair>
            <key>General</key>
            <value>LEVEL_DEBUG</value>
        </pair>
    </member>

LogRotation
^^^^^^^^^^^

**Type:** boolean

**Default:** false

Indicates if the logs should be rotated on a periodic basis.

Example
"""""""

.. code-block:: xml

    <member name="LogRotation">true</member>

LogCompression
^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Indicates if the logs should be compressed when rotated.

Example
"""""""

.. code-block:: xml

    <member name="LogCompression">true</member>

LogRotationCount
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 3

Indicates the number of rotated logs to keep. A negative value
will keep all rotated logs.

Example
"""""""

.. code-block:: xml

    <member name="LogRotationCount">-1</member>

LogRotationDays
^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 1

Indicates the number of days before the logs will be rotated.

Example
"""""""

.. code-block:: xml

    <member name="LogRotationDays">7</member>

CapturePath
^^^^^^^^^^^

**Type:** string

**Default:** *blank*

Directory to store packet captures of client sessions. If this
is empty, client sessions are not saved.

Example
"""""""

.. code-block:: xml

    <member name="CapturePath">/var/log/comp_hack/captures</member>

ServerConstantsPath
^^^^^^^^^^^^^^^^^^^

**Type:** string

**Default:** *blank*

Path to the server constants.xml file. If this is blank it defaults
to the same directory as the lobby, world and channel config files.

Example
"""""""

.. code-block:: xml

    <member name="ServerConstantsPath">/etc/comp_hack/constants.xml</member>


Lobby Server Configuration
--------------------------

These settings apply to only the lobby server configuration.

WebListeningPort
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 10999

Port to listen for web authentication or REST API requests on.
This defaults to 10999. Do not expose this to clients if you use
the REST API for your own site. Have users login through your
site instead. If you give clients access to this port you should
disable the REST API.

Example
"""""""

.. code-block:: xml

    <member name="WebListeningPort">10999</member>

WebCertificate
^^^^^^^^^^^^^^

**Type:** string

**Default:** *blank*

SSL certificate for the web authentication or REST API. If blank,
SSL is not used. It is highly recommended to use SSL, especially
if the port is exposed to the internet. This should combine the
certificate and any other certificates you need (like the CA cert).

Example
"""""""

.. code-block:: xml

    <member name="WebCertificate">/etc/ssl/mysite.crt</member>

WebRoot
^^^^^^^

**Type:** string

**Default:** *blank*

Directory to add to the web authentication server. If blank no
directory is added. This may not be safe when exposed to the
internet. This directory should not contain symlinks as they may
be followed.

Example
"""""""

.. code-block:: xml

    <member name="WebRoot">/var/www/comp_hack</member>

WebAuthTimeOut
^^^^^^^^^^^^^^

**Type:** integer

**Default:** 15

Number of seconds until a new lobby connection via web auth
expires. If the user does not log into the lobby before this
expires they have to start the login process over again.

Example
"""""""

.. code-block:: xml

    <member name="WebAuthTimeOut">10</member>

ClientVersion
^^^^^^^^^^^^^

**Type:** float

**Default:** 1.666

Version the client must be to login. This defaults to 1.666 (the
last Japanese client release).

Example
"""""""

.. code-block:: xml

    <member name="ClientVersion">1.666</member>

CharacterDeletionDelay
^^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 1440

Number of minutes unil a character marked for deletion is
deleted. This defaults to 24 hours.

Example
"""""""

.. code-block:: xml

    <member name="CharacterDeletionDelay">10</member>

CharacterTicketCost
^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0

Cost (in CP) to purchase another character ticket.

Example
"""""""

.. code-block:: xml

    <member name="CharacterTicketCost">10</member>

StartupCharacterDelete
^^^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Automatically deletes characters from a connecting world server
that have a KillTime that has passed. If not enabled, these
characters will be deleted the next time their associated account
is sent the character list.

Example
"""""""

.. code-block:: xml

    <member name="StartupCharacterDelete">false</member>

RegistrationCP
^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0

Amount of CP to give new accounts.

Example
"""""""

.. code-block:: xml

    <member name="RegistrationCP">1000</member>

RegistrationTicketCount
^^^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 1

Amount of character tickets to give new accounts.

Example
"""""""

.. code-block:: xml

    <member name="RegistrationTicketCount">3</member>

RegistrationUserLevel
^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0

User level of new accounts. 0 is a normal account, anything
non-zero is a GM and 1000 is a full GM with all permissions.

Example
"""""""

.. code-block:: xml

    <member name="RegistrationUserLevel">1000</member>

RegistrationAccountEnabled
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Indicates if new accounts should be enabled by default.

Example
"""""""

.. code-block:: xml

    <member name="RegistrationAccountEnabled">false</member>

PlayOpeningMovie
^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

When set the opening cutscene will play for new accounts.

Example
"""""""

.. code-block:: xml

    <member name="PlayOpeningMovie">false</member>

AllowImport
^^^^^^^^^^^

**Type:** boolean

**Default:** true

Allow importing of account data.

Example
"""""""

.. code-block:: xml

    <member name="AllowImport">false</member>

ImportStripUserLevel
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Reset the user level of imported accounts to 0.

Example
"""""""

.. code-block:: xml

    <member name="ImportStripUserLevel">false</member>

ImportStripCP
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Reset the CP of imported accounts to 0.

Example
"""""""

.. code-block:: xml

    <member name="ImportStripCP">false</member>

ImportMaxPayload
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 5120

Sets the maximum payload size (in kilobytes) for an account
import request.

Example
"""""""

.. code-block:: xml

    <member name="ImportMaxPayload">1024</member>

ImportWorld
^^^^^^^^^^^

**Type:** integer

**Default:** 0

ID of the world to import characters into when importing
account data.

Example
"""""""

.. code-block:: xml

    <member name="ImportWorld">1</member>

MaxClients
^^^^^^^^^^

**Type:** integer

**Default:** 0

Maximum number of clients that may connect from the same machine.
If this number is 0 or less, an unlimited number of clients may
connect from the same machine.

Example
"""""""

.. code-block:: xml

    <member name="MaxClients">2</member>


World Server Configuration
--------------------------

These settings apply to only the world server configuration.

ID
^^^

**Type:** integer

**Default:** 0

ID of the world when registering it with the lobby. Should start
at 0 and increment for each new world. Must be in the range
(0, 255).

Example
"""""""

.. code-block:: xml

    <member name="ID">0</member>

Name
^^^^

**Type:** string

**Default:** *blank*

Name of the world (shown in lobby).

Example
"""""""

.. code-block:: xml

    <member name="Name">Test World</member>

LobbyIP
^^^^^^^

**Type:** string

**Default:** 127.0.0.1

IP address of the lobby server to connect to. This defaults to
127.0.0.1 (localhost).

Example
"""""""

.. code-block:: xml

    <member name="LobbyIP">127.0.0.1</member>

LobbyPort
^^^^^^^^^

**Type:** integer

**Default:** 10666

Port of the lobby server to connect to. This defaults to 10666.

Example
"""""""

.. code-block:: xml

    <member name="LobbyPort">127.0.0.1</member>

ChannelConnectionTimeOut
^^^^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 15

Number of seconds until a new channel connection expires. If the
user does not log into the channel before this expires they have
to start the login process over again.

Example
"""""""

.. code-block:: xml

    <member name="ChannelConnectionTimeOut">10</member>

WorldSharedConfig
^^^^^^^^^^^^^^^^^

**Type:** object (WorldSharedConfig)

**Default:** default settings object

Sub-section for all world shared config settings. For more
information see the dedicated documentation section.

Example
"""""""

.. code-block:: xml

    <member name="WorldSharedConfig">
        <object>
            <member name="AutoCompressCurrency">true</member>
        </object>
    </member>


Channel Server Configuration
----------------------------

These settings apply to only the channel server configuration.

Name
^^^^

**Type:** string

**Default:** *blank*

Name of the channel (shown in the old channel list).

Example
"""""""

.. code-block:: xml

    <member name="Name">Test Channel</member>

ExternalIP
^^^^^^^^^^

**Type:** string

**Default:** *blank*

IP of the channel to give clients. If blank, the IP is auto-detected.

Example
"""""""

.. code-block:: xml

    <member name="ExternalIP">Test Channel</member>

WorldIP
^^^^^^^

**Type:** string

**Default:** 127.0.0.1

IP address of the world server to connect to. This defaults to
127.0.0.1 (localhost).

Example
"""""""

.. code-block:: xml

    <member name="WorldIP">127.0.0.1</member>

WorldPort
^^^^^^^^^

**Type:** integer

**Default:** 18666

Port of the world server to connect to. This defaults to 18666.

Example
"""""""

.. code-block:: xml

    <member name="WorldPort">18666</member>

Timeout
^^^^^^^

**Type:** integer

**Default:** 0

Number of seconds with no new packets until the client will
timeout and be disconnected. Defaults to 0 (no timeout).

Example
"""""""

.. code-block:: xml

    <member name="Timeout">15</member>

SystemMessage
^^^^^^^^^^^^^

**Type:** string

**Default:** *blank*

System ticker message to show client upon connection to the
channel. If blank, no ticker is shown.

Example
"""""""

.. code-block:: xml

    <member name="SystemMessage">Welcome to the server!</member>

SystemMessageColor
^^^^^^^^^^^^^^^^^^

**Type:** enum

**Default:** *BLUE*

System ticker message color to use when displaying the configured
SystemMessage. Valid options are RED, WHITE, BLUE and PURPLE.

Example
"""""""

.. code-block:: xml

    <member name="SystemMessageColor">RED</member>

AutoCompressCurrency
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

When set Macca and Magnetite will combine into notes and pressures
when selling items and picking up drops.

Example
"""""""

.. code-block:: xml

    <member name="AutoCompressCurrency">true</member>

PerfMonitorEnabled
^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Enables performance monitoring statistics of the server.

Example
"""""""

.. code-block:: xml

    <member name="PerfMonitorEnabled">true</member>

VerifyServerData
^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Enables server data verification upon startup, stopping the server
if an issue is found.

Example
"""""""

.. code-block:: xml

    <member name="VerifyServerData">true</member>


World Shared Configuration
--------------------------

These settings apply to the world server configuration as well
as any channels that connect to it.

TimeOffset
^^^^^^^^^^

**Type:** integer

**Default:** 540

Number of minutes to offset the functional system time actions
by from the normal GMT based time, defaulting to JST (GMT+9).
Adjusts both time of day as well as what the current day is
(can fastforward to weekend etc). This will affect all system
time based events and triggers as well as when the servers
consider it to be midnight.

Example
"""""""

.. code-block:: xml

    <member name="TimeOffset">-180</member>

GreetMessage
^^^^^^^^^^^^

**Type:** string

**Default:** Welcome to the COMP_hack Server!

Sets the message that will display in the chat when the
character logs into the channel.

Example
"""""""

.. code-block:: xml

    <member name="GreetMessage">Welcome to my custom server!</member>

COMPShopMessage
^^^^^^^^^^^^^^^

**Type:** string

**Default:** Welcome to the COMP Shop!

Sets the message that will display in the COMP shop main menu.

Example
"""""""

.. code-block:: xml

    <member name="COMPShopMessage">Welcome to my custom COMP Shop!</member>

MoveCorrection
^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Double check player movements server side to ensure they are
not glitching through walls. Only disable if server performance
is very poor.

Example
"""""""

.. code-block:: xml

    <member name="MoveCorrection">true</member>

AutoCompressCurrency
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

If enabled, completing a full macca or mag stack of items will
convert it to a macca note or mag presser.

Example
"""""""

.. code-block:: xml

    <member name="AutoCompressCurrency">true</member>

NRAStatusNull
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

If enabled, a single point in affinity null, reflect or absorb
will nullify all negative status effects being applied that have
the same affinity.

Example
"""""""

.. code-block:: xml

    <member name="NRAStatusNull">false</member>

DeathPenaltyDisabled
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

If enabled, no death XP penalty will be enforced.

Example
"""""""

.. code-block:: xml

    <member name="DeathPenaltyDisabled">true</member>

DeadTokuseiDisabled
^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

If enabled, tokusei effects originating from an entity will
stop the moment it dies and re-enable when they are revived.
Effects that target the dead entity will still apply.

Example
"""""""

.. code-block:: xml

    <member name="DeadTokuseiDisabled">true</member>

DropLuckScalingCap
^^^^^^^^^^^^^^^^^^

**Type:** float

**Default:** -1.0

When set to 0 or greater, sets a hard multiplier cap on how
much a drop rate can increase due to the source entity's luck
stat (ex: 0.5 would cap a 5% drop rate at 7.5%). When set to
less than 0, no cap applies. Applies before and does not affect
DropRateBonus setting.

Example
"""""""

.. code-block:: xml

    <member name="DropLuckScalingCap">1.0</member>

XPBonus
^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional XP percentage gained when killing an enemy
(ex: 0.5 would grant 150%). Does not affect individual boosts
players have.

Example
"""""""

.. code-block:: xml

    <member name="XPBonus">50.0</member>

ExpertiseBonus
^^^^^^^^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional expertise percentage gained when using a
skill (ex: 0.1 would grant 110%). Does not affect individual
boosts players have.

Example
"""""""

.. code-block:: xml

    <member name="ExpertiseBonus">10.0</member>

DropRateBonus
^^^^^^^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional item drop rate multiplied to all item
drops (ex: 10.0 would make 5% drops start at 5.5%).

Example
"""""""

.. code-block:: xml

    <member name="DropRateBonus">10.0</member>

LoginPointBonus
^^^^^^^^^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional login point gain percentage (used for clans)
when logging in for the day (ex: 0.5 would grant 150%).

Example
"""""""

.. code-block:: xml

    <member name="LoginPointBonus">3.0</member>

FusionGaugeBonus
^^^^^^^^^^^^^^^^

**Type:** float

**Default:** 3.0

Sets the additional fusion gauge amount gained when using a
skill (ex: 0.1 would grant 110%).

Example
"""""""

.. code-block:: xml

    <member name="FusionGaugeBonus">10.0</member>

BethelBonus
^^^^^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional bethel percentage gained during a Pentalpha
match.

Example
"""""""

.. code-block:: xml

    <member name="BethelBonus">10.0</member>

DigitalizePointBonus
^^^^^^^^^^^^^^^^^^^^

**Type:** float

**Default:** 0.0

Sets the additional digitalize XP percentage gained when killing
an enemy or completing a digitalize dungeon (ex: 0.5 would grant
150%). Does not affect individual boosts players have.

Example
"""""""

.. code-block:: xml

    <member name="DigitalizePointBonus">5.0</member>

LevelUpBonusChance
^^^^^^^^^^^^^^^^^^

**Type:** float

**Default:** 20.0

Flat chance per level gained by characters to get 2 bonus
unallocated skill points. If multiple levels are gained at once,
this is checked once per level. Can be further increased using
the LevelUpBonusMax setting.

Example
"""""""

.. code-block:: xml

    <member name="LevelUpBonusChance">50.0</member>

LevelUpBonusMax
^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 1

Number of times per level up bonus points can be gained for a
character. Bonus points are gained in pairs of 2 so a value of 4
can result in 0, 2, 4, 6 or 8 points per level. For each bonus
the LevelUpBonusChance setting is checked again.

Example
"""""""

.. code-block:: xml

    <member name="LevelUpBonusMax">2</member>

EnabledDemonQuests
^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0x0777

Flag mask that determines which types of demon quests can be
offered to players. The flags are as follows: kill demons (1),
fuse demons (2), contract demons (3), give items (4), crystallize
demon (5), tarot enchant (6), soul enchant (7), modify equipment
(8), synth melee weapon (9), synth long range weapon (10) and
open plasma (11). Enabling a quest type does not bypass other
restrictions to receive them.

Example
"""""""

.. code-block:: xml

    <member name="EnabledDemonQuests">0x07FF</member>

BazaarMarketCost
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0

Overrides bazaar market costs (in macca) for all zones where
bazaars exist if not set to zero.

Example
"""""""

.. code-block:: xml

    <member name="BazaarMarketCost">1000</member>

BazaarMarketTime
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 0

Overrides bazaar market times (in minutes) for all zones where
bazaars exist if not set to zero.

Example
"""""""

.. code-block:: xml

    <member name="BazaarMarketTime">30</member>

CritDefenseReduction
^^^^^^^^^^^^^^^^^^^^

**Type:** float

**Default:** 1.0

Designates what percentage of the defensive stat factored into
damage formulas is ignored when a critical hit or higher occurs.
Values must be between 0.0 and 1.0.

Example
"""""""

.. code-block:: xml

    <member name="CritDefenseReduction">0.5</member>

LevelCap
^^^^^^^^

**Type:** integer

**Default:** 99

Maximum level that can be obtained for player characters and
demons. Enemies can still go over this level. Values must be
between 1 and 99.

Example
"""""""

.. code-block:: xml

    <member name="LevelCap">98</member>

ReunionMax
^^^^^^^^^^

**Type:** integer

**Default:** 9

Maximum reunion rank that can be obtained by a demon. Ranks higher
than 9 can be obtained at level 99 and stat progression will
continue uniformly but while the correct rank will display in the
UI, the rank 9 growth type will still be used. Values must be
between 9 and 99.

Example
"""""""

.. code-block:: xml

    <member name="ReunionMax">99</member>

BikeBoostHide
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

If disabled, when a character boosts on a bike, they will still
speed up and enter first person view but will not become invisible
to other players.

Example
"""""""

.. code-block:: xml

    <member name="BikeBoostHide">false</member>

PvPQueueWait
^^^^^^^^^^^^

**Type:** integer

**Default:** 120

Amount of time in seconds that must pass before a queued PvP match
goes from player count requirements met to sending invites.

Example
"""""""

.. code-block:: xml

    <member name="PvPQueueWait">5</member>

PvPGhosts
^^^^^^^^^

**Type:** array of two integers

**Default:** 0

Set of two values that can be specified to artifically inflate
the number of players waiting in the Fate (first) and Valhalla
(second) PvP queues to allow for smaller matches. These numbers
will not decrease the maximum size if more players are waiting.

Example
"""""""

.. code-block:: xml

    <member name="PvPGhosts">
        <element>10</element>
        <element>4</element>
    </member>

PentalphaPayoutAll
^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Determines if everyone should be paid cowrie from participating
in a Pentalpha match as opposed to just the rank 1 team(s). Teams
below 1st place are reduced by 10% per rank.

Example
"""""""

.. code-block:: xml

    <member name="PentalphaPayoutAll">true</member>

PentalphaMinMaxPayout
^^^^^^^^^^^^^^^^^^^^^

**Type:** array of two integers

**Default:** -1

Set of two values that specify the minimum and maximum base
cowrie payout values respectively upon paying a Pentalpha match.
Negative values default to limits of 100 min, 2000 max.

Example
"""""""

.. code-block:: xml

    <member name="PentalphaMinMaxPayout">
        <element>10</element>
        <element>300</element>
    </member>

DailyGPLoss
^^^^^^^^^^^

**Type:** integer

**Default:** 3

Indicates the number of PvP Grade Points points lost every day
since the player's last login to guarantee active players are
top rank only.

Example
"""""""

.. code-block:: xml

    <member name="DailyGPLoss">0</member>

ChannelDistribution
^^^^^^^^^^^^^^^^^^^

**Type:** map of integer to integer

**Default:** empty

Map of ServerZone or ServerZoneInstance GroupID values to channel
IDs specifying which channel the zones should be created on. If no
entries are specified here, each channel server connected to the
world will host every zone and the manual channel change control
will be enabled. If a single entry is specified, all unspecified
zones will default to the primary channel (channel 0).

Example
"""""""

.. code-block:: xml

    <member name="ChannelDistribution">
        <pair>
            <key>10101</key>
            <value>0</value>
        </pair>
    </member>

AIAggroLimit
^^^^^^^^^^^^

**Type:** enumeration

**Default:** PLAYER_SHARED

Determines how AI controlled eneities decide to aggro on a valid
target that is already being pursued. Values must be *PLAYER_SHARED*
(character and demon limit are shared), *INDEPENDENT* (character and
demon limit are not shared) or *NONE* (aggro limit is ignored).

Example
"""""""

.. code-block:: xml

    <member name="AIAggroLimit">INDEPENDENT</member>

AICombatStagger
^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Causes AI controlled entities to pause for a brief period after
skill charge completion as well as when they are able to move but
their target is being knocked back.

Example
"""""""

.. code-block:: xml

    <member name="AICombatStagger">false</member>

AIEstomaBossIgnore
^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Defaults BOSS category spawns to completely ignore the effects of
the aggro clearing Estoma skill.

Example
"""""""

.. code-block:: xml

    <member name="AIEstomaBossIgnore">true</member>

AIEstomaChargeIgnore
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Causes AI controlled entities to not lose aggro when hit with
Estoma from an entity they are currently targeting with a skill.

Example
"""""""

.. code-block:: xml

    <member name="AIEstomaChargeIgnore">true</member>

AIEstomaDuration
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 30

Sets the amount of time in seconds that an AI controlled entity
hit with Estoma will remain inactive.

Example
"""""""

.. code-block:: xml

    <member name="AIEstomaDuration">10</member>

AILazyPathing
^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Disables AI pathing around zone geometry when enabled. Entities
will still navigate around obstacles for explicit move commands,
wandering and finding their way back to spawn points.

Example
"""""""

.. code-block:: xml

    <member name="AILazyPathing">false</member>

IFramesEnabled
^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

Enforce invincibility frames when multiple entities try to use
combat skills on one target at the same time resulting in retry
packets being sent to some of the attackers to stagger hits.
Never applies to entities with hitstun null unless they are being
knocked back.

Example
"""""""

.. code-block:: xml

    <member name="IFramesEnabled">false</member>

SpawnSpamUserLevel
^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 500

Users below this account level will be subject to spawn limitations.
This means spawns through skills or items (demon eggs) will be
limited and potentially ban the user.

Example
"""""""

.. code-block:: xml

    <member name="SpawnSpamUserLevel">500</member>

SpawnSpamUserMax
^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 30

Maximum number of enemies a user may have spawned in a given zone
before they are prevented from spawning more enemies. This is a
soft cap. If a skill or item spawns multiple enemies it is possible
to go over this cap; however, another skill or item cannot be used
to spawn enemies again until the number of enemies spawned drops
below the cap again.

Example
"""""""

.. code-block:: xml

    <member name="SpawnSpamUserMax">30</member>

SpawnSpamGlobalZoneMax
^^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 100

Maximum number of enemies that may have spawned through an item
or skill in a given global zone before users are prevented from
spawning more enemies. This is a soft cap. If a skill or item
spawns multiple enemies it is possible to go over this cap;
however, another skill or item cannot be used to spawn enemies
again until the number of enemies spawned drops below the cap
again. Note that users at or above the `SpawnSpamUserLevel`
level may still spawn enemies. These will count towards the
soft cap and so other users must await until the total count
goes below the soft cap before they can spawn new enemies.

Example
"""""""

.. code-block:: xml

    <member name="SpawnSpamGlobalZoneMax">100</member>

SpawnSpamInstanceZoneMax
^^^^^^^^^^^^^^^^^^^^^^^^

**Type:** integer

**Default:** 30

Maximum number of enemies that may have spawned through an item
or skill in a given instanced zone before users are prevented
from spawning more enemies. This is a soft cap. If a skill or
item spawns multiple enemies it is possible to go over this cap;
however, another skill or item cannot be used to spawn enemies
again until the number of enemies spawned drops below the cap
again. Note that users at or above the `SpawnSpamUserLevel`
level may still spawn enemies. These will count towards the
soft cap and so other users must await until the total count
goes below the soft cap before they can spawn new enemies.

Example
"""""""

.. code-block:: xml

    <member name="SpawnSpamInstanceZoneMax">30</member>

AutobanSpawnSpammers
^^^^^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

If a user (that is not at or above the `SpawnSpamUserLevel` level)
attempts to spawn more enemies after reaching their soft cap for
the zone (`SpawnSpamUserMax`) they will be automatically banned.
The reason will be recorded for GMs to later ajudicate the ban.

Example
"""""""

.. code-block:: xml

    <member name="AutobanSpawnSpammers">true</member>

ClockSkewThreshold
^^^^^^^^^^^^^^^^^^

**Type:** float

**Default:** 1.5

Users who have a client time that increases at a faster than normal
rate that is at or above this threshold will be either kicked or
banned depending on the `AutobanClockSkew` configuration option.

Example
"""""""

.. code-block:: xml

    <member name="ClockSkewThreshold">1.5</member><!-- 1.5x normal -->

ClockSkewCount
^^^^^^^^^^^^^^

**Type:** integer

**Default:** 5

Number of times a user may go over the `ClockSkewThreshold` before
they are kicked or banned depending on the `AutobanClockSkew` setting.

Example
"""""""

.. code-block:: xml

    <member name="ClockSkewCount">5</member>

AutobanClockSkew
^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** false

Users who have a client time that increases at a faster than normal
rate that is at or above the `ClockSkewThreshold` threshold will be
automatically banned. The reason will be recorded for GMs to later
ajudicate the ban.

Example
"""""""

.. code-block:: xml

    <member name="AutobanClockSkew">true</member>

Database Configuration
----------------------

These settings configure the database.

DatabaseName
^^^^^^^^^^^^

**Type:** string

**Default:** *comp_hack* / *world*

Name of the database to use. This defaults to *comp_hack* for
the lobby server and *world* for the world server.

Example
"""""""

.. code-block:: xml

    <member name="DatabaseName">comp_hack</member>

MockData
^^^^^^^^

**Type:** boolean

**Default:** false

If the database should be pre-loaded with data.

Example
"""""""

.. code-block:: xml

    <member name="MockData">comp_hack</member>

MockDataFilename
^^^^^^^^^^^^^^^^

**Type:** string

**Default:** *blank*

File of objects to pre-load the database with.

Example
"""""""

.. code-block:: xml

    <member name="MockDataFilename">/etc/comphack/test_lobby_setup.xml</member>

AutoSchemaUpdate
^^^^^^^^^^^^^^^^

**Type:** boolean

**Default:** true

If disabled, this will prevent the database from being updated.

Example
"""""""

.. code-block:: xml

    <member name="AutoSchemaUpdate">true</member>


MariaDB Configuration
---------------------

These settings configure the MariaDB database.

IP
^^^^

**Type:** string

**Default:** 127.0.0.1

IP address of the MariaDB or MySQL server. This defaults to
127.0.0.1 (localhost).

Example
"""""""

.. code-block:: xml

    <member name="IP">127.0.0.1</member>

Port
^^^^

**Type:** integer

**Default:** 3306

Port of the MariaDB or MySQL server. If omitted this uses the
default port.

Example
"""""""

.. code-block:: xml

    <member name="Port">127.0.0.1</member>

Username
^^^^^^^^

**Type:** string

**Default:** *blank*

Username to use for the database connection.

Example
"""""""

.. code-block:: xml

    <member name="Username">admin</member>

Password
^^^^^^^^

**Type:** string

**Default:** *blank*

Password to use for the database connection.

Example
"""""""

.. code-block:: xml

    <member name="Password">hackme</member>


SQLite3 Database Configuration
------------------------------

These settings configure the SQLite3 database.

FileDirectory
^^^^^^^^^^^^^

**Type:** string

**Default:** *blank*

The directory to store the database file in.

Example
"""""""

.. code-block:: xml

    <member name="FileDirectory">/var/lib/comp_hack/</member>

MaxRetryCount
^^^^^^^^^^^^^

**Type:** integer

**Default:** 3

Number of times to retry a SQL operation. This defaults to 3.

Example
"""""""

.. code-block:: xml

    <member name="MaxRetryCount">1</member>

RetryDelay
^^^^^^^^^^

**Type:** integer

**Default:** 500

Number of milliseconds to wait before retrying an SQL operation.
This defaults to 500 milliseconds.

Example
"""""""

.. code-block:: xml

    <member name="RetryDelay">250</member>
