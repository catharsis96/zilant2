
# Target platform
ifndef TARGET
	TARGET=srf06-cc26xx
	BOARD=sensortag
endif

# Name of the .c file containing the main application
CONTIKI_PROJECT_RX = myproject_rx
CONTIKI_PROJECT_TX = myproject_tx

# What to compile
all-rx: $(CONTIKI_PROJECT_RX)
all-tx: $(CONTIKI_PROJECT_TX)

# Upload
upload-rx: $(CONTIKI_PROJECT_RX).upload
upload-tx: $(CONTIKI_PROJECT_TX).upload

# Path to custom configuration file
CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# Additional source files to be compiled (if any)
#CONTIKI_TARGET_SOURCEFILES += library.c

CONTIKI_WITH_IPV6 = 1

# Path to main Contiki folder
CONTIKI = ../../contiki
# Include Contiki makefile
include $(CONTIKI)/Makefile.include
