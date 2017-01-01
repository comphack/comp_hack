@RETURN_TYPE@ @OBJECT_NAME@::Load@RETURN_NAME@By@LOOKUP_TYPE@(const std::shared_ptr<libcomp::Database>& db, @ARGUMENTS@)
{
    std::list<libcomp::DatabaseBind*> bindings;
    @BINDINGS@
    @ASSIGNMENT_CODE@
    for(auto bind : bindings)
    {
        delete bind;
    }

    return @RETURN_VAR@;
}
