[&]() {
    std::stringstream stream(std::stringstream::out |
        std::stringstream::binary);
    @SAVE_CODE@;

    auto stringData = stream.str();
    auto data = std::vector<char>(stringData.c_str(), stringData.c_str() +
        stringData.size());

    return new libcomp::DatabaseBindBlob(@COLUMN_NAME@, data);
}