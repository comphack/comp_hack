import("server");

function up(db, server)
{
    local objs = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Demon"), db);

    print("Checking " + objs.len() + " demons.");

    foreach(obj in objs)
    {
        local updated = false;

        local demon = ToDemon(obj);
        for(local i = demon.AcquiredSkillsCount() - 1; i >= 0; i--)
        {
            if(demon.GetAcquiredSkillsByIndex(i) == 0)
            {
                demon.RemoveAcquiredSkills(i);
                updated = true;
            }
        }

        if(updated)
        {
            print("Fixing blank acquired skills on " +
                demon.GetUUID().ToString());
            if(!demon.Update(db))
            {
                print("ERROR: Demon update failed");
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
