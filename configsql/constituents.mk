libnames := configsql

libsrcs_configsql := DatabaseError.cc ValueTypeMismatch.cc WrongParams.cc
libsrcs_configsql += QueryProcessor.cc
libsrcs_configsql += DbClient.cc
libsrcs_configsql += XtcClient.cc
libincs_configsql := pdsdata/include
