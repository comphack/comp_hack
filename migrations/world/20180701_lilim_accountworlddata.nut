import("server");

function up(db, server)
{
    local chars = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Character"), db);

    local awds = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("AccountWorldData"), db);

    print("Checking " + awds.len() + " AccountWorldData records.");

    foreach(awd in awds)
    {
        local worldData = ToAccountWorldData(awd);

        if(!worldData.Account.IsNull())
        {
            local accountUID = worldData.Account.GetUUID();

            print("Processing account '" + accountUID.ToString() +
                "' for AccountWorldData updates.");

            local updated = false;
            foreach(c in chars)
            {
                local character = ToCharacter(c);
            
                if(character.Account == accountUID &&
                    !character.Progress.IsNull())
                {
                    local progress = character.Progress.Get(db);
                    if(progress != null)
                    {
                        // Merge the devil book values from the old location
                        for(local i = 0; i < 128; i++)
                        {
                            local newVal = awd.GetDevilBook(i);
                            local oldVal = progress.GetDevilBook(i);
                            newVal = newVal | oldVal;
                            awd.SetDevilBook(i, newVal);

                            updated = updated || (newVal != oldVal);
                        }
                    }
                }
            }
            
            if(updated)
            {
                print("Updating AccountWorldData on Account " +
                    accountUID.ToString());

                if(!awd.Update(db))
                {
                    print("ERROR: AccountWorldData update failed");
                    return false;
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
