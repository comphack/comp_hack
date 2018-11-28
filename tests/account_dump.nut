include("test.nut");

account_idx <- 0;

c <- ChannelClient();
C(LoginWithCharacter(c, account_idx, "Test1"));
C(c.SendState());
C(c.SendPopulateZone());
C(c.AmalaRequestAccountDump());
c.Disconnect();
