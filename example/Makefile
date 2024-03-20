include file_list.mk

RM              = rm -rf
#DEFINES         = -DWMA_INCLUDE -DUSE_TAGLIB
DEFINES = -DWMA_INCLUDE 
DEBUG_FLAG      = -g3 -O0
SRC_DIR         = ./src
OBJ_DIR         = ./obj
DEST_DIR        = ./bin
DEST_NAME       = simpledb
SRCS            = $(addprefix $(SRC_DIR)/, $(SOURCES))
OBJS            = $(addprefix $(OBJ_DIR)/, $(patsubst %.c, %.o, $(SOURCES)))
INC_PATH        = -I./include -I./tagparser/include
LIBS            = -L./libs/ -lTcDBGen -lm -lpthread

pkg_packages 	:= sqlite3 glib-2.0 libxml-2.0 taglib_c

PKG_CFLAGS  	:= $(shell pkg-config --cflags $(pkg_packages))
PKG_LDFLAGS 	:= $(shell pkg-config --libs $(pkg_packages))

CFLAGS  		:= $(CFLAGS) $(PKG_CFLAGS) $(DEFINES) $(INC_PATH)
LDFLAGS 		:= $(LDFLAGS) $(PKG_LDFLAGS) $(LIBS)

TARGET          = $(DEST_DIR)/$(DEST_NAME)

#.SUFFIXES: .c
#.c.o:
#	$(CC) -c $(CFLAGS) $(INC_PATH) -o $@ $<

$(OBJ_DIR)/%.o:$(SRC_DIR)/%.c
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEBUG_FLAG) -c -o $@ $<

first: all

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(DEST_DIR)
	$(CC) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	$(RM) $(OBJ_DIR)/*.o $(TARGET)


