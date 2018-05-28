include("test.nut");

account_idx <- 0;

c <- ChannelClient();
C(LoginWithCharacter(c, account_idx, "Test1"));
C(c.SendState());
C(c.SendPopulateZone());
C(c.Say("@effect 3 5"));
sleep(3);
c.Disconnect();
c <- ChannelClient();
sleep(3);
C(LoginWithCharacter(c, account_idx, "Test1"));
C(c.SendState());
C(c.SendPopulateZone());
sleep(15); // status effect lasts 15 seconds
C(c.Say("@effect 3 5"));
sleep(3);
c.Disconnect();
c <- ChannelClient();
sleep(3);
C(LoginWithCharacter(c, account_idx, "Test1"));

// Cleanup after...
c.Disconnect();
c <- LobbyClient();
C(LoginAccountToLobby(c, account_idx));
C(c.GetCharacterList());
C(-1 != c.GetCharacterID("Test1"));
C(c.DeleteCharacter(c.GetCharacterID("Test1")));
