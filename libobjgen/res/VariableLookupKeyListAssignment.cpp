std::list<std::shared_ptr<@OBJECT_NAME@>> @RETURN_VAR@;
for(auto obj : LoadObjects(typeid(@OBJECT_NAME@).hash_code(), db, bind))
{
    @RETURN_VAR@.push_back(std::dynamic_pointer_cast<@OBJECT_NAME@>(obj));
}
