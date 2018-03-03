function ProcessLoginRequest(req)
{
    req.pageError = "index.nut";

    if(req.postVarExists("quit"))
    {
        req.operation = Operation_t.OPERATION_QUIT;
        req.page = "quit.nut";
    }
    else if(req.postVarExists("login"))
    {
        req.operation = Operation_t.OPERATION_LOGIN;
        req.page = "authenticated.nut";
    }
    else
    {
        req.operation = Operation_t.OPERATION_GET;
        req.page = "index.nut";
    }

    if(req.postVarExists("ID"))
    {
        req.username = req.postVar("ID");
    }

    if(req.postVarExists("PASS"))
    {
        req.password = req.postVar("PASS");
    }

    if(req.postVarExists("IDSAVE"))
    {
        req.rememberUsername = "on" == req.postVar("IDSAVE");
    }
    else
    {
        req.rememberUsername = false;
    }

    if(req.postVarExists("cv"))
    {
        try
        {
            req.clientVersion = req.postVar("cv").tofloat();
        }
        catch(error)
        {
            req.clientVersion = 0.0;
        }
    }
    else
    {
        req.clientVersion = 0.0;
    }

    return true;
}

function ProcessLoginReply(reply)
{
    if("" != reply.errorMessage)
    {
        local msg = "<span style=\"font-size:12px;color:#edb81e;" +
            "font-weight:bold;\"><br>&nbsp;";
        msg += reply.errorMessage;
        msg += "</span>";

        reply.replaceVarSet("{COMP_HACK_MSG}", msg);
    }
    else
    {
        reply.replaceVarSet("{COMP_HACK_MSG}", "");
    }

    if(reply.lockControls)
    {
        reply.replaceVarSet("{COMP_HACK_SUBMIT}",
            "<input class=\"login_disabled\" type=\"submit\" " +
            "value=\"\" tabindex=\"4\" name=\"login\" height=\"60\" " +
            "width=\"67\" />");

        local readonly = "readonly=\"readonly\" ";

        // Make sure the user can't edit these (they can only quit).
        reply.replaceVarSet("{COMP_HACK_ID_READONLY}", readonly);
        reply.replaceVarSet("{COMP_HACK_PASS_READONLY}", readonly);
        reply.replaceVarSet("{COMP_HACK_IDSAVE_READONLY}", readonly);
    }
    else
    {
        reply.replaceVarSet("{COMP_HACK_SUBMIT}",
            "<input class=\"login\" type=\"submit\" " +
            "value=\"\" tabindex=\"4\" name=\"login\" height=\"60\" " +
            "width=\"67\" />");

        reply.replaceVarSet("{COMP_HACK_ID_READONLY}", "");
        reply.replaceVarSet("{COMP_HACK_PASS_READONLY}", "");
        reply.replaceVarSet("{COMP_HACK_IDSAVE_READONLY}", "");
    }

    reply.replaceVarSet("{COMP_HACK_ID}", reply.username);
    reply.replaceVarSet("{COMP_HACK_PASS}", reply.password);

    if(reply.rememberUsername)
    {
        reply.replaceVarSet("{COMP_HACK_IDSAVE}", "checked");
        reply.replaceVarSet("{COMP_HACK_IDSAVE_INT}", "1");
    }
    else
    {
        reply.replaceVarSet("{COMP_HACK_IDSAVE}", "");
        reply.replaceVarSet("{COMP_HACK_IDSAVE_INT}", "0");
    }

    reply.replaceVarSet("{COMP_HACK_BIRTHDAY}", "1");
    reply.replaceVarSet("{COMP_HACK_CV_INPUT}",
        reply.clientVersion.tostring());
    /// @todo There was a message "~ Client needs to be updated ~" here.
    reply.replaceVarSet("{COMP_HACK_CV}",
        reply.clientVersion.tostring());
    reply.replaceVarSet("{COMP_HACK_SID1}", reply.sid1);
    reply.replaceVarSet("{COMP_HACK_SID2}", reply.sid2);

    return true;
}
