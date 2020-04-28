CONTIKI_PROJECT = vlc
all: $(CONTIKI_PROJECT)

# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

# include sdlib
MODULES_REL += sdlib

# Include CoAP resources
MODULES_REL += ./resources

CONTIKI = ../..

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
include $(CONTIKI)/Makefile.include