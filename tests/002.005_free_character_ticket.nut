include("test.nut");

account_idx <- 0;

// 1. Create a new account.
// 2. Log into the lobby with the account.
c <- LobbyClient();
C(LoginAccountToLobby(c, account_idx));
C(c.GetCharacterList());
C(-1 == c.GetCharacterID("Test1"));
C(-1 == c.GetCharacterID("Test2"));

C(c.QueryTicketPurchase())
cp_balance <- c.GetCP();

// 3. Create a character.
C(c.CreateCharacter("Test1"));
C(c.GetCharacterList());
C(-1 != c.GetCharacterID("Test1"));

// 4. Ensure the ticket count is 0.
C(0 == c.GetTicketCount());

// 5. Mark the character for deletion.
C(c.DeleteCharacter(c.GetCharacterID("Test1")));

// 6. Wait for the character to delete.
// 7. Ensure there is no characters listed.
C(c.GetCharacterList());
C(-1 == c.GetCharacterID("Test1"));
C(-1 == c.GetCharacterID("Test2"));

// 8. Ensure the ticket count is now 1.
C(1 == c.GetTicketCount());

// 9. Create a character.
C(c.CreateCharacter("Test2"));
C(c.GetCharacterList());
C(-1 != c.GetCharacterID("Test2"));

// 10. Ensure the CP balance has not changed.
C(c.QueryTicketPurchase())
C(cp_balance == c.GetCP());

// 11. Ensure the ticket count is 0.
C(0 == c.GetTicketCount());

// 12. Mark the character for deletion.
C(c.DeleteCharacter(c.GetCharacterID("Test2")));

// 13. Wait for the character to delete.
// 14. Ensure there is no characters listed.
C(c.GetCharacterList());
C(-1 == c.GetCharacterID("Test1"));
C(-1 == c.GetCharacterID("Test2"));

// 15. Ensure the ticket count is now 1.
C(1 == c.GetTicketCount());

// 16. Log out and back into the lobby.
c <- LobbyClient();
C(LoginAccountToLobby(c, account_idx));
C(c.GetCharacterList());

// 17. Ensure the ticket count is still 1.
C(1 == c.GetTicketCount());

// 18. Create a character.
C(c.CreateCharacter("Test1"));
C(c.GetCharacterList());
C(-1 != c.GetCharacterID("Test1"));

// 19. Ensure the CP balance has not changed.
C(c.QueryTicketPurchase())
C(cp_balance == c.GetCP());

// 20. Ensure the ticket count is 0.
C(0 == c.GetTicketCount());

// Cleanup after...
C(c.DeleteCharacter(c.GetCharacterID("Test1")));
