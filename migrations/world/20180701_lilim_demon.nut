import("server");

function up(db, server)
{
    local objs = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Demon"), db);

    print("Checking " + objs.len() + " demons.");

    local defman = DefinitionManager();
    defman.LoadAllData(server.GetDataStore());

    foreach(obj in objs)
    {
        local demon = ToDemon(obj);

        if(demon.GetGrowthType() == 0)
        {
            local def = defman.GetDevilData(demon.GetType());
            if(def)
            {
                local defaultType = def.GetGrowth().GetGrowthType();
                if(defaultType != 0)
                {
                    print("Setting default growth type on " +
                        demon.GetUUID().ToString());

                    demon.SetGrowthType(defaultType);

                    if(!demon.Update(db))
                    {
                        print("ERROR: Demon update failed");
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
