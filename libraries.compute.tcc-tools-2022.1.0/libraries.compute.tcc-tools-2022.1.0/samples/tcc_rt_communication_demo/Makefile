BUILD_DIR=./build
INSTALL_DIR="./install"
OPCUA_SERVER_INCLUDE_DIR="src/opcua_pubsub_server"
COMMON_SOURCE_DIR=../common/src
COMMON_INCLUDE_DIR=../common/include
SRC_DIR=src/opcua_pubsub_server
DATA_MGR_SRC_DIR=src/data_mgr
CFLAGS+=-O2 -fstack-protector-strong -fPIE -fPIC -D_FORTIFY_SOURCE=2
CFLAGS+=-Wall -Wno-parentheses -Wformat -Wformat-security
CFLAGS+=-I $(COMMON_INCLUDE_DIR)
CFLAGS+=$(shell pkg-config --cflags open62541-iotg)
CFLAGS_SERVER=$(CFLAGS)
CFLAGS_SERVER+=-I $(DATA_MGR_SRC_DIR) -I $(OPCUA_SERVER_INCLUDE_DIR)
CFLAGS_DATA_MGR=$(CFLAGS)
CFLAGS_DATA_MGR+=-I $(OPCUA_SERVER_INCLUDE_DIR)
CFLAGS_DATA_MGR+=-DDATA_MGR_TIMER_RT_THREAD

LDFLAGS+=-pie -z noexecstack -z relro -z now
LDLIBS_SERVER=-lrt -lm -pthread -Wl,--no-as-needed -ljson-c -lrtcdatamgr -lelf
LDLIBS_SERVER+=$(shell pkg-config --libs open62541-iotg)

LDLIBS_DATA_MGR=-lrt -pthread -ltcc -ltcc_cache -ltcc_static -littnotify -ldl

# Builds tcc_rt_communication_demo
all: opcua_server librtcdatamgr.so

opcua_server: dirs librtcdatamgr.so $(BUILD_DIR)/json_helper.o \
	$(BUILD_DIR)/multicallback_server.o $(BUILD_DIR)/opcua_common.o \
	$(BUILD_DIR)/opcua_publish.o $(BUILD_DIR)/opcua_subscribe.o \
	$(BUILD_DIR)/opcua_datasource.o $(BUILD_DIR)/opcua_custom.o
	$(CC) -L $(BUILD_DIR) \
	$(BUILD_DIR)/json_helper.o $(BUILD_DIR)/multicallback_server.o \
	$(BUILD_DIR)/opcua_common.o $(BUILD_DIR)/opcua_publish.o \
	$(BUILD_DIR)/opcua_subscribe.o $(BUILD_DIR)/opcua_datasource.o \
	$(BUILD_DIR)/opcua_custom.o \
	$(LDFLAGS) $(LDLIBS_SERVER) -o $(BUILD_DIR)/$@

librtcdatamgr.so: dirs $(BUILD_DIR)/listener.o $(BUILD_DIR)/compute.o \
	$(BUILD_DIR)/talker.o $(BUILD_DIR)/ptimer.o \
	$(BUILD_DIR)/pdatasource.o \
	$(BUILD_DIR)/pointer_chasing.o \
	$(BUILD_DIR)/pointer_chasing_workload.o
	make -C ../common all
	$(CC) -shared $(BUILD_DIR)/listener.o $(BUILD_DIR)/compute.o \
	$(BUILD_DIR)/talker.o $(BUILD_DIR)/ptimer.o \
	$(BUILD_DIR)/pointer_chasing.o \
	$(BUILD_DIR)/pdatasource.o $(BUILD_DIR)/pointer_chasing_workload.o \
	$(LDLIBS_DATA_MGR) -o $(BUILD_DIR)/$@

$(BUILD_DIR)/json_helper.o: $(SRC_DIR)/json_helper.c \
	$(SRC_DIR)/json_helper.h $(SRC_DIR)/opcua_utils.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/json_helper.c -o $@

$(BUILD_DIR)/multicallback_server.o: $(SRC_DIR)/multicallback_server.c \
	$(SRC_DIR)/opcua_common.h $(SRC_DIR)/opcua_publish.h \
	$(SRC_DIR)/opcua_subscribe.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/multicallback_server.c -o $@

