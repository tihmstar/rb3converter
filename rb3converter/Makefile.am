AM_CFLAGS =  $(GLOBAL_CFLAGS) $(openssl_CFLAGS) $(libgeneral_CFLAGS)
AM_CXXFLAGS = $(AM_CFLAGS) $(GLOBAL_CXXFLAGS)
AM_LDFLAGS = $(openssl_LIBS) $(libgeneral_LIBS) -lpthread

bin_PROGRAMS = rb3converter

rb3converter_CFLAGS = $(AM_CFLAGS)
rb3converter_CXXFLAGS = $(AM_CXXFLAGS)
rb3converter_LDFLAGS = $(AM_LDFLAGS)
rb3converter_SOURCES =	main.cpp \
												ConvertMid.cpp \
												ConvertMogg.cpp \
												ConvertPNG.cpp \
												dtaParser.cpp \
												FileLoader.cpp \
												NPD.cpp \
												songsManager.cpp \
												STFS.cpp
