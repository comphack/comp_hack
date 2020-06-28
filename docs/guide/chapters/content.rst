Game Content
============

This chapter describes how to create and modify game content to
create a custom feel to your server.

Zone Definitions
----------------

Zones are populated via server side definition files and driven
by actions and events mapped to the various parts defined within
them. All NPCs, doors, enemies and triggered actions are defined
in such files. Each server zone definition element is detailed
below.

ServerZone
^^^^^^^^^^

Defines a zone mapped to a unique dynamic map ID. All server
side zone settings begin with these definitions. Here is a
minimal example:

.. code-block:: xml

    <object name="ServerZone">
        <member name="ID">1</member>
        <member name="DynamicMapID">1</member>
        <member name="Global">true</member>
        <member name="GroupID">1</member>
        <member name="StartingX">0</member>
        <member name="StartingY">0</member>
        <member name="StartingRotation">0</member>
        <member name="NPCs"/>
        <member name="Objects"/>
        <member name="Bazaars"/>
        <member name="Spots"/>
    </object>

ID
""""

**Type:** integer

ID of the zone that must match defined IDs in the client-side
*ZoneData.sbin* file.

Example
#######

.. code-block:: xml

    <member name="ID">1</member>

DynamicMapID
""""""""""""

**Type:** integer

Unique dynamic map ID of the zone that must match defined IDs in
the client-side DynamicMapData.bin file. Zones with the same ID
but different dynamic map ID will often times have different spot
locations and will often incorporate different spawns or NPC
placement.

Example
#######

.. code-block:: xml

    <member name="DynamicMapID">1337</member>

Global
""""""

**Type:** boolean

Indicates that the zone is not instanced and will always exist on
the server. All players can enter global zones. If not set, the
zone will only exist for as long as it is needed for a zone
instance.

Example
#######

.. code-block:: xml

    <member name="Global">true</member>

GroupID
"""""""

**Type:** integer

Grouping ID used to identify the lobby zone for non-global
instanced zones or the zone itself, if defining one of these
lobbies.

Example
#######

.. code-block:: xml

    <member name="GroupID">42</member>

StartingX
"""""""""

**Type:** float

X coordinate to start all players entering the zone at unless
overridden by a zone change action.

Example
#######

.. code-block:: xml

    <member name="StartingX">1.23</member>

StartingY
"""""""""

**Type:** float

Y coordinate to start all players entering the zone at unless
overridden by a zone change action.

Example
"""""""

.. code-block:: xml

    <member name="StartingY">4.56</member>

StartingRotation
""""""""""""""""

**Type:** float

Rotation value (in radians) to start all players entering the
zone at unless overridden by a zone change action.

Example
#######

.. code-block:: xml

    <member name="StartingRotation">1.576</member>

XPMultiplier
""""""""""""

**Type:** float

Multiplier to apply to all defeated enemy values in the zone.
Defaults to 1.0.

Example
#######

.. code-block:: xml

    <member name="XPMultiplier">2.5</member>

BazaarMarketCost
""""""""""""""""

**Type:** integer

Macca cost for opening a bazaar market in the zone. Defaults to 100.

Example
#######

.. code-block:: xml

    <member name="BazaarMarketCost">9999</member><!-- Over 9000! -->

BazaarMarketTime
""""""""""""""""

**Type:** integer

Time in minutes that a bazaar market can be rented by one player.
Defaults to 60.

Example
#######

.. code-block:: xml

    <member name="BazaarMarketTime">30</member>


Background Music
----------------

Adding and Editing Entries
^^^^^^^^^^^^^^^^^^^^^^^^^^

To change or add a new sound into the game you must first edit the
*BinaryData\\Client\\CSoundData.bin* file with the *comp_bdpatch*
tool.

In this example we will modify the background music for 3rd Home.
First, dump the *CSoundData.bin* file into XML form. The
*comp_bdpatch* tool should be run from the Windows Command Prompt
or a Linux terminal. If you are using Windows you can drag and drop
a file into the command prompt to simplify typing in the path. Here
is how to dump the file into XML:

.. code-block:: bash

    comp_bdpatch load csound CSoundData.bin CSoundData.xml

Open up the XML file and you should see entries like the one below:

.. code-block:: xml

    <object name="MiCSoundData">
        <member name="ID">6</member>
        <member name="path"><![CDATA[bgm_000_some_file.mp3]]></member>
        <member name="location">SOUND_BGM</member>
        <member name="unk2">0</member>
        <member name="repeat">REPEAT_RANGE</member>
        <member name="unk4">0</member>
        <member name="volume">0.8000000119</member>
        <member name="start">1891264</member>
        <member name="stop">6964244</member>
    </object>

