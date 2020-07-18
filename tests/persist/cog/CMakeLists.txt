
LINK_LIBRARIES(
    persist-dht
    persist
    atomspace
)

# The seven unit tests, ported over from the
# atomspace/persist/sql/multi-driver unit tests.
# Same as the atomspace/persist/ipfs unit tests.
ADD_CXXTEST(BasicSaveUTest)
ADD_CXXTEST(ValueSaveUTest)
ADD_CXXTEST(PersistUTest)
ADD_CXXTEST(FetchUTest)
TARGET_LINK_LIBRARIES(FetchUTest execution)
ADD_CXXTEST(DeleteUTest)
ADD_CXXTEST(MultiPersistUTest)
ADD_CXXTEST(MultiUserUTest)

# XXX FIXME Disable these two tests for now; they hang
# (take forever to run) Don't know why. Needs fixing.
# ADD_CXXTEST(LargeFlatUTest)
# ADD_CXXTEST(LargeZipfUTest)
