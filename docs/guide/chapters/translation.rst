Translation
===========

This chapter describes how to use various tools available to translate game content to another language.

comp_translator
---------------

``comp_translator`` is the primary tool used to take client-side game content files in human-readable XML format and package them into binary file format readable by the game. The tool is configurable by editing the human-readable Squirrel script file ``build.nut``.

Setting up the directory for comp_translator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Before running ``comp_translator``, it needs to have file directories next to it filled with XML files which can eb downloaded from the translation git repository. Generally, the directory tree will look something like this:

.. code-block:: none
    :force: 

    comp_translator directory
    ├── comp_translator.exe (on Windows)
    ├── build.nut
    ├── BinaryData
    │   ├── Client
    │   │   └── XML files
    │   ├── Shield
    │   │   └── XML files
    ├── Event
    │   ├── MultiTalk
    │   │   └── Subdirectories with XML files
    │   ├── PolygonMovies
    │   │   └── Subdirectories with XML files

Running comp_translator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After running ``comp_translator``, two things will be generated: a ``build`` directory and a logging file named ``build.log.`` It is recommended to examine build.log for any errors before applying any translation. "Build Successful" should be on the bottom.

Editing build.nut
-----------------

``build.nut`` is editable using any number of text editors, with something like Notepad++ being recommended if using Microsoft Windows. Below is an example of the relevant sections of build.nut that a user might wish to edit.

Sample
^^^^^^

.. code-block:: javascript

    OUT_DIR <- "build";

    CLIENT_FILES <- {
    "UIInfoData": "uiinfo",
    };

    SHIELD_FILES <- {
    "CBattleTalk": "ctalkmessage",
    };

OUT_DIR
"""""""

**Type:** string

This variable defines the name of the directory that ``comp_translator`` will deposit built binary files to. The contents of this directory can be copied directly to the actual game directory, in order to translate client-side game content.

Example
#######

.. code-block:: javascript

    OUT_DIR <- "build";

In this example, output files will be deposited in a subdirectory named build, located where comp_translator is run.

CLIENT_FILES
""""""""""""

**Type:** string pairs

This list of string pairs, separated by a comma, defines the name of an XML file located in BinaryData/Client, along with what binary data format it shall be encoded in to build the equivalent bin file for the game.

Example
#######

.. code-block:: javascript

    CLIENT_FILES <- {
    "UIInfoData": "uiinfo",
    };

In this example, ``UIInfoData.xml``, located in BinaryData/Client, will be built using the uiinfo format into a bin usable by the game.

SHIELD_FILES
""""""""""""

**Type:** string pairs

This list of string pairs, separated by a comma, defines the name of an XML file located in BinaryData/Shield, along with what binary data format it shall be encoded in to build the equivalent bin file for the game.

Example
#######

.. code-block:: javascript

    SHIELD_FILES <- {
    "CBattleTalk": "ctalkmessage",
    };

In this example, ``CBattleTalk.xml``, located in BinaryData/Shield, will be built using the ctalkmessage format into an sbin file usable by the game.

Comments on Testing Cutscenes
-----------------------------

Cutscenes are found in the Event/PolygonMovie/BinaryData directory, all with names starting with "CMovieData." In order to test a modified cutscene, it is recommended to replace the movie file ``CMovieData_OP_001.bin`` in a working client with the cutscene to test. 

Once this is done, load a character into the Virtual Battle access room and speak to this NPC. The movie to test may be opened by selecting "Play a Vizionz" followed by "Opening." You may continuously swap in other cutscene files to test them without needing to close and reopen the game.

.. image:: /_static/vizionz_girl.png

