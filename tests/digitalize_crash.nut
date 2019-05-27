include("test.nut");

account_idx <- 0;

ACTIVATION_DEMON <- 2;

c <- ChannelClient();
C(LoginWithCharacter(c, account_idx, "Test1"));
C(c.SendState());
C(c.SendPopulateZone());
// C(c.ContractDemon(569));
// C(c.Say("@item mag 50000"));
// C(c.Say("@levelup 11"));
// C(c.Say("@instance 5403 60000"));
// C(c.Say("@zone 540101"));
// C(c.SendPopulateZone()); // we need to make ourself visible to the zone before we execute the skill
// C(c.Say("@valuable 686"));
// C(c.ActivateSkill(c.GetEntityID(), 50387, ACTIVATION_DEMON, c.GetDemonID(0)));
// sleep(4);
// C(c.Say("@event Z540101_540X_10000_05"));
// sleep(1);

// Cleanup after...
c.Disconnect();
c <- LobbyClient();
C(LoginAccountToLobby(c, account_idx));
C(c.GetCharacterList());
C(-1 != c.GetCharacterID("Test1"));
C(c.DeleteCharacter(c.GetCharacterID("Test1")));
