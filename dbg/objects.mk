USER_OBJS :=

LIBS := `pkg-config --libs opencv` -lboost_system -lboost_filesystem -lboost_exception -lboost_thread -lboost_chrono -lboost_serialization

