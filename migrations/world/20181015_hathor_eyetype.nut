import("server");

function up(db, server)
{
    local objs = PersistentObject.LoadObjects(
        PersistentObject.GetTypeHashByName("Character"), db);

    print("Checking " + objs.len() + " characters.");

    foreach(obj in objs)
    {
        local character = ToCharacter(obj);

        local eyeType = character.Gender == 0
            ? ((character.GetFaceType() - 1) % 3 + 1)
            : ((character.GetFaceType() - 101) % 3 + 101);
        if(character.GetEyeType() != eyeType)
        {
            character.SetEyeType(eyeType);
            if(!character.Update(db))
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
