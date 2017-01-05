@RETURN_TYPE@ @OBJECT_NAME@::Load@RETURN_NAME@By@LOOKUP_TYPE@(const std::shared_ptr<libcomp::Database>& db, @ARGUMENT@)
{
    libcomp::DatabaseBind* bind = @BINDING@();
    @ASSIGNMENT_CODE@
    delete bind;

    return @RETURN_VAR@;
}
