std::list<libcomp::DatabaseBind*> @OBJECT_NAME@::GetMemberBindValues()
{
    std::list<libcomp::DatabaseBind*> values;
    @BINDS@
    return values;
}

bool @OBJECT_NAME@::LoadDatabaseValues(libcomp::DatabaseQuery& query)
{
    @GET_DATABASE_VALUES@

    if(!query.GetValue("uid", mUUID))
    {
        return false;
    }

    return true;
}

std::shared_ptr<libobjgen::MetaObject> @OBJECT_NAME@::GetObjectMetadata()
{
    return @OBJECT_NAME@::GetMetadata();
}

std::shared_ptr<libobjgen::MetaObject> @OBJECT_NAME@::GetMetadata()
{
    auto m = libcomp::PersistentObject::GetRegisteredMetadata(typeid(@OBJECT_NAME@));
    if(nullptr == m)
    {
        static char metadataBytes[@BYTE_COUNT@] = { @BYTES@ };
        m = libcomp::PersistentObject::GetMetadataFromBytes(metadataBytes, @BYTE_COUNT@);
    }

    if(nullptr == m)
    {
        LOG_CRITICAL("Metadata for object '@OBJECT_NAME@' could not be generated.\n");
        sInitializationFailed = true;
    }

    return m;
}
