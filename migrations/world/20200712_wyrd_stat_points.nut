import("server");

function up(db, server)
{
    local objs = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Character"), db);

    print("Checking " + objs.len() + " characters.");

    foreach(obj in objs)
    {
        local c = ToCharacter(obj);

        if(!c.CoreStats.IsNull())
        {
            local stats = c.CoreStats.Get(db, true);
            local lvl = stats.GetLevel();
            local points = c.GetPoints();
            local gain = lvl / 5;

            c.SetPoints(points + gain);

            print("Level " + lvl + " character was given an extra " +
                gain + " points.");

            if(!c.Update(db))
            {
                print("ERROR: Character update failed");
                return false;
            }
        }
    }

    return true;
}

function down(db)
{
    return true;
}