$(BUILD_DIR)/opcua_common.o: $(SRC_DIR)/opcua_common.c \
	$(SRC_DIR)/opcua_common.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/opcua_common.c -o $@

$(BUILD_DIR)/opcua_publish.o: $(SRC_DIR)/opcua_publish.c \
	$(SRC_DIR)/opcua_common.h $(SRC_DIR)/json_helper.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/opcua_publish.c -o $@

$(BUILD_DIR)/opcua_subscribe.o: $(SRC_DIR)/opcua_publish.c \
	$(SRC_DIR)/opcua_common.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/opcua_subscribe.c -o $@

$(BUILD_DIR)/opcua_custom.o: $(SRC_DIR)/opcua_custom.c \
	$(SRC_DIR)/opcua_custom.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/opcua_custom.c -o $@

$(BUILD_DIR)/opcua_datasource.o: $(SRC_DIR)/opcua_datasource.c \
	$(SRC_DIR)/opcua_datasource.h
	$(CC) $(CFLAGS_SERVER) -c $(SRC_DIR)/opcua_datasource.c -o $@

$(BUILD_DIR)/listener.o: $(DATA_MGR_SRC_DIR)/pconfig.h \
	$(DATA_MGR_SRC_DIR)/pcheck.h $(DATA_MGR_SRC_DIR)/ptimer.h \
	$(DATA_MGR_SRC_DIR)/listener.c
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/listener.c -o $@

$(BUILD_DIR)/compute.o: $(DATA_MGR_SRC_DIR)/pconfig.h \
	$(DATA_MGR_SRC_DIR)/pcheck.h $(DATA_MGR_SRC_DIR)/ptimer.h \
	$(DATA_MGR_SRC_DIR)/compute.c
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/compute.c -o $@

$(BUILD_DIR)/talker.o: $(DATA_MGR_SRC_DIR)/pconfig.h \
	$(DATA_MGR_SRC_DIR)/pcheck.h $(DATA_MGR_SRC_DIR)/ptimer.h \
	$(DATA_MGR_SRC_DIR)/talker.c
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/talker.c -o $@

$(BUILD_DIR)/ptimer.o: $(DATA_MGR_SRC_DIR)/ptimer.h \
	$(DATA_MGR_SRC_DIR)/ptimer.c $(DATA_MGR_SRC_DIR)/pcheck.h
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/ptimer.c -o $@

$(BUILD_DIR)/pdatasource.o: $(DATA_MGR_SRC_DIR)/pdatasource.h \
	$(DATA_MGR_SRC_DIR)/pdatasource.c $(DATA_MGR_SRC_DIR)/pcheck.h
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/pdatasource.c -o $@

$(BUILD_DIR)/pointer_chasing_workload.o: $(DATA_MGR_SRC_DIR)/pointer_chasing_workload.h \
	$(DATA_MGR_SRC_DIR)/pointer_chasing_workload.c
	$(CC) $(CFLAGS_DATA_MGR) -c $(DATA_MGR_SRC_DIR)/pointer_chasing_workload.c -o $@

$(BUILD_DIR)/pointer_chasing.o: $(COMMON_SOURCE_DIR)/pointer_chasing.c \
	$(COMMON_INCLUDE_DIR)/pointer_chasing.h
	$(CC) $(CFLAGS_DATA_MGR) -c $(COMMON_SOURCE_DIR)/pointer_chasing.c -o $@

# Copies sample executable, all required scripts, and config files
# to a single folder to simplify copying to remote machines
install: opcua_server librtcdatamgr.so
	mkdir -p $(INSTALL_DIR)
	cp $(BUILD_DIR)/opcua_server $(INSTALL_DIR)/.
	cp $(BUILD_DIR)/librtcdatamgr.so $(INSTALL_DIR)/.
	cp -r src/scripts/* $(INSTALL_DIR)/.
	ln -f -s ./tcc_rt_communication_demo.py $(INSTALL_DIR)/tcc_rt_communication_demo

dirs: $(BUILD_DIR)
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
clean:
	rm -rf $(BUILD_DIR)/*
