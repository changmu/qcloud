INCLUDEPATH += "D:/Program Files/include"
INCLUDEPATH += "."

HEADERS += \
    Server.h \
    Util.h \
    ConnectionContext.h \
    InodeInfo.h \
    protocol.h \
    UserInfo.h

SOURCES += \
    main.cpp \
    Server.cpp \
    Util.cpp \
    unit_test/server_test.cpp \
    ConnectionContext.cpp \
    InodeInfo.cpp \
    UserInfo.cpp