What follows is the description of some of these fields. If the
field is unknown you should not change it.

The *path* can be to a WAV or MP3 file. Other formats may be
supported but have not been tested. We have not tried adding a
directory to the path but it will most likely work. This file can
be encrypted or decrypted with the *comp_bgmtool* application.
Note that if you use this tool with the original client files you
may have to modify the key and magic used and then rebuild the
application. See the source code for why and how.

The *location* field determines where to load the sound file from.
It has the two following locations to pick from:

- SOUND_BGM - These files are stored in *sound\\bgm*.

- SOUND_SE - These files are stored in *sound\\se*.

The *repeat* field determines how the music will repeat (if at all).
It has the three following values to pick from:

- REPEAT_NONE - The music will not repeat.

- REPEAT_FULL - The music will repeat the entire file in a loop.

- REPEAT_RANGE - The music will play the file from the beginning and
  then loop back to *start* every time it reaches *stop*.

The *volume* is a floating point number with 1.0 being 100% volume.

The *start* and *stop* appear to be byte offsets for the repeat loop
when *repeat* is set to *REPEAT_RANGE*. The majority of the
following sections will be devoted to finding values to put here.

Once you have edited or added an entry (with a unique ID) you may
save the XML back into the *CSoundData.bin* file as follows:

.. code-block:: bash

    comp_bdpatch save csound CSoundData.xml CSoundData.bin

If you add a new entry you must tell the client when to play the
music/sound effect. For example, to change the background music or
battle theme for a zone edit the *BinaryData\\Shield\\ZoneData.sbin*
file with *comp_bdpatch*. Change the *zoneSoundID* and/or
*battleSoundID* for the desired zone:

.. code-block:: xml

    <member name="bgm">
        <object name="MiZoneBGMData">
            <member name="zoneSoundID">6</member>
            <member name="battleSoundID">60</member>
        </object>
    </member>

Background Music with Audacity
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This method of setting the repeat values does not require
proprietary software but does not work very well for MP3 files
so you may have to save the music as a WAV file. In any case open
your file in Audacity:

Using Audacity to loop background music.
""""""""""""""""""""""""""""""""""""""""

.. image:: /_static/bgm_audacity.png

There is two important things to note in this picture. First, the
format of the data is 44100 Hz Stereo 32-bit PCM. The 32-bit means
each sample is 4 bytes and the 44100 Hz means there is 44100
samples for every second of audio. Stereo means this has both
a left and right sample. Changing these parameters may change the
*start* and *stop* values in the XML by some factor. In this
example you multiply the sample number by 4 to get the *start* and
*stop* values. You may have to experiment if your audio is in a
different format.

Second is the start and end selection on the bottom of the screen.
This was changed from a time format to the number of samples. The
samples are directly related to the *start* and *stop* fields in
the XML.

Find the sample range that repeats the music with no noticeable
glitches in the audio. To help there is a loop play feature in
Audacity. Once you have found the samples and multiplied them by
the correct factor (4 in this example), save them into the XML
and re-generate the *CSoundData.bin* file. Try out the audio in
the client and enjoy.

If you really want to try looping an MP3 with Audacity you can
try this formula:

::
    ((sample / rate) * cbr) / 8 + offset

The value for *start* and *stop* appear to be a byte offset into
the file where the audio is. As such, there is a fixed *offset* of
bytes where the audio starts. For WAV this is either not needed or
so small that it doesn't seem to matter. For MP3 this can be in the
thousands and the Miles tool in the section below actually tells
you what it is. If you don't have this tool you'll need to guess.
The *rate* is the sample rate of the audio (in this example it was
44100 Hz). The *cbr* is the bit rate. Obviously I used a constant
bit rate here as a variable bit rate will make this very difficult.
The bit rate should be expressed in bits so in our example a rate
of 320 kbps would be a value of 320000 in the equation.

This equation gets the loop close but not quite right. Using this
method may take a lot of trial and error with the client. If you
have a better method of doing this, please let us know.

Background Music with Miles Sound Player
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The client is using the Miles sound system library so using their
tools would be the easiest way to do loops. If memory serves, this
tool used to be a download on the Miles website. This was many
years ago so your luck may vary. If you have access to this tool
it makes the process super simple. Here is what it looks like:

Using Miles Sound Player to loop background music.
""""""""""""""""""""""""""""""""""""""""""""""""""

.. image:: /_static/bgm_miles.png

The wave form on the bottom of the screen can be searched with
the slider up top or you can drag it. You can zoom with the scroll
wheel. When you left or right click on one of the vertical lines
it will place the start or stop time at that point in the audio.
You can then play the music and confirm your loop sounds okay.
Once you have a good loop just copy these two values from the top
left of the waveform into your XML and try it out in the client!
