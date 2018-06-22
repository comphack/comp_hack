import("server");

function up(db, server)
{
    local objs = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Character"), db);

    print("Checking " + objs.len() + " characters.");

    foreach(obj in objs)
    {
        local character = ToCharacter(obj);

        if(!character.Progress.IsNull())
        {
            local progress = character.Progress.Get(db, true);

            local val = progress.GetSpecialTitlesByIndex(127);
            if(0 == (val & 0xC0))
            {
                print("Adding default title values to " +
                    character.GetUUID().ToString());

                progress.SetSpecialTitlesByIndex(127, val | 0xC0);

                character.SetCustomTitlesByIndex(0, 1022);
                character.SetCustomTitlesByIndex(1, 1023);
                character.SetCustomTitlesByIndex(2, 0);
                character.SetCustomTitlesByIndex(3, 0);
                character.SetCustomTitlesByIndex(4, 0);
                character.SetCustomTitlesByIndex(5, 0);
                character.SetCustomTitlesByIndex(6, 0);
                character.SetCustomTitlesByIndex(7, 0);
                character.SetCustomTitlesByIndex(8, 0);
                character.SetCustomTitlesByIndex(9, 0);
                character.SetCustomTitlesByIndex(10, 0);
                character.SetCustomTitlesByIndex(11, 0);
                character.SetCustomTitlesByIndex(12, 0);

                if(!progress.Update(db) || !character.Update(db))
                {
                    print("ERROR: CharacterProgress or Character update failed");
                    return false;
                }
            }
        }

        // Skip ATTACK as it will be correct already
        for(local i = 1; i < 38; i++)
        {
            if(!character.GetExpertisesByIndex(i).IsNull())
            {
                local expertise = character.GetExpertisesByIndex(i).Get(db, true);

                if(expertise.GetExpertiseID() == 0)
                {
                    print("Setting ID for expertise " + i +
                        " on character " + character.GetUUID().ToString());

                    expertise.SetExpertiseID(i);

                    if(!expertise.Update(db))
                    {
                        print("ERROR: Expertise update failed");
                        return false;
                    }
                }
            }
        }

        // Skip first page that is correct already
        for(local i = 1; i < 10; i++)
        {
            if(!character.GetHotbarsByIndex(i).IsNull())
            {
                local hotbar = character.GetHotbarsByIndex(i).Get(db, true);

                if(hotbar.GetPageID() == 0)
                {
                    print("Setting page ID for hotbar " + i +
                        " on character " + character.GetUUID().ToString());

                    hotbar.SetPageID(i);

                    if(!hotbar.Update(db))
                    {
                        print("ERROR: Hotbar update failed");
                        return false;
                    }
                }
            }
        }
    }

    return true;
}

function down(db)
{
    return true;
}
