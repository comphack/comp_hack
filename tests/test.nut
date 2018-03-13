include("config.nut");

function C(cond)
{
    if(!cond)
    {
        throw "Test failed! Exiting now...\n";
    }
}

function GetAccountCount()
{
    return ACCOUNTS.len();
}

function LoginAccountToLobby(client, accountIndex)
{
    if(0 <= accountIndex || ACCOUNTS.len() > accountIndex)
    {
        local account = ACCOUNTS[accountIndex];
        local username = account["username"];
        local password = account["password"];

        return client.ClassicLogin(username, password);
    }

    return false;
}

function LoginAccountToChannel(client, accountIndex, character = "")
{
    if(0 <= accountIndex || ACCOUNTS.len() > accountIndex)
    {
        local account = ACCOUNTS[accountIndex];
        local username = account["username"];
        local password = account["password"];

        return client.Login(username, password, character);
    }

    return false;
}

function LoginWithCharacter(client, accountIndex, character)
{
    if(0 <= accountIndex || ACCOUNTS.len() > accountIndex)
    {
        local account = ACCOUNTS[accountIndex];

        local c = LobbyClient();

        C(LoginAccountToLobby(c, 0));
        C(c.GetCharacterList());

        // Create the character if it does not exist.
        if(0 > c.GetCharacterID(character))
        {
            C(c.CreateCharacter(character));
            C(c.GetCharacterList());
        }

        cid <- c.GetCharacterID(character);
        wid <- c.GetCharacterID(character);

        C(c.StartGame(cid, wid));
        sessionKey <- c.GetSessionKey();

        C(client.LoginWithKey(account["username"], sessionKey));
        C(client.SendData());

        return true;
    }

    return false;
}

function LoginWithNewCharacter(client, accountIndex, character)
{
    if(0 <= accountIndex || ACCOUNTS.len() > accountIndex)
    {
        local account = ACCOUNTS[accountIndex];

        local c = LobbyClient();

        C(LoginAccountToLobby(c, 0));
        C(c.GetCharacterList());

        cid <- c.GetCharacterID(character);

        // Create the character if it does not exist.
        if(0 <= cid)
        {
            C(c.DeleteCharacter(cid));
        }

        C(c.CreateCharacter(character));
        C(c.GetCharacterList());

        cid <- c.GetCharacterID(character);
        wid <- c.GetCharacterID(character);

        C(c.StartGame(cid, wid));
        sessionKey <- c.GetSessionKey();

        C(client.LoginWithKey(account["username"], sessionKey));
        C(client.SendData());

        return true;
    }

    return false;
}
