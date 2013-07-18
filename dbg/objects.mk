USER_OBJS :=

LIBS := `pkg-config --libs opencv` -lboost_system -lboost_exception -lboost_thread -lboost_chrono
LIBS := `pkg-config --libs opencv` -lboost_system -lboost_filesystem -lboost_thread -lboost_serialization

