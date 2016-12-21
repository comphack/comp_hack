virtual std::list<libcomp::DatabaseBind*> GetMemberBindValues();
virtual bool LoadDatabaseValues(libcomp::DatabaseQuery& query);
virtual std::shared_ptr<libobjgen::MetaObject> GetObjectMetadata();
static std::shared_ptr<libobjgen::MetaObject> GetMetadata();
